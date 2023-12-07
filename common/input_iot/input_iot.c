#include <stdio.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include <freertos/timers.h>
#include <driver/gpio.h>
#include "input_iot.h"
#include "sdkconfig.h"

input_callback_t input_callback = NULL;
input_callback_status_t input_callback_status = NULL;
static TimerHandle_t xTimers;
int x = 0;
static TickType_t start = 0;
static TickType_t stop = 0;

void vTimerCallback(TimerHandle_t xTimer)
{
    uint32_t ulCount;
    configASSERT(xTimer);
    ulCount = (uint32_t)pvTimerGetTimerID(xTimer);
    if (ulCount == 0)
    {
        x=1;
        printf("timeout\n");
        input_callback_status(x);
        x=0;
    }
}

static void IRAM_ATTR gpio_input_handler(void *arg)
{
    static TickType_t buttonPressDuration = 0;
    int gpio_num = (uint32_t)arg;
    if (gpio_get_level(gpio_num) == 1)
    {
        xTimerStart(xTimers, 0);
        start = xTaskGetTickCount();
    }
    else if (gpio_get_level(gpio_num) == 0)
    {
        x=0;
        xTimerStop(xTimers, 0);
        stop = xTaskGetTickCount();
        buttonPressDuration = stop - start;
    }
    input_callback(gpio_num,buttonPressDuration); // khi xảy ra sự kiện ngắt gọi truyền tham số cho hàm ở main
}

void input_io_creat(gpio_num_t gpio_num, interrupt_type_edge_t type)
{
    gpio_pad_select_gpio(gpio_num);
    gpio_set_direction(gpio_num, GPIO_MODE_INPUT);
    gpio_set_pull_mode(gpio_num, GPIO_PULLDOWN_ONLY);
    gpio_set_intr_type(gpio_num, type);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(gpio_num, gpio_input_handler, (void *)gpio_num); // phát hiejn sự kiện ngắt và truyền cho hàm xử lý
    xTimers = xTimerCreate("Timer", pdMS_TO_TICKS(5000), pdFAIL, (void *)0, vTimerCallback);
}
void input_to_get_level(gpio_num_t gpio_num)
{
    return gpio_get_level(gpio_num);
}

void input_set_callback(void *cb)
{
    input_callback = cb; // gán địa chỉ của hàm cho con trỏ input_callback
}
void input_set_callback_status(void *par)
{
    input_callback_status = par; // gán địa chỉ của hàm cho con trỏ input_callback
}