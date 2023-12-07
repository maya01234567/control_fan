//#pragma once
#ifndef __WS2812B_IO_H
#define __WS2812B_IO_H
// C/C++ Headers

// Other Posix headers

// IDF he<der
#include "esp_err.h" //thư viện nhận diện lỗi
#include "hal/gpio_types.h"//thư viện in các tin nhắn debug
// Component header

// Public Headers

// Private Headers

void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b);
void ws2812b_init(int tx_pin, int number_led);
void set_color_ws(int index,int r,int g,int b);
void update_color_ws(void);
void clear_color_ws(void);

#endif