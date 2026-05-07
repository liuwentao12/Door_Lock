#include "fm225.h"
#include "board_config.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "indicator.h"   // 引入蜂鸣器联动
#include "lcd1602.h"     // 引入屏幕联动

// 在 fm225.c 收到 result == 0x00 (验证成功) 后：
extern void change_system_state(int new_state);

static const char *TAG = "FM225";
#define FM225_UART_PORT UART_NUM_1

// 魔法数组：录入普通用户人脸 (交互录入，超时时间10秒)
// 包含 32 字节的空名字，全靠最后的校验和 0x3A 撑场面
static uint8_t cmd_enroll[41] = {
    0xEF, 0xAA,             // 帧头
    0x13,                   // MsgID: ENROLL
    0x00, 0x23,             // Size: 35个字节 (十六进制0x23)
    0x00,                   // admin: 0(普通用户)
    // 后面跟 32 个 0x00，代表用户名为空 (模块会自动分配一个数字 ID)
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,                   // face_dir: 0(交互式，也就是需要你扭头的那种)
    0x0A,                   // timeout: 10秒
    0x3A,                    // ParityCheck: 校验和
};

// 触发录入人脸的函数
void fm225_start_enroll(void) {
    uart_write_bytes(FM225_UART_PORT, (const char*)cmd_enroll, sizeof(cmd_enroll));
    ESP_LOGI(TAG, "!!! 已下发录入人脸指令，请盯住摄像头并根据语音提示扭头 !!!");
}


static uint8_t cmd_verify[]={0xEF, 0xAA, 0x12, 0x00, 0x02, 0x00, 0x0A, 0x18};

void fm225_init(void)
{
    uart_config_t uart_config={
        .baud_rate=115200,
        .data_bits=UART_DATA_8_BITS,
        .parity=UART_PARITY_DISABLE,
        .flow_ctrl=UART_HW_FLOWCTRL_DISABLE,
        .source_clk=UART_SCLK_DEFAULT,
    };
    uart_param_config(FM225_UART_PORT,&uart_config);
    uart_set_pin(FM225_UART_PORT, PIN_UART_TX, PIN_UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(FM225_UART_PORT,1024,1024,0,NULL,0);
    ESP_LOGI(TAG, "FM225 串口配置完成！");
}

void fm225_start_verify(void) {
    uart_write_bytes(FM225_UART_PORT, (const char*)cmd_verify, sizeof(cmd_verify));
    ESP_LOGI(TAG, "-> 已向模块发送：开启人脸比对指令！");
}

void fm225_task(void *pvParameters) {
    uint8_t data[256]; 
    
    while (1) {
        // 读取串口，等待 50 毫秒
        int length = uart_read_bytes(FM225_UART_PORT, data, sizeof(data), pdMS_TO_TICKS(50));
        
        if (length >= 9) { // 只有长度大于9，才包含完整的结果信息
            
            // 暴力搜索帧头：0xEF 0xAA 0x00 (回复报文)
            for (int i = 0; i <= length - 9; i++) {
                if (data[i] == 0xEF && data[i+1] == 0xAA && data[i+2] == 0x00) {
                    
                    uint8_t mid = data[i+5];    // 提取指令类型
                    uint8_t result = data[i+6]; // 提取结果码
                    
                    if (mid == 0x12) { // 确认是对 VERIFY (0x12) 指令的回复
                        
                        if (result == 0x00) { // MR_SUCCESS: 验证成功！
                            // 拼合高低八位，算出 User ID
                            uint16_t user_id = (data[i+7] << 8) | data[i+8]; 
                            
                            ESP_LOGI(TAG, "【门锁事件】人脸识别成功！欢迎用户 ID: %d", user_id);
                            
                            // 完美声光与屏幕联动
                            buzzer_beep(1, 100); 
                            lcd1602_clear();
                            lcd1602_print(0, "Face Matched!");
                            lcd1602_print(1, "Welcome Home");
                            change_system_state(2 /* STATE_UNLOCKED */);

                        } else if (result == 0x08) { // MR_FAILED4_UNKNOWNUSER: 陌生人
                            
                            ESP_LOGW(TAG, "【门锁事件】警报！陌生人脸入侵！");
                            buzzer_beep(3, 100); // 滴滴滴报警
                            lcd1602_clear();
                            lcd1602_print(0, "Unknown Face!");
                            lcd1602_print(1, "Access Denied");
                            change_system_state(0 /* STATE_IDLE */);
                        } else if (result == 0x0D) { // MR_FAILED4_TIMEOUT: 超时没人
                            ESP_LOGI(TAG, "识别超时，面前没有看到人脸。");
                            lcd1602_clear();
                            lcd1602_print(0, "Timeout.");
                            change_system_state(0 /* STATE_IDLE */);
                        }
                        
                        // 识别结束后，把屏幕恢复成默认状态
                        vTaskDelay(pdMS_TO_TICKS(2000));
                        lcd1602_clear();
                        lcd1602_print(0, "Please Swipe");
                        lcd1602_print(1, "Or Show Face");
                    }
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20)); // 防止卡死
    }
}

