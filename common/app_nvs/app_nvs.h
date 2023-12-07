//#pragma once
#ifndef __APP_NVS_H
#define __APP_NVS_H
// C/C++ Headers

// Other Posix headers

// IDF he<der
// #include "esp_err.h" //thư viện nhận diện lỗi
// #include "hal/gpio_types.h" //thư viện in các tin nhắn debug
// 
// Component header

// Public Headers

// Private Headers
void app_nvs_set_value(char *key, int *value);
void app_nvs_get_value(char *key, int *value);
void app_nvs_get_str(const char *key, char* out_str);
void app_nvs_set_str(const char *key, const char *str);

#endif