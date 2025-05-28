#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <SoftwareSerial.h>
#include "config.h"

class Bluetooth {
private:
    SoftwareSerial btSerial;
    
public:
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
        for (byte row = 0; row < BOARD_SIZE; row++) {
            btSerial.write(boardState[row]);
        }
    }
};

#endif