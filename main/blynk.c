
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "blynk_iot.h"
#include "app_config.h"

#define BLINK_GPIO CONFIG_BLINK_GPIO

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");


void app_main(void)
{
    app_config();
    // while (1)
    // {
    // }
    // blynk_client_t *client = malloc(sizeof(blynk_client_t));
    // blynk_init(client);
    // blynk_options_t opt = {
    // .token = BLYNK_TOKEN,
    // .server = BLYNK_SERVER,
    // /* Use default timeouts */
    // };
    // blynk_set_options(client, &opt);
    // /* Subscribe to state changes and errors */
    // blynk_set_state_handler(client, state_handler, NULL);
    // /* blynk_set_handler sets hardware (BLYNK_CMD_HARDWARE) command handler */
    // blynk_set_handler(client, "vw", vw_handler, NULL);
    // blynk_set_handler(client, "vr", vr_handler, NULL);
    // /* Start Blynk client task */
    // blynk_start(client);
}
