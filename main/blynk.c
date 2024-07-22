
#include <stdio.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "blynk_iot.h"
#include "app_config.h"
#include "esp_event.h"
#include "timer_tick.h"
#include "output_iot.h"
#include "otadrive_esp.h"
#include "cJSON.h"
#include "mqtt_app.h"
#include "string.h"

#define TIME_REQUEST_AGAIN 900000
static const char *TAG = "otadrive";
#define OTADRIVE_APIKEY "da260e55-0566-4581-85e7-22d8b9b3483b"
#define APP_VERSION "v@1.6"

// static SemaphoreHandle_t s_semph_get_ip_addrs = NULL;
// static int s_retry_num = 0;

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
void otadrive_test(void *pvParameter);
// void create_json_string();

// void blynk_task(void);

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
        {
            ESP_LOGI(TAG, "CONNECTTED WIFI");
            xTaskCreate(&otadrive_test, "otadrive_example_task", 1024 * 16, NULL, 5, NULL);
            mqtt_app_init();
            // blynk_task();
            // xTaskCreate(&blynk_task, "blynk test", 1024 * 2, NULL, 5, NULL);
            break;
        }
        case WIFI_DISCONNECTTED:
            ESP_LOGI(TAG, "DISCONNECTTED WIFI");
            break;
        }
    }
}

static void mqtt_even(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    if (event_base == EVENT_DEV_BASE)
    {
        if (event_id == MQTT_DEV_EVENT_CONNECTED)
        {
            ESP_LOGW(TAG, "Detected MQTT_DEV_EVENT_CONNECTED");
            // create_json_string();
            //  app_mqtt_subscribe("/v1.6/devices/demo2/#");
            app_mqtt_subscribe("downlink/ds/VP_ADC");
            app_mqtt_subscribe("downlink/ds/vw");
        }
        else if (event_id == MQTT_DEV_EVENT_DISCONNECT)
        {
            ESP_LOGW(TAG, "Detected MQTT_DEV_EVENT_DISCONNECT");
        }
        else if (event_id == MQTT_DEV_EVENT_DATA)
        {
            ESP_LOGW(TAG, "Detected MQTT_DEV_EVENT_DATA");
        }
        else if (event_id == MQTT_DEV_EVENT_SUBSCRIBED)
        {
            ESP_LOGW(TAG, "Detected MQTT_DEV_EVENT_SUBSCRIBED");
            // app_mqtt_publish("/v1.6/devices/demo2", "hello");
        }
    }
}
void url_data(int length_topic,char *topic, int length, char *url)
{
    ESP_LOGW(TAG, "Detected data");
    ESP_LOGI(TAG, "TOPIC=%.*s\r\n",length_topic, topic);
    ESP_LOGI(TAG, "DATA=%.*s\r\n", length, url);
    if(strncmp(topic,"downlink/ds/vw",length_topic) == 0)
    {
        ESP_LOGI(TAG, "BUTTON=%.*s\r\n", length, url);
        ESP_LOGI(TAG, "BUTTON INT=%d\r\n",atoi(url));
        output_io_set_level(GPIO_NUM_2,atoi(url));
    }
}

void otadrive_test(void *pvParameter)
{
    otadrive_setInfo(OTADRIVE_APIKEY, APP_VERSION);

    while (1)
    {
        // check size và lỗi khi mở nhận http
        otadrive_result r = otadrive_updateFirmwareInfo();
        ESP_LOGI(TAG, "RES %d,%lu", r.code, (long unsigned int)r.available_size);
        if (r.code == OTADRIVE_NewFirmwareExists)
        {
            ESP_LOGI(TAG, "Lets download new firmware %s,%lu Bytes. Current firmware is %s",
                     r.available_version, (long unsigned int)r.available_size, otadrive_currentversion());
            otadrive_updateFirmware();
        }
        vTaskDelay(pdMS_TO_TICKS(TIME_REQUEST_AGAIN)); // 15p hỏi 1 lan
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

void blink_app(void)
{
    output_io_toggle(GPIO_NUM_2);
}

// void create_json_string()
// {
////Tạo một đối tượng cJSON
// cJSON *data = cJSON_CreateObject();
//
////Thêm các trường vào đối tượng JSON
// cJSON_AddItemToObject(data, "temperature", cJSON_CreateNumber(temperature));
// cJSON_AddItemToObject(data, "humidity", cJSON_CreateNumber(humidity));
// cJSON_AddItemToObject(data, "temperature1", cJSON_CreateNumber(temperature1));
// cJSON_AddItemToObject(data, "humidity1", cJSON_CreateNumber(humidity1));
////cJSON_AddItemToObject(data, "LED", cJSON_CreateNumber(1));
//
////Chuyển đối tượng cJSON thành chuỗi JSON
// char *json_string = cJSON_Print(data);
// app_mqtt_publish("/v1.6/devices/demo2", json_string);
//
////In chuỗi JSON
// printf("JSON String: %s\n", json_string);
//
////Giải phóng bộ nhớ được cấp phát cho đối tượng cJSON và chuỗi JSON
// cJSON_Delete(data);
// free(json_string);
// }

void blynk_task(void)
{
}

void app_main(void)
{
    output_io_creat(GPIO_NUM_2);
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_event_handler_register(OTADRIVE_EVENTS, ESP_EVENT_ANY_ID, &otadrive_event_handler, NULL);
    esp_event_handler_register(EVENT_DEV_BASE, ESP_EVENT_ANY_ID, &mqtt_even, NULL);
    app_config_init();
    mqtt_data_set_callback(&url_data);
    xTaskCreate(handler_task, "handler_task", 3072, NULL, 3, NULL);
    //  while (1)
    //  {
    //  }
    // blynk_client_t *client = malloc(sizeof(blynk_client_t));
    // blynk_init(client);
    // blynk_options_t opt = {
    // .token = "GCQ4dDHBEYi5eYP9iHdQGJ7ENDKDcfSN",
    // .server = "blynk.io",
    // /* Use default timeouts */
    // };
    // blynk_set_options(client, &opt);
    // /* Subscribe to state changes and errors */
    //  blynk_set_state_handler(client, state_handler, NULL);
    //  /* blynk_set_handler sets hardware (BLYNK_CMD_HARDWARE) command handler */
    //  blynk_set_handler(client, "vw", vw_handler, NULL);
    //  blynk_set_handler(client, "vr", vr_handler, NULL);
    // /* Start Blynk client task */
    // blynk_start(client);
}
