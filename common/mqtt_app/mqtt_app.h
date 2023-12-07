//#pragma once
#ifndef __MQTT_APP_H
#define __MQTT_APP_H
// C/C++ Headers
ESP_EVENT_DECLARE_BASE(EVENT_DEV_BASE);

enum event_dev_id {
    MQTT_DEV_EVENT_CONNECTED,
    MQTT_DEV_EVENT_DISCONNECT,
    MQTT_DEV_EVENT_DATA,
    MQTT_DEV_EVENT_SUBSCRIBED
};
// Other Posix headers

typedef void (*ota_data_callback_t) (char *topic,int leng,char* data);
// IDF he<der
// Component header

// Public Headers

// Private Headers
void mqtt_app_init(void);
void mqtt_data_set_callback(void *cb);
void app_mqtt_publish(char *topic, char *data);
void app_mqtt_subscribe(char *topic);
#endif