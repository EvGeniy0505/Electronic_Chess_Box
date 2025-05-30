import os
import subprocess
import time
import serial
from collections import defaultdict, deque

BOARD_SIZE = 8
SAMPLE_SIZE = 5
STABLE_THRESHOLD = 3
FIFO_IN_PATH = "./build/arduino_in"
FIFO_OUT_PATH = "./build/arduino_out"

class ChessBoardAnalyzer:
    def __init__(self, serial_conn, fifo_path="/tmp/arduino_out"):
        self.prev_stable_state = None
        self.intermediate_state = None
        self.ser = serial_conn
        self.processing = False
        self.move_timeout = 0
        self.sample_buffer = deque(maxlen=SAMPLE_SIZE)
        self.stable_counter = 0
        self.empty_board = '0' * (BOARD_SIZE * BOARD_SIZE)  # Состояние пустой доски
        
        try:
            self.fifo = open(fifo_path, "w")
            print(f"Connected to FIFO at {fifo_path}")
        except Exception as e:
            print(f"Failed to open FIFO: {e}")
            self.fifo = None

    def get_most_common_state(self):
        if not self.sample_buffer:
            return None
            
        state_counts = defaultdict(int)
        for state in self.sample_buffer:
            state_counts[state] += 1
            
        return max(state_counts.items(), key=lambda x: x[1])[0]

    def is_state_stable(self, new_state):
        if len(self.sample_buffer) < self.sample_buffer.maxlen:
            return False
            
        most_common = self.get_most_common_state()
        if most_common == new_state:
            self.stable_counter += 1
        else:
            self.stable_counter = 0
            
        return self.stable_counter >= STABLE_THRESHOLD

    def is_empty_board(self, state):
        """Проверяет, является ли доска пустой"""
        return state == self.empty_board

    def send_to_cpp(self, move):
        if self.fifo:
            try:
                self.fifo.write(f"{move}\n")
                self.fifo.flush()
                print(f"Sent to C++: {move}")
            except Exception as e:
                print(f"Error writing to FIFO: {e}")

    def send_error(self, message):
        clean_message = ''.join(c for c in message if ord(c) < 128)
        try:
            self.ser.write(f"ERROR:{clean_message}\n".encode('ascii'))
            print(f"Sent error: {clean_message}")
        except Exception as e:
            print(f"Failed to send error to serial: {e}")

    @staticmethod
    def square_to_notation(row, col):
        return chr(ord('a') + col) + str(BOARD_SIZE - row)

    def is_valid_move(self, from_sq, to_sq):
        try:
            if not (isinstance(from_sq, (tuple, list))) or not (isinstance(to_sq, (tuple, list))):
                raise ValueError("Coordinates must be tuple/list")
                
            from_row, from_col = map(int, from_sq)
            to_row, to_col = map(int, to_sq)
            
            board_range = range(BOARD_SIZE)
            if (from_row not in board_range or from_col not in board_range or
                to_row not in board_range or to_col not in board_range):
                self.send_error("Coordinates out of board")
                return False
                
            if (from_row, from_col) == (to_row, to_col):
                self.send_error("Piece didn't move")
                return False
                
            return True
        
        except (ValueError, TypeError) as e:
            self.send_error(f"Move validation error: {str(e)}")
            return False

    def analyze_move(self, old_state, new_state):
        """Analyze board changes and validate moves"""
        # Если доска пустая, не анализируем ходы
        if self.is_empty_board(new_state):
            self.intermediate_state = None
            return None
            
        changes = []
        
        for row in range(BOARD_SIZE):
            for col in range(BOARD_SIZE):
                idx = row*BOARD_SIZE + col
                if old_state[idx] != new_state[idx]:
                    changes.append((row, col, old_state[idx], new_state[idx]))
        
        # Если нет изменений, выходим
        if not changes:
            return None
        
        # Поднятие фигуры
        if len(changes) == 1:
            (r, c, old, new) = changes[0]
            
            if old == '1' and new == '0':
                self.intermediate_state = new_state
                self.move_timeout = time.time() + 2.0
                print("Piece lifted, waiting for placement...")
                return None
                
            elif old == '0' and new == '1' and self.intermediate_state is not None:
                from_sq = None
                for row in range(BOARD_SIZE):
                    for col in range(BOARD_SIZE):
                        if (self.intermediate_state[row*BOARD_SIZE + col] == '0' and 
                            old_state[row*BOARD_SIZE + col] == '1'):
                            from_sq = (row, col)
                            break
                    if from_sq is not None:
                        break
                
                if from_sq is not None:
                    to_sq = (r, c)
                    if self.is_valid_move(from_sq, to_sq):
                        move = self.square_to_notation(*from_sq) + self.square_to_notation(*to_sq)
                        self.intermediate_state = None
                        return move
                return None
        
        # Полный ход
        elif len(changes) == 2:
            (r1, c1, old1, new1), (r2, c2, old2, new2) = changes
            
            if old1 == '1' and new1 == '0' and old2 == '0' and new2 == '1':
                from_sq = (r1, c1)
                to_sq = (r2, c2)
                if self.is_valid_move(from_sq, to_sq):
                    self.intermediate_state = None
                    return self.square_to_notation(r1, c1) + self.square_to_notation(r2, c2)
                return None
                    
            elif old1 == '0' and new1 == '1' and old2 == '1' and new2 == '0':
                from_sq = (r2, c2)
                to_sq = (r1, c1)
                if self.is_valid_move(from_sq, to_sq):
                    self.intermediate_state = None
                    return self.square_to_notation(r2, c2) + self.square_to_notation(r1, c1)
                return None
        
        # Рокировка
        elif len(changes) == 4:
            moved = [(r,c) for r,c,old,new in changes if old == '1' and new == '0']
            arrived = [(r,c) for r,c,old,new in changes if old == '0' and new == '1']
            
            if len(moved) == 2 and len(arrived) == 2:
                if {moved[0][1], moved[1][1], arrived[0][1], arrived[1][1]} == {0, 2, 3, 4}:
                    self.intermediate_state = None
                    return "O-O-O"
                elif {moved[0][1], moved[1][1], arrived[0][1], arrived[1][1]} == {4, 5, 6, 7}:
                    self.intermediate_state = None
                    return "O-O"
        
        # Взятие
        elif len(changes) == 3:
            disappeared = [(r,c) for r,c,old,new in changes if old == '1' and new == '0']
            appeared = [(r,c) for r,c,old,new in changes if old == '0' and new == '1']
            
            if len(disappeared) == 2 and len(appeared) == 1:
                from_sq = next((sq for sq in disappeared if not any(sq == a for a in appeared))), None
                to_sq = appeared[0]
                
                if from_sq and self.is_valid_move(from_sq, to_sq):
                    self.intermediate_state = None
                    return self.square_to_notation(*from_sq) + self.square_to_notation(*to_sq)
        
        # Сброс при таймауте
        if self.intermediate_state is not None and time.time() > self.move_timeout:
            print("Move timeout, resetting intermediate state")
            self.intermediate_state = None
            
        # Не отправляем ошибку для пустой доски или когда нет изменений
        if not self.is_empty_board(new_state) and changes:
            self.send_error(f"Unrecognized move pattern")
        return None

    def process_state(self, state):
        if self.processing:
            return

        self.sample_buffer.append(state)
        
        if len(self.sample_buffer) < self.sample_buffer.maxlen:
            return
            
        current_state = self.get_most_common_state()
        
        if not self.is_state_stable(current_state):
            return

        if self.prev_stable_state is None:
            self.prev_stable_state = current_state
            self.stable_counter = 0
            return

        self.processing = True

        old_state = self.intermediate_state if self.intermediate_state is not None else self.prev_stable_state
        move = self.analyze_move(old_state, current_state)

        if move is not None:
            print(f"\nMove made: {move}")
            self.send_to_cpp(move)
            try:
                self.ser.write(f"MOVE:{move}\n".encode('utf-8'))
            except Exception as e:
                print(f"Failed to send move to serial: {e}")
            self.prev_stable_state = current_state
            self.stable_counter = 0

        self.processing = False

def read_chessboard():
    ser = serial.Serial(port='/dev/rfcomm0', baudrate=9600, timeout=1)
    analyzer = ChessBoardAnalyzer(ser)

    print("Connecting to chess board. Waiting for moves...")

    try:
        while True:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8').strip()
                if len(line) == BOARD_SIZE * BOARD_SIZE and all(c in '01' for c in line):
                    analyzer.process_state(line)
                else:
                    print(f"Skipping invalid data: {line}")
    except KeyboardInterrupt:
        print("\nShutting down...")
    finally:
        ser.close()
        print("Connection closed")

if __name__ == "__main__":
    read_chessboard()