#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "reed_matrix.h"

class LedControl {
private:
    Adafruit_NeoPixel strip;
    ReedMatrix& reedMatrix;
    
public:
    LedControl(ReedMatrix& rm) : 
        strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800),
        reedMatrix(rm) {}
    
    void init() {
        strip.begin();
        strip.show(); // Инициализация всех светодиодов выключенными
    }
    
    // Обновляет светодиоды в соответствии с состоянием герконов
    void updateFromReedMatrix() {
        for (byte row = 0; row < BOARD_SIZE; row++) {
            for (byte col = 0; col < BOARD_SIZE; col++) {
                byte ledIndex = row * BOARD_SIZE + col;
                if (reedMatrix.isCellActive(row, col)) {
                    strip.setPixelColor(ledIndex, LED_ON_COLOR);
                } else {
                    strip.setPixelColor(ledIndex, LED_OFF_COLOR);
                }
            }
        }
        strip.show();
    }
    
    void clearAll() {
        strip.clear();
        strip.show();
    }
};

#endif