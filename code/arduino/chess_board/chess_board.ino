#include "config.h"
#include "shift_registers.h"
#include "reed_matrix.h"
#include "bluetooth.h"
#include "led_control.h"

ShiftRegisters shiftRegisters;
ReedMatrix reedMatrix(shiftRegisters);
Bluetooth bluetooth;
LedControl ledControl(reedMatrix);

void setup() {
    Serial.begin(9600);
    
    shiftRegisters.init();
    reedMatrix.init();
    bluetooth.init();
    ledControl.init();
    
    Serial.println("Chess Board Initialized");
}

void loop() {
    // 1. Сканируем состояние герконов
    reedMatrix.scan();
    
    // 2. Обновляем светодиоды в соответствии с герконами
    ledControl.updateFromReedMatrix();
    
    // 3. (Пока не используем Bluetooth для управления светодиодами)
    // Можно оставить для будущего расширения
    if (bluetooth.available()) {
        bluetooth.read(); // Просто очищаем буфер
    }
    
    delay(50); // Оптимальная задержка для стабильного чтения
}