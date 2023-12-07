//#pragma once
#ifndef _LEDC_APP_H
#define _LEDC_APP_H
// C/C++ Headers

// Other Posix headers

// IDF he<der
#include "esp_err.h" //thư viện nhận diện lỗi
#include "hal/gpio_types.h"
#include "hal/ledc_types.h" //thư viện in các tin nhắn debug

// Component header

// Public Headers

// Private Headers
void ledc_init(ledc_timer_bit_t resolution, uint32_t freq_hz, ledc_mode_t speed_mode, ledc_timer_t timer);
void add_pin_ledc(ledc_channel_t chanel, gpio_num_t gpio_num, int hpoint,ledc_mode_t speed_mode, ledc_timer_t timer);
void set_duty_ledc(int duty,ledc_mode_t speed_mode,ledc_channel_t chanel);
#endif