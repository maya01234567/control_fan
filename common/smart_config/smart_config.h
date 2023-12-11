//#pragma once
#ifndef __SMART_CONFIG_H
#define __SMART_CONFIG_H
// C/C++ Headers

// Other Posix headers

// IDF he<der
#include "esp_err.h" //thư viện nhận diện lỗi
#include "hal/gpio_types.h" //thư viện in các tin nhắn debug
#include "esp_wifi.h"

// Component header
typedef void (*info_wifi_t)(wifi_config_t* _wifi_config);

// Public Headers

// Private Headers

void smart_config_connect(wifi_config_t* _wifi_config);
void set_callback_infowifi_smart(void *cb);

#endif