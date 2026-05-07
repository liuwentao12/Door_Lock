#include "my_i2c.h"

i2c_master_bus_handle_t global_i2c_bus;

void i2c_bus_init(void) {
    // 1. 配置 I2C 主机总线参数
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,   // 默认时钟源
        .i2c_port = -1,                      // 自动分配空闲的 I2C 端口 (通常是0)
        .scl_io_num = PIN_I2C_SCL,           // SCL 引脚
        .sda_io_num = PIN_I2C_SDA,           // SDA 引脚
        .glitch_ignore_cnt = 7,              // 过滤掉硬件上的杂波干扰
        .flags.enable_internal_pullup = true,// 开启内部上拉电阻
    };

    // 2. 创建总线，并把系统发下来的通行证存进 global_i2c_bus 里
    esp_err_t err = i2c_new_master_bus(&i2c_mst_config, &global_i2c_bus);

    if (err != ESP_OK) {
        printf("I2C 初始化失败，错误码：%d\n", err);
    }
}