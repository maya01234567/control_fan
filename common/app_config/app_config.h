//#pragma once
#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H
// C/C++ Headers

// Other Posix headers

// IDF he<der
#include "esp_err.h" //thư viện nhận diện lỗi
#include "hal/gpio_types.h" //thư viện in các tin nhắn debug
#include "esp_wifi.h"
typedef enum
{
    PROVISITION_SMARTCONFIG = 0,
    PROVISITION_ACCESSPOIN = 1
} provision_type_t;
typedef enum
{
    NORMAL,
    SELECTTION_MODE_CONNECT,
    CONNECT_WIFI,
    ERASE_FLASH,
    CONNECT_ERROR
} status_connect_wifi_t;
typedef struct 
{
    wifi_config_t wifi_config;
    status_connect_wifi_t status_connect;
    provision_type_t provision_type;
    uint8_t sta;
    uint8_t block_init;
}info_config_wifi_t;


// Component header

// Public Headers

// Private Headers
void handle_connect_wifi(void);
void app_config_init();

#endif