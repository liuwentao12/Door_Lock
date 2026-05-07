#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

static const char *TAG ="WIFI_STATION";

static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

static void event_handler(void *arg,esp_event_base_t event_base,int32_t event_id,void *event_data)
{
    if (event_base==WIFI_EVENT && event_id==WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG,"开始尝试连接路由器");
        esp_wifi_connect();
    }
    else if(event_base==WIFI_EVENT && event_id==WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGW(TAG, "连接失败！尝试重新连接...");
        esp_wifi_connect();
    }
    else if(event_base==IP_EVENT && event_id==IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event=(ip_event_got_ip_t*)event_data;
        ESP_LOGI(TAG, "拿到了IP地址: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_and_connect(const char* ssid, const char* password)
{
    ESP_LOGI(TAG,"开始连接并且初始化WIFI: %s",ssid);

    wifi_event_group=xEventGroupCreate();

    esp_err_t ret=nvs_flash_init();
    if (ret==ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret=nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg=WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,ESP_EVENT_ANY_ID,&event_handler,NULL,NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,IP_EVENT_STA_GOT_IP,&event_handler,NULL,NULL));

    wifi_config_t wifi_config={0};
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    xEventGroupWaitBits(wifi_event_group,WIFI_CONNECTED_BIT,pdFALSE,pdFALSE,portMAX_DELAY);
}