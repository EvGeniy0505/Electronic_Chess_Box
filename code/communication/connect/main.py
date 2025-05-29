def process_chessboard(board_bytes, output_file="chessboard.txt"):
    """
    Обрабатывает шахматную доску, представленную 8 байтами,
    и записывает координаты занятых клеток в файл
    
    :param board_bytes: 8 байт, каждый представляет строку доски
    :param output_file: имя файла для записи результата
    """
    if len(board_bytes) != 8:
        raise ValueError("Должно быть ровно 8 байт для шахматной доски 8x8")
    
    occupied_cells = []
    
    for row in range(8):
        byte = board_bytes[row]
        for col in range(8):
            # Проверяем бит (от старшего к младшему)
            if byte & (1 << (7 - col)):
                # Преобразуем координаты в шахматную нотацию
                letter = chr(ord('a') + col)
                number = 8 - row
                occupied_cells.append(f"{letter}{number}")
    
    # Записываем в файл
    with open(output_file, 'w') as f:
        if occupied_cells:
            f.write("\n".join(occupied_cells) + "\n")
            f.write(f"Всего занято: {len(occupied_cells)} клеток\n")
        else:
            f.write("На доске нет фигур\n")
    
    print(f"Результат записан в {output_file}")

# Пример использования
if __name__ == "__main__":
    # Пример входных данных (8 байт)
    # Начальная позиция в шахматах:
    input_bytes = [
        0b11111111,  # Белые пешки (ряд 2)
        0b11100111,  # Белые фигуры (ряд 1)
        0b00000000,  # Пустые ряды
        0b00000000,
        0b00000000,
        0b00000000,
        0b11111111,  # Чёрные пешки (ряд 7)
        0b11100111   # Чёрные фигуры (ряд 8)
    ]
    
    # Конвертируем числа в байты
    board_bytes = bytes(input_bytes)
    
    # Обрабатываем и записываем в файл
    process_chessboard(board_bytes)