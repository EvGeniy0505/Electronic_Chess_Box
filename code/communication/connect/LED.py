import serial
import time

def read_chessboard_file(filename="chessboard.txt"):
    """Читает файл с шахматными координатами"""
    occupied = []
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if len(line) == 2 and line[0] in 'abcdefgh' and line[1] in '12345678':
                occupied.append(line)
    return occupied

def create_byte_board(occupied_cells):
    """Создаёт 8-байтовое представление доски"""
    board = [0b00000000 for _ in range(8)]
    
    for cell in occupied_cells:
        col = ord(cell[0]) - ord('a')  # a-h -> 0-7
        row = 8 - int(cell[1])         # 1-8 -> 0-7
        board[row] |= (1 << (7 - col))
    
    return bytes(board)

def send_to_arduino(port='/dev/ttyACM0', baudrate=115200):
    """Отправляет данные на Arduino"""
    occupied = read_chessboard_file()
    byte_board = create_byte_board(occupied)
    
    try:
        with serial.Serial(port, baudrate, timeout=1) as ser:
            time.sleep(2)  # Ожидание инициализации Arduino
            ser.write(byte_board)
            print(f"Отправлено на Arduino: {byte_board}")
            
            # Чтение ответа от Arduino
            while True:
                if ser.in_waiting:
                    line = ser.readline().decode().strip()
                    print(line)
                    if "Доска обновлена" in line:
                        break
    except Exception as e:
        print(f"Ошибка: {e}")

if __name__ == "__main__":
    send_to_arduino()