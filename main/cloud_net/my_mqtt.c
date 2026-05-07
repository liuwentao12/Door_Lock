#include "my_mqtt.h"
#include "esp_log.h"
#include "cJSON.h"
#include "mqtt_client.h"

static const char *TAG = "MQTT_DEPT"; 
static esp_mqtt_client_handle_t client =NULL;

static void mqtt_event_handler(void *handler_args,esp_event_base_t base,int32_t event_id,void *event_data)
{
    esp_mqtt_event_handle_t event=event_data;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG,"连接成功");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG,"连接失败");
    break;
    default:
        break;
    }
}

void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_Cfg={
        .broker.address.uri="mqtt://server.oonb.top:1883",
    };

    client =esp_mqtt_client_init(&mqtt_Cfg);
    esp_mqtt_client_register_event(client,ESP_EVENT_ANY_ID,mqtt_event_handler,NULL);
    esp_mqtt_client_start(client);
}

void mqtt_publish_lock_event(const char *action,const char *user)
{
    if (client==NULL)return;
    cJSON *root=cJSON_CreateObject();
    cJSON_AddStringToObject(root,"device_id","Door_lock");
    cJSON_AddStringToObject(root,"action",action);
    cJSON_AddStringToObject(root,"user",user);

    char *json_sting = cJSON_PrintUnformatted(root);
    
    esp_mqtt_client_publish(client,"door/event",json_sting,0,1,0);

    ESP_LOGI(TAG,"发送：%s",json_sting);

    cJSON_Delete(root);
    free(json_sting);
}