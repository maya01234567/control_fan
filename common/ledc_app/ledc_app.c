/* LEDC (LED Controller) fade example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "hal/ledc_types.h"

// static ledc_mode_t speed_mode;
// static ledc_timer_t timer;
// static ledc_channel_t chanel;
/*
 * About this example
 *
 * 1. Start with initializing LEDC module:
 *    a. Set the timer of LEDC first, this determines the frequency
 *       and resolution of PWM.
 *    b. Then set the LEDC channel you want to use,
 *       and bind with one of the timers.
 *
 * 2. You need first to install a default fade function,
 *    then you can use fade APIs.
 *
 * 3. You can also set a target duty directly without fading.
 *
 * 4. This example uses GPIO18/19/4/5 as LEDC output,
 *    and it will change the duty repeatedly.
 *
 * 5. GPIO18/19 are from high speed channel group.
 *    GPIO4/5 are from low speed channel group.
 *
 */

void ledc_init(ledc_timer_bit_t resolution, uint32_t freq_hz, ledc_mode_t speed_mode, ledc_timer_t timer)
{
    /*
     * Prepare and set configuration of timers
     * that will be used by LED Controller
     */
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = resolution, // resolution of PWM duty
        .freq_hz = freq_hz,            // frequency of PWM signal
        .speed_mode = speed_mode,      // timer mode
        .timer_num = timer,            // timer index
        .clk_cfg = LEDC_AUTO_CLK,      // Auto select the source clock
    };
    // Set configuration of timer0 for high speed channels
    ledc_timer_config(&ledc_timer);
}
void add_pin_ledc(ledc_channel_t chanel, gpio_num_t gpio_num, int hpoint, ledc_mode_t speed_mode, ledc_timer_t timer)
{
    /*
     * Prepare individual configuration
     * for each channel of LED Controller
     * by selecting:
     * - controller's channel number
     * - output duty cycle, set initially to 0
     * - GPIO number where LED is connected to
     * - speed mode, either high or low
     * - timer servicing selected channel
     *   Note: if different channels use one timer,
     *         then frequency and bit_num of these channels
     *         will be the same
     */
    ledc_channel_config_t ledc_channel = {
        .channel = chanel,
        .duty = 0,
        .gpio_num = gpio_num,
        .speed_mode = speed_mode,
        .hpoint = hpoint,
        .timer_sel = timer,
    };
    // Set LED Controller with previously prepared configuratio
    ledc_channel_config(&ledc_channel);
}
// 0-8191
// 0-100
void set_duty_ledc(int duty, ledc_mode_t speed_mode, ledc_channel_t chanel)
{
    if (duty == 0)
        duty = 1;
    float duty1 = (float)duty;
    float duty2 = 1 - duty1/255;
    printf("duty1:%3.3f\n",duty2);
    ledc_set_duty(speed_mode, chanel, duty2 * 8192);
    ledc_update_duty(speed_mode, chanel);
}
