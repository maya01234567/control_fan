//#pragma once
#ifndef WIFI_INIT_SOFAP_IO_H
#define WIFI_INIT_SOFAP_IO_H
// C/C++ Headers

// Other Posix headers

// IDF he<der
#include "esp_err.h" //thư viện nhận diện lỗi
#include "hal/gpio_types.h" //thư viện in các tin nhắn debug

// Component header

// Public Headers

// Private Headers

void wifi_init_softap();
void delete_netif_sofAP();

#endif