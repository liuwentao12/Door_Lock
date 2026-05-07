#ifndef MY_I2C_H
#define MY_I2C_H

#include "driver/i2c_master.h"
#include "driver/i2c_master.h"
#include "board_config.h"

// 声明一个全局的总线句柄 (通行证)，让其他设备模块都能用到
extern i2c_master_bus_handle_t global_i2c_bus;

// 初始化 I2C 总线
void i2c_bus_init(void);

#endif