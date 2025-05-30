#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <SoftwareSerial.h>
#include "config.h"

class Bluetooth {
private:
    
public:
    SoftwareSerial btSerial;

    Bluetooth() : btSerial(BT_RX_PIN, BT_TX_PIN) {}
    
    void init() {
        btSerial.begin(9600);
    }
    
    bool available() {
        return btSerial.available();
    }
    
    int read() {
        return btSerial.read();
    }
    
    void sendBoardState(byte* boardState) {
        // Отправляем состояние доски одной строкой из 0 и 1
        for (byte row = 0; row < BOARD_SIZE; row++) {
            for (byte col = 0; col < BOARD_SIZE; col++) {
                bool cellState = (boardState[col] & (1 << row)) != 0;
                btSerial.print(cellState ? "1" : "0");
            }
        }
        btSerial.println(); // Переводим строку в конце
    }
};

#endif