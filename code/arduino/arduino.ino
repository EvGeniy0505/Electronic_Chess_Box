#include "config.h"
#include "shift_registers.h"
#include "reed_matrix.h"
#include "bluetooth.h"
#include "led_control.h"  // Добавляем заголовочный файл для управления светодиодами

ShiftRegisters shiftRegisters;
ReedMatrix reedMatrix(shiftRegisters);
Bluetooth bluetooth;
LedControl ledControl(reedMatrix);  // Создаем экземпляр управления светодиодами

// Для отслеживания изменений
byte lastBoardState[BOARD_SIZE] = {0};

void setup() {
    Serial.begin(115200);
    
    shiftRegisters.init();
    reedMatrix.init();
    bluetooth.init();
    ledControl.init();  // Инициализируем светодиоды
    
    Serial.println("Chess Board Initialized");
    Serial.println("Waiting for Bluetooth connection...");
    
    // Первоначальное обновление светодиодов
    ledControl.updateFromReedMatrix();
}

void loop() {
    // 1. Сканируем состояние герконов
    reedMatrix.scan();
    
    // 2. Проверяем изменения
    //if (hasBoardChanged()) {
        // 3. Отправляем только при изменениях
        bluetooth.sendBoardState(reedMatrix.getBoardState());
       
        printMatrixState(reedMatrix.getBoardState());
        
        // Обновляем светодиоды при изменении состояния
        ledControl.updateFromReedMatrix();
    //}
    
    // 4. Проверяем обратную связь
    checkBluetoothFeedback();
    
    delay(50); // Уменьшенная задержка
}

// Остальные функции остаются без изменений

bool hasBoardChanged() {
    byte* current = reedMatrix.getBoardState();
    for (byte i = 0; i < BOARD_SIZE; i++) {
        if (current[i] != lastBoardState[i]) {
            memcpy(lastBoardState, current, BOARD_SIZE);
            return true;
        }
    }
    return false;
}

void printMatrixState(byte* boardState) {
    Serial.println("Matrix state (Debug):");
    for (byte row = 0; row < BOARD_SIZE; row++) {
        for (byte col = 0; col < BOARD_SIZE; col++) {
            Serial.print((boardState[col] & (1 << row)) ? "1 " : "0 ");
        }
        Serial.println();
    }
    Serial.println();
}

void checkBluetoothFeedback() {
    while (bluetooth.btSerial.available()) {
        String feedback = bluetooth.btSerial.readStringUntil('\n');
        feedback.trim();
        
        Serial.print("[BT Feedback] ");  // Метка для отладки
        Serial.println(feedback);  // Вывод всего сообщения
        
        if (feedback.startsWith("ERROR:")) {
            String errorMsg = feedback.substring(6);
            Serial.print("Ошибка хода: ");
            Serial.println(errorMsg);
        }
        else if (feedback.startsWith("MOVE:")) {
            String move = feedback.substring(5);
            Serial.print("Принят ход: ");
            Serial.println(move);
        }
    }
}