// my_mqtt.h
#ifndef MY_MQTT_H   
#define MY_MQTT_H

void mqtt_app_start(void);
void mqtt_publish_lock_event(const char *action,const char *user);

#endif
