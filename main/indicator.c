#include "indicator.h"
#include "board_config.h"      
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

// ADC 句柄 
static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t cali_handle;

void indicator_init(void)
{
    //普通io配置，蜂鸣器和电量指示灯
    gpio_config_t io_config={
        .pin_bit_mask=(1ULL << PIN_BUZZER) | (1ULL << PIN_LED1) | 
                        (1ULL << PIN_LED2) | (1ULL << PIN_LED3) | (1ULL << PIN_LED4),
        .mode=GPIO_MODE_OUTPUT,
        .pull_down_en=0,
        .pull_up_en=0,
        .intr_type=GPIO_INTR_DISABLE
    };
    gpio_config(&io_config);
    // 默认全关
    gpio_set_level(PIN_BUZZER, 0);
    gpio_set_level(PIN_LED1, 0); gpio_set_level(PIN_LED2, 0);
    gpio_set_level(PIN_LED3, 0); gpio_set_level(PIN_LED4, 0);

    //adc电量监测引脚配置
    //初始化ADC1单元
    adc_oneshot_unit_init_cfg_t init_config1={
        .unit_id=ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config1,&adc1_handle);
    // 2. 配置通道 0 (对应 IO1)
    adc_oneshot_chan_cfg_t config={
        .atten=ADC_ATTEN_DB_12,
        .bitwidth=ADC_BITWIDTH_DEFAULT
    };
    adc_oneshot_config_channel(adc1_handle,ADC_CHANNEL_0,&config);
    //配置adc校准
    adc_cali_curve_fitting_config_t cali_config={
        .unit_id=ADC_UNIT_1,
        .chan=ADC_CHANNEL_1,
        .atten=ADC_ATTEN_DB_12,
        .bitwidth=ADC_BITWIDTH_DEFAULT,
    };
    adc_cali_create_scheme_curve_fitting(&cali_config,&cali_handle);
}

static int get_battery_voltage_mv(void)
{
    int raw_value=0;
    int voltage_mv=0;
    adc_oneshot_read(adc1_handle,ADC_CHANNEL_0,&raw_value);
    adc_cali_raw_to_voltage(cali_handle,raw_value,&voltage_mv);
    int real_battery_mv=voltage_mv*2;
    return real_battery_mv;
}

static void update_battery_led(void)
{
    // 1. 获取电池电压（单位：mV）
    int mv = get_battery_voltage_mv();
    int level;

    // 2. 判断电量等级（单节锂电池：4.2V满电，3.3V没电）
    if (mv >= 4000)      level = 4;   // 4个灯全亮
    else if (mv >= 3800) level = 3;   // 亮3个
    else if (mv >= 3600) level = 2;   // 亮2个
    else if (mv >= 3400) level = 1;   // 亮1个
    else                 level = 0;   // 全灭

    // 3. 根据等级点亮LED
    gpio_set_level(PIN_LED1, level >= 1 ? 1 : 0);
    gpio_set_level(PIN_LED2, level >= 2 ? 1 : 0);
    gpio_set_level(PIN_LED3, level >= 3 ? 1 : 0);
    gpio_set_level(PIN_LED4, level >= 4 ? 1 : 0);
}

void battery_task(void *arg)
{
    while(1)
    {
        update_battery_led();  // 一次调用，全部搞定
        vTaskDelay(pdMS_TO_TICKS(3000)); // 3秒刷新一次
    }
}

void buzzer_beep(int times,uint32_t duration_ms)
{
    for (int i = 0; i < times; i++)
    {
        gpio_set_level(PIN_BUZZER,1);
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        gpio_set_level(PIN_BUZZER,0);
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
    }
    
}
