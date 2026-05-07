#include "lcd1602.h"
#include "my_i2c.h" 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static i2c_master_dev_handle_t lcd_handle;
static uint8_t backlight_state = 0x08; // 默认开启背光

// 底层：向 I2C 发送 1 个字节
static void lcd_send_raw(uint8_t data) {
    i2c_master_transmit(lcd_handle, &data, 1, -1);
}

// 底层：制造一个下降沿脉冲 (让屏幕读取数据)
static void lcd_pulse_enable(uint8_t data) {
    lcd_send_raw(data | 0x04); // E=1
    vTaskDelay(pdMS_TO_TICKS(1)); 
    lcd_send_raw(data & ~0x04); // E=0
    vTaskDelay(pdMS_TO_TICKS(1));
}

// 发送 4 位数据
static void lcd_send_nibble(uint8_t data_4bit, uint8_t rs) {
    uint8_t data = (data_4bit & 0xF0) | backlight_state | (rs ? 0x01 : 0x00);
    lcd_send_raw(data);
    lcd_pulse_enable(data);
}

// 发送 8 位命令或数据
static void lcd_send_byte(uint8_t data, uint8_t rs) {
    lcd_send_nibble(data & 0xF0, rs);          // 发高 4 位
    lcd_send_nibble((data << 4) & 0xF0, rs);   // 发低 4 位
}

void lcd1602_init(void) {
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x27, // 【注意】大部分 1602 I2C 地址是 0x27，如果点不亮可能是 0x3F
        .scl_speed_hz = 100000,
    };
    i2c_master_bus_add_device(global_i2c_bus, &dev_cfg, &lcd_handle);

    // LCD1602 标准初始化时序
    vTaskDelay(pdMS_TO_TICKS(50));
    lcd_send_nibble(0x30, 0); vTaskDelay(pdMS_TO_TICKS(5));
    lcd_send_nibble(0x30, 0); vTaskDelay(pdMS_TO_TICKS(1));
    lcd_send_nibble(0x30, 0); vTaskDelay(pdMS_TO_TICKS(1));
    lcd_send_nibble(0x20, 0); vTaskDelay(pdMS_TO_TICKS(1)); // 切换到 4-bit 模式

    lcd_send_byte(0x28, 0); // 4位数据线，2行显示，5x8点阵
    lcd_send_byte(0x0C, 0); // 开显示，关光标，关闪烁
    lcd_send_byte(0x01, 0); // 清屏
    vTaskDelay(pdMS_TO_TICKS(5));
}

void lcd1602_clear(void) {
    lcd_send_byte(0x01, 0); // 发送清屏指令
    vTaskDelay(pdMS_TO_TICKS(5));
}

void lcd1602_print(int line, const char* str) {
    // line: 0 是第一行，1 是第二行
    uint8_t addr = (line == 0) ? 0x80 : 0xC0;
    lcd_send_byte(addr, 0); // 设置显示位置

    // 逐个发送字符
    while (*str) {
        lcd_send_byte(*str++, 1);
    }
}