#ifndef SHIFT_REGISTERS_H
#define SHIFT_REGISTERS_H

#include <Arduino.h>
#include "config.h"

class ShiftRegisters {
public:
    void init() {
        pinMode(DATA_PIN_OUT, OUTPUT);
        pinMode(CLOCK_PIN_OUT, OUTPUT);
        pinMode(LATCH_PIN_OUT, OUTPUT);
        
        pinMode(DATA_PIN_IN, INPUT);
        pinMode(CLOCK_PIN_IN, OUTPUT);
        pinMode(LATCH_PIN_IN, OUTPUT);
    }

    // Установка битов в выходном регистре
    void setOutput(byte row) {
        digitalWrite(LATCH_PIN_OUT, LOW);
        shiftOut(DATA_PIN_OUT, CLOCK_PIN_OUT, LSBFIRST, row);
        digitalWrite(LATCH_PIN_OUT, HIGH);
    }

    // Чтение входного регистра
    byte readInput() {
        digitalWrite(LATCH_PIN_IN, LOW);
        digitalWrite(LATCH_PIN_IN, HIGH);
        return shiftIn(DATA_PIN_IN, CLOCK_PIN_IN, LSBFIRST);
    }
};

#endif