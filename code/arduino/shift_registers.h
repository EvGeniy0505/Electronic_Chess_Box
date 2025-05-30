#ifndef SHIFT_REGISTERS_H
#define SHIFT_REGISTERS_H

#include <Arduino.h>
#include <GyverShift.h>
#include "config.h"

class ShiftRegisters {
private:
    GyverShift<OUTPUT, 1> sh595;
    GyverShift<INPUT, 1> sh165;
    
public:
    ShiftRegisters() : 
        sh595(LATCH_PIN_OUT, DATA_PIN_OUT, CLOCK_PIN_OUT),
        sh165(LATCH_PIN_IN, DATA_PIN_IN, CLOCK_PIN_IN) {}

    void init() {
        sh595.update(); // Очищаем выход
    }

    // Установка битов в выходном регистре (столбец)
    void setOutput(byte column) {
        for (byte i = 0; i < 8; i++) {
            sh595[i] = (column & (1 << i)) != 0;
        }
        sh595.update();
    }

    // Чтение входного регистра (строка)
    byte readInput() {
        sh165.update();
        byte result = 0;
        for (byte i = 0; i < 8; i++) {
            if (sh165[i]) {
                result |= (1 << i);
            }
        }
        return result;
    }
};

#endif