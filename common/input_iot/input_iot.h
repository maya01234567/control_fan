//#pragma once
#ifndef INPUT_IO_H
#define INPUT_IO_H
// C/C++ Headers

// Other Posix headers

// IDF he<der
#include "esp_err.h" //thư viện nhận diện lỗi
#include "hal/gpio_types.h" //thư viện in các tin nhắn debug

// Component header

// Public Headers

// Private Headers

typedef void (*input_callback_t) (int gpio_num,TickType_t tick_time); //khai báo con trỏ hàm funtion pointer
typedef void (*input_callback_status_t) (int status); //khai báo con trỏ hàm funtion pointer



typedef enum{
    HI_TO_LO =2,
    LO_TO_HI=1,
    ANY_EDLE=3
} interrupt_type_edge_t;

void input_io_creat(gpio_num_t gpio_num, interrupt_type_edge_t type);
void input_io_get_level(gpio_num_t gpio_num);
void input_set_callback(void *cb);
void input_set_callback_status(void *par);
#endif