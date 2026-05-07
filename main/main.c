#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "indicator.h"
#include "my_i2c.h"      
#include "pn532.h" 
#include "lcd1602.h"
#include "esp_log.h"
#include "my_mqtt.h"
#include "wifi_station.h"

#define SSID "oo"
#define PASS "lwtnb666"


// 1. 门锁系统的四大状态
typedef enum {
    STATE_IDLE = 0,         // 待机状态：睡觉，等刷卡
    STATE_WAIT_FACE,        // 刷卡通过了：正在等待人脸验证
    STATE_UNLOCKED,         // 门开了：倒计时关门
    STATE_ENROLLING         // 录入模式：管理员刷卡后进入
} system_state_t;
// 2. 全局状态变量（门锁的大脑）
volatile system_state_t sys_state = STATE_IDLE;
// 3. 给其他模块调用的接口，用来改变状态
void change_system_state(system_state_t new_state) {
    sys_state = new_state;
}
void fsm_controller_task(void *pvParameters);

static const char *TAG = "SMART_LOCK";
// 全局定义任务句柄
TaskHandle_t xBatteryTaskHandle = NULL;
TaskHandle_t xpn532TaskHandle = NULL;
TaskHandle_t xfsmcontrollerHandle = NULL;

void app_main(void)
{
    wifi_init_and_connect(SSID,PASS);
    indicator_init();
    i2c_bus_init();
    // 初始化外设
    pn532_init();
    lcd1602_init();
    // 屏幕开机动画
    lcd1602_print(0, "Smart Lock v1.0");
    lcd1602_print(1, "System Starting.");
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    lcd1602_clear();
    lcd1602_print(0, "Please Swipe");
    lcd1602_print(1, "Your Card...");

    xTaskCreate(pn532_read_task, "pn532_task", 4096, NULL, 5,&xpn532TaskHandle);
    xTaskCreate(battery_task,"battery_task",2048,NULL,5,&xBatteryTaskHandle);
    xTaskCreate(fsm_controller_task, "fsm_task", 4096, NULL, 5,&xfsmcontrollerHandle);
}

void fsm_controller_task(void *pvParameters) {
    system_state_t last_state = -1; // 记录上一次的状态，用来检测状态变化
    int unlock_timer = 0;           // 开门倒计时
    
    while(1) {
        // 当状态发生“切换”的那一瞬间，更新屏幕
        if (sys_state != last_state) {
            lcd1602_clear();
            switch (sys_state) {
                case STATE_IDLE:
                    ESP_LOGI(TAG, "系统进入 -> 待机模式");
                    lcd1602_print(0, "Please Swipe");
                    lcd1602_print(1, "Your Card...");
                    break;
                case STATE_WAIT_FACE:
                    ESP_LOGI(TAG, "系统进入 -> 人脸验证模式");
                    lcd1602_print(0, "Card OK!");
                    lcd1602_print(1, "Verifying Face..");
                    break;
                case STATE_UNLOCKED:
                    ESP_LOGI(TAG, "系统进入 -> 已开锁模式");
                    lcd1602_print(0, "Face Matched!");
                    lcd1602_print(1, "Door Unlocked!");
                    buzzer_beep(1, 200);
                    // TODO: 在这里给继电器通电开门
                    unlock_timer = 50; // 设置 50 个周期 (5秒) 后自动关门
                    break;
                case STATE_ENROLLING:
                    ESP_LOGI(TAG, "系统进入 -> 录入人脸模式");
                    lcd1602_print(0, "Admin Mode");
                    lcd1602_print(1, "Look at Camera");
                    break;
            }
            last_state = sys_state;
        }

        // 持续性动作：处理开门倒计时
        if (sys_state == STATE_UNLOCKED) {
            unlock_timer--;
            if (unlock_timer <= 0) {
                ESP_LOGI(TAG, "时间到，自动关门！");
                // TODO: 继电器断电
                buzzer_beep(2, 50); // 滴滴两声提示锁门
                change_system_state(STATE_IDLE); // 自动切回待机！
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); // 每 100ms 巡视一次状态
    }
}