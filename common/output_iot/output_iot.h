//#pragma once
#ifndef OUTPUT_IO_H
#define OUTPUT_IO_H
// C/C++ Headers

// Other Posix headers

// IDF he<der
#include "esp_err.h" //thư viện nhận diện lỗi
#include "hal/gpio_types.h" //thư viện in các tin nhắn debug

// Component header

// Public Headers

// Private Headers

void output_io_creat(gpio_num_t gpio_num);
void output_io_set_level(gpio_num_t gpio_num,int level);
void output_io_toggle(gpio_num_t gpio_num);
void output_io_blink(gpio_num_t gpio_num);
#endif