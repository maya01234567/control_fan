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

// Component header

// Public Headers

// Private Headers
void handle_connect_wifi(void);

#endif