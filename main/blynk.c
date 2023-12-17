
#include <stdio.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "blynk_iot.h"
#include "app_config.h"
#include "esp_event.h"
#include "otadrive_esp.h"

#define BLINK_GPIO CONFIG_BLINK_GPIO
static const char *TAG = "otadrive_idf_example";
#define OTADRIVE_APIKEY "da260e55-0566-4581-85e7-22d8b9b3483b"
#define APP_VERSION "v@1.0"

// static SemaphoreHandle_t s_semph_get_ip_addrs = NULL;
// static int s_retry_num = 0;

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
void otadrive_test(void *pvParameter);

static void otadrive_event_handler(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data)
{
    if (event_base == OTADRIVE_EVENTS)
    {
        switch (event_id)
        {
        case OTADRIVE_EVENT_START_CHECK:
            ESP_LOGI(TAG, "OTA driver start check");
            break;
        case OTADRIVE_EVENT_UPDATE_AVAILABLE:
            ESP_LOGI(TAG, "OTA driver update avaiable");
            break;
        case OTADRIVE_EVENT_START_UPDATE:
            ESP_LOGI(TAG, "OTA start update");
            break;
        case OTADRIVE_EVENT_FINISH_UPDATE:
            ESP_LOGI(TAG, "OTA driver Finish update");
            break;
        case OTADRIVE_EVENT_UPDATE_FAILED:
            ESP_LOGI(TAG, "OTA driver update failed");
            break;
        case OTADRIVE_EVENT_FIRMWARE_UPDATE_PROGRESS:
            ESP_LOGD(TAG, "OTA driver update fimware progress");
            break;
        case OTADRIVE_EVENT_STORAGE_UPDATE_PROGRESS:
            ESP_LOGI(TAG, "OTA driver storage update progress");
            break;
        case OTADRIVE_EVENT_PENDING_REBOOT:
            ESP_LOGI(TAG, "OTA reboot");
            break;
        case WIFI_CONNECTTED:
            ESP_LOGI(TAG, "CONNECTTED WIFI");
            xTaskCreate(&otadrive_test, "otadrive_example_task", 1024 * 16, NULL, 5, NULL);
            break;
        case WIFI_DISCONNECTTED:
            ESP_LOGI(TAG, "DISCONNECTTED WIFI");
            break;
        }
    }
}

void otadrive_test(void *pvParameter)
{
    otadrive_setInfo(OTADRIVE_APIKEY, APP_VERSION);

    while (1)
    {
        if (otadrive_timeTick(5000))
        {
            // check size và lỗi khi mở nhận http
            otadrive_result r = otadrive_updateFirmwareInfo();
            ESP_LOGI(TAG, "RES %d,%lu", r.code, (long unsigned int)r.available_size);
            if (r.code == OTADRIVE_NewFirmwareExists)
            {
                ESP_LOGI(TAG, "Lets download new firmware %s,%luBytes. Current firmware is %s",
                         r.available_version, (long unsigned int)r.available_size, otadrive_currentversion());
                otadrive_updateFirmware();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void handler_task(void *cb)
{
    while (1)
    {
        handle_connect_wifi();
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_event_handler_register(OTADRIVE_EVENTS, ESP_EVENT_ANY_ID, &otadrive_event_handler, NULL);
    app_config_init();
    xTaskCreate(handler_task, "handler_task", 3072, NULL, 3, NULL);
    // xTaskCreate(&otadrive_test, "otadrive_example_task", 1024 * 16, NULL, 5, NULL);
    //  while (1)
    //  {
    //  }
    //  blynk_client_t *client = malloc(sizeof(blynk_client_t));
    //  blynk_init(client);
    //  blynk_options_t opt = {
    //  .token = BLYNK_TOKEN,
    //  .server = BLYNK_SERVER,
    //  /* Use default timeouts */
    //  };
    //  blynk_set_options(client, &opt);
    //  /* Subscribe to state changes and errors */
    //  blynk_set_state_handler(client, state_handler, NULL);
    //  /* blynk_set_handler sets hardware (BLYNK_CMD_HARDWARE) command handler */
    //  blynk_set_handler(client, "vw", vw_handler, NULL);
    //  blynk_set_handler(client, "vr", vr_handler, NULL);
    //  /* Start Blynk client task */
    //  blynk_start(client);
}
