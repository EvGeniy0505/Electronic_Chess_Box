import unittest
from unittest.mock import MagicMock, patch
from arduino_bridge import ArduinoBridge, analyze_board_changes
import serial

class TestArduinoBridge(unittest.TestCase):
    # @patch('serial.Serial')
    # def test_connection(self, mock_serial):
    #     """Тестирование подключения к Arduino"""
    #     mock_serial.return_value = MagicMock()
    #     bridge = ArduinoBridge()
    #     self.assertTrue(bridge.connect("/dev/ttyACM0"))
    #     mock_serial.assert_called_with("/dev/ttyACM0", baudrate=9600, timeout=1)

    # @patch.object(serial.Serial, 'read')
    # def test_read_board_state(self, mock_read):
    #     """Тестирование чтения состояния доски"""
    #     mock_read.return_value = bytes([0b00010000, 0, 0, 0, 0, 0, 0, 0])  # Пешка на a2
    #     bridge = ArduinoBridge()
    #     bridge.serial = MagicMock()
    #     state = bridge.read_board_state()
    #     self.assertEqual(state, [16, 0, 0, 0, 0, 0, 0, 0])

    def test_analyze_board_changes(self):
        """Тестирование анализа изменений на доске"""
        # Стандартный ход пешки
        old = [0, 0b00010000, 0, 0, 0, 0, 0, 0]  # Пешка на a2
        new = [0, 0, 0b00010000, 0, 0, 0, 0, 0]  # Пешка на a3
        self.assertEqual(analyze_board_changes(old, new), "a2a3")

        # Взятие фигуры
        old = [0b00010000, 0, 0, 0b01000000, 0, 0, 0, 0]  # Пешка на a1, слон на g1
        new = [0b00000000, 0, 0, 0b00010000, 0, 0, 0, 0]  # Пешка взяла слона на g1
        self.assertEqual(analyze_board_changes(old, new), "a1g1")

if __name__ == "__main__":
    unittest.main()