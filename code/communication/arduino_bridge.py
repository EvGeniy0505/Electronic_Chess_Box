import os
import sys
import serial
import time
from serial.tools import list_ports
from PyQt6.QtWidgets import QMessageBox

class ArduinoBridge:
    def __init__(self):
        self.serial = None
        self.connected = False
        self.last_board_state = [0] * 8  # 8x8 битовая маска (64 клетки)
        
    def connect(self, port_name=None, baudrate=9600):
        """Подключение к Arduino через Bluetooth"""
        try:
            if port_name:
                self.serial = serial.Serial(port_name, baudrate, timeout=1)
            else:
                # Автопоиск Bluetooth-порта
                ports = list_ports.comports()
                for port in ports:
                    if "Bluetooth" in port.description or "HC-05" in port.device:
                        self.serial = serial.Serial(port.device, baudrate, timeout=1)
                        break
            
            if self.serial:
                self.connected = True
                print(f"Connected to Arduino Bluetooth at {self.serial.port}")
                # Инициализация - запросить текущее состояние
                self.serial.write(b'INIT\n')
                return True
        except Exception as e:
            print(f"Bluetooth connection error: {e}")
        return False

    def read_board_state(self):
        """Чтение состояния доски (8 байт)"""
        if not self.connected:
            return None
            
        try:
            self.serial.write(b'GET\n')
            response = self.serial.read(8)  # Ждём 8 байт
            if len(response) == 8:
                return list(response)
        except Exception as e:
            print(f"Read error: {e}")
            self.connected = False
        return None

    def highlight_move(self, from_pos, to_pos, duration=1.0):
        """Подсветка хода на LED ленте"""
        if not self.connected:
            return False
            
        try:
            # Формат: "LED a1 h8 1.0\n" (последнее число - длительность в секундах)
            cmd = f"LED {from_pos} {to_pos} {duration}\n".encode()
            self.serial.write(cmd)
            return True
        except Exception as e:
            print(f"LED control error: {e}")
        return False

    def close(self):
        """Закрытие соединения"""
        if self.serial and self.connected:
            self.serial.close()
            self.connected = False

def start_arduino_bridge():
    """Запускает мост к Arduino в отдельном процессе"""
    try:
        # Создаем FIFO каналы для связи
        os.makedirs("./build", exist_ok=True)
        if not os.path.exists("./build/arduino_in"):
            os.mkfifo("./build/arduino_in")
        if not os.path.exists("./build/arduino_out"):
            os.mkfifo("./build/arduino_out")
            
        # Запускаем процесс
        import subprocess
        return subprocess.Popen(
            [sys.executable, os.path.join(os.path.dirname(__file__), "arduino_bridge.py")],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
    except Exception as e:
        QMessageBox.warning(None, "Arduino Error", f"Failed to start bridge: {str(e)}")
        return None

def analyze_board_changes(old_state, new_state):
    """Анализ изменений на доске и преобразование в шахматный ход"""
    # Здесь должна быть логика сравнения old_state и new_state
    # и преобразования в нотацию типа "e2e4"
    
    # Заглушка - в реальной реализации нужно:
    # 1. Найти различия между old_state и new_state
    # 2. Определить начальную и конечную позицию
    # 3. Преобразовать в шахматную нотацию (например, "e2e4")
    return "e2e4"

def fifo_communication_loop():
    """Основной цикл обмена через FIFO"""
    os.makedirs("./build", exist_ok=True)
    
    # FIFO для связи с GUI (приём ходов от Lichess)
    lichess_in = "./build/lichess_in"
    if not os.path.exists(lichess_in):
        os.mkfifo(lichess_in)
    
    # FIFO для отправки ходов в Lichess
    lichess_out = "./build/lichess_out"
    if not os.path.exists(lichess_out):
        os.mkfifo(lichess_out)
    
    bridge = ArduinoBridge()
    if not bridge.connect():
        print("Не удалось подключиться к Arduino по Bluetooth")
        return
    
    try:
        with open(lichess_in, 'r') as fifo_in, open(lichess_out, 'w') as fifo_out:
            while True:
                # 1. Проверяем FIFO от Lichess (ходы противника)
                line = fifo_in.readline().strip()
                if line:
                    if line.startswith("move:"):
                        _, move = line.split(":", 1)
                        from_pos, to_pos = move[:2], move[2:]
                        bridge.highlight_move(from_pos, to_pos)
                    elif line.startswith("fen "):
                        # Обработка начальной позиции (если нужно)
                        pass
                
                # 2. Опрашиваем состояние доски
                board_state = bridge.read_board_state()
                if board_state and board_state != bridge.last_board_state:
                    # Анализируем изменения и формируем ход
                    move = analyze_board_changes(bridge.last_board_state, board_state)
                    if move:
                        fifo_out.write(f"engine:{move}\n")
                        fifo_out.flush()
                    bridge.last_board_state = board_state.copy()
                
                time.sleep(0.1)  # Чтобы не нагружать CPU
    except KeyboardInterrupt:
        print("\nЗавершение работы")
    finally:
        bridge.close()

if __name__ == "__main__":
    print("Starting Arduino Bridge...")
    fifo_communication_loop()