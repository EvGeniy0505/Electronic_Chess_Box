#ifndef CONFIG_H
#define CONFIG_H

// Пины 74HC595 (выходные - столбцы)
#define LATCH_PIN_OUT 6     // CS_595 (Latch)
#define DATA_PIN_OUT 5       // DAT_595
#define CLOCK_PIN_OUT 7      // CLK_595

// Пины 74HC165 (входные - строки)
#define LATCH_PIN_IN 3       // LAT_165 (Load)
#define DATA_PIN_IN 4        // DAT_165
#define CLOCK_PIN_IN 2       // CLK_165

// Bluetooth пины
#define BT_TX_PIN 8          // Пин TX Bluetooth
#define BT_RX_PIN 9         // Пин RX Bluetooth

// Константы
#define BOARD_SIZE 8         // Размер шахматной доски (8x8)

// LED Configuration
#define LED_PIN 11           // Пин, к которому подключена лента
#define LED_COUNT 64        // 8x8 = 64 светодиода
#define LED_ON_COLOR strip.Color(0, 255, 0)  // Зеленый цвет для занятых клеток
#define LED_OFF_COLOR strip.Color(0, 0, 0)   // Выключенный светодиод

#endif