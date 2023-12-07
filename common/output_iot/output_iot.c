#include <stdio.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/gpio.h>
#include "output_iot.h"

void output_io_creat(gpio_num_t gpio_num)
{
    gpio_pad_select_gpio(gpio_num);
    gpio_set_direction(gpio_num, GPIO_MODE_INPUT_OUTPUT);
}
void output_io_set_level(gpio_num_t gpio_num, int level)
{
    gpio_set_level(gpio_num, level);
}
void output_io_toggle(gpio_num_t gpio_num)
{

    int old_level = gpio_get_level(gpio_num);
    gpio_set_level(gpio_num, 1 - old_level);
}
void output_io_blink(gpio_num_t gpio_num)
{

    int x = 0;
    gpio_set_level(gpio_num,x);
    vTaskDelay(500/portTICK_PERIOD_MS);
    gpio_set_level(gpio_num,1-x);
    vTaskDelay(500/portTICK_PERIOD_MS);
}
