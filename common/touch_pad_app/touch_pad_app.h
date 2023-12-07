//#pragma once
#ifndef __TOUCH_PAD_APP_H
#define __TOUCH_PAD_APP_H
// C/C++ Headers

// Other Posix headers

// IDF he<der
#include "esp_err.h" //thư viện nhận diện lỗi
#include "hal/gpio_types.h" //thư viện in các tin nhắn debug

// Component header

// Public Headers

// Private Headers
void touch_pad_app_init();
void touch_pad_app_config(int touch_num);
void touch_pad_app_read(int touch_num,uint16_t *touch_value,uint16_t *touch_filter_value,int *flag);
#endif