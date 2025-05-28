#ifndef CONFIG_H
#define CONFIG_H

// Конфигурация пинов
#define DATA_PIN_OUT  2    // Пин данных для выходного регистра
#define CLOCK_PIN_OUT 3    // Пин такта для выходного регистра
#define LATCH_PIN_OUT 4    // Пин защелки для выходного регистра

#define DATA_PIN_IN   5    // Пин данных для входного регистра
#define CLOCK_PIN_IN  6    // Пин такта для входного регистра
#define LATCH_PIN_IN  7    // Пин защелки для входного регистра

#define LED_PIN       8    // Пин управления светодиодной лентой
#define BT_TX_PIN     9    // Пин TX Bluetooth
#define BT_RX_PIN     10   // Пин RX Bluetooth

// Константы
#define BOARD_SIZE 8       // Размер шахматной доски (8x8)
#define LED_COUNT 64       // Количество светодиодов в ленте

// Цвета светодиодов
#define LED_ON_COLOR 0x00FF00  // Зелёный (формат GRB)
#define LED_OFF_COLOR 0x000000 // Выключен

#endif