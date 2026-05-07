#include "my_i2c.h" 
#include <stdio.h>
#include"esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "indicator.h"   // 引入蜂鸣器联动
#include "fm225.h"

extern volatile int sys_state; 
extern void change_system_state(int new_state);

static i2c_master_dev_handle_t pn532_handle;
//唤醒并配置 PN532 的固定指令 (SAMConfiguration)
static uint8_t sam_config[] = {0x00, 0x00, 0xFF, 0x03, 0xFD, 0xD4, 0x14, 0x01, 0x17, 0x00};
//让模块去寻找卡片的固定指令 (InListPassiveTarget)
static uint8_t read_card_cmd[] = {0x00, 0x00, 0xFF, 0x04, 0xFC, 0xD4, 0x4A, 0x01, 0x00, 0xE1, 0x00};

void pn532_init(void)
{
    i2c_device_config_t dev_cfg={
        .dev_addr_length=I2C_ADDR_BIT_LEN_7,
        .device_address=0x24,
        .scl_speed_hz=100000,
    };
    esp_err_t err=i2c_master_bus_add_device(global_i2c_bus,&dev_cfg,&pn532_handle);
    if (err !=ESP_OK)
    {
        printf("PN532模块挂载失败");
    }
    // 刚上电，向 PN532 发送唤醒指令
    i2c_master_transmit(pn532_handle,sam_config,sizeof(sam_config),-1);
}

void pn532_read_task(void *pvParameters)
{
    // 分身刚启动，先让 PN532 去寻卡
    i2c_master_transmit(pn532_handle, read_card_cmd, sizeof(read_card_cmd), -1);
    
    while (1)
    {
        uint8_t rx_buf[40] = {0};

        // 读取从机数据
        esp_err_t err = i2c_master_receive(pn532_handle, rx_buf, sizeof(rx_buf), -1);
        
        if (err == ESP_OK)
        {
            // 0x01 代表 PN532 读到数据并准备好了
            if (rx_buf[0] == 0x01)
            {
                // 滑动窗口：暴力寻找刷卡成功的固定特征码 D5 4B 01
                for (int i = 0; i < 30; i++) 
                {
                    if (rx_buf[i] == 0xD5 && rx_buf[i+1] == 0x4B && rx_buf[i+2] == 0x01) 
                    {
                        // 特征码往后数第 7 个字节是卡号长度
                        uint8_t uid_len = rx_buf[i+7]; 
                        // 第 8 个字节是真实物理卡号的开头
                        uint8_t *uid = &rx_buf[i+8]; 
                        
                        printf("\n[PN532] 读到卡片 UID: ");
                        for (int j = 0; j < uid_len; j++) {
                            printf("%02X ", uid[j]); 
                        }
                        printf("\n");

                        // ==========================================
                        // 🌟 状态机业务逻辑介入
                        // ==========================================
                        
                        // 只有在“待机状态(0)”下刷卡，系统才理你，防止重复刷卡
                        if (sys_state == 0) 
                        {
                            // 身份 1：你的合法开门卡 (🚨把这里改成你真实的校园卡UID🚨)
                            if (uid_len == 4 && uid[0] == 0x1A && uid[1] == 0x2B && uid[2] == 0x3C && uid[3] == 0x4D) 
                            {
                                ESP_LOGI("PN532", "-> 刷卡成功！准备唤醒人脸...");
                                buzzer_beep(1, 50); // 滴一声
                                
                                change_system_state(1); // 状态机切换到：STATE_WAIT_FACE (等待人脸)
                                fm225_start_verify();   // 向串口发送识别指令，红外灯亮起！
                            }
                            // 身份 2：管理员录入卡 (比如借你舍友的卡，🚨把这里改成他的UID🚨)
                            else if (uid_len == 4 && uid[0] == 0xFF && uid[1] == 0xEE)
                            {
                                ESP_LOGW("PN532", "-> 管理员卡！进入人脸录入模式...");
                                buzzer_beep(2, 80); // 滴滴两声
                                
                                change_system_state(3); // 状态机切换到：STATE_ENROLLING (录入模式)
                                fm225_start_enroll();   // 向串口发录入指令，模块语音提示录入！
                            }
                            // 身份 3：陌生人的卡
                            else 
                            {
                                ESP_LOGE("PN532", "-> 未知卡片！拒绝访问！");
                                buzzer_beep(3, 80); // 滴滴滴急促报警
                                // 状态机什么都不做，依然保持待机
                            }
                        }
                        else 
                        {
                            ESP_LOGI("PN532", "系统正忙(正在认脸或门开着)，忽略本次刷卡...");
                        }
                        // ==========================================

                        // 读完卡后强制休眠 1.5 秒，防止你的手没拿开，它一秒钟读了几十次
                        vTaskDelay(pdMS_TO_TICKS(1500)); 
                        
                        // 【最关键的一步】重新发送寻卡指令，让模块进入下一轮找卡状态！
                        i2c_master_transmit(pn532_handle, read_card_cmd, sizeof(read_card_cmd), -1);
                        
                        break; // 处理完毕，跳出内部的 for 循环
                    }
                }
            }
        }
        
        // 分身歇 100 毫秒，继续下一轮盯梢
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}