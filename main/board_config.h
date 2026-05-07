#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// ==========================================
// 智能门锁全局硬件引脚映射表 (Pin Map)
// ==========================================

// --- I2C 总线 (PN532刷卡模块 & LCD1602屏幕) ---
#define PIN_I2C_SCL         4
#define PIN_I2C_SDA         5

// --- 门锁控制与提示 ---
#define PIN_RELAY_EN        7    // 继电器控制引脚 (高电平开门/低电平开门取决于硬件)
#define PIN_BUZZER          15   // 蜂鸣器引脚

// --- UART 串口 (FM225 人脸识别模块) ---
#define PIN_UART_TX         16
#define PIN_UART_RX         17

// --- SPI 总线 (W25Q64 外部Flash) ---
#define PIN_SPI_CS          10
#define PIN_SPI_MOSI        11
#define PIN_SPI_CLK         12
#define PIN_SPI_MISO        13

// --- 传感器与电源监测 ---
#define PIN_BAT_ADC         1    // 电池电压检测 (ADC1_CH0)
#define PIN_DOOR_SENSOR     2    // 门磁传感器

// --- 状态指示灯 LED ---
#define PIN_LED4            38
#define PIN_LED3            39
#define PIN_LED2            40
#define PIN_LED1            41

#endif // BOARD_CONFIG_H