#ifndef REED_MATRIX_H
#define REED_MATRIX_H

#include <Arduino.h>
#include "config.h"
#include "shift_registers.h"

class ReedMatrix {
private:
    ShiftRegisters& shiftRegisters;
    byte boardState[BOARD_SIZE]; // Каждый бит представляет состояние клетки
    
public:
    ReedMatrix(ShiftRegisters& sr) : shiftRegisters(sr) {}
    
    void init() {
        memset(boardState, 0, sizeof(boardState)); // Очищаем состояние
    }
    
    // Сканирует всю матрицу и возвращает массив состояний
    void scan() {
        for (byte row = 0; row < BOARD_SIZE; row++) {
            // Активируем текущую строку
            shiftRegisters.setOutput(1 << row);
            
            // Читаем состояние столбцов
            boardState[row] = shiftRegisters.readInput();
            
            // Небольшая задержка для стабилизации
            delayMicroseconds(50);
        }
    }
    
    // Проверяет состояние конкретной клетки
    bool isCellActive(byte row, byte col) {
        if (row >= BOARD_SIZE || col >= BOARD_SIZE) return false;
        return (boardState[row] & (1 << col)) != 0;
    }
    
    // Возвращает указатель на массив состояний
    byte* getBoardState() {
        return boardState;
    }
};

#endif