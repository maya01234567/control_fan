//#pragma once
#ifndef UART_IO_H
#define UART_IO_H
// C/C++ Headers

// Other Posix headers

// IDF he<der
#include "esp_err.h" //thư viện nhận diện lỗi
#include "hal/gpio_types.h" //thư viện in các tin nhắn debug

typedef void (*uart_callback_t) (uint8_t *data, size_t length);

// Component header

// Public Headers

// Private Headers
void uart_io_creat(uart_port_t uart_num,uint32_t baudrate,int tx_io_num , int rx_io_num);
void uart_set_callback(void *cb);
#endif