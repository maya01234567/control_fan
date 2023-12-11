/* Esptouch example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "smart_config.h"
#include <string.h>
#include <stdlib.h>
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_smartconfig.h"
#include "app_config.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define ESPTOUCH_DONE_BIT BIT2

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static info_wifi_t info_wifi_smart;
static EventGroupHandle_t s_wifi_event_group;
static wifi_config_t* wifi_config;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
static const char *TAG = "SMART_CONFIG";

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "wifi_start");
        ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));
        smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE)
    {
        ESP_LOGI(TAG, "Scan done");
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL)
    {
        ESP_LOGI(TAG, "Found channel");
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD)
    {
        ESP_LOGI(TAG, "Got SSID and password");

        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        bzero(wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config->sta.ssid, evt->ssid, sizeof(wifi_config->sta.ssid));
        memcpy(wifi_config->sta.password, evt->password, sizeof(wifi_config->sta.password));
        wifi_config->sta.bssid_set = evt->bssid_set;
        if (wifi_config->sta.bssid_set == true)
        {
            memcpy(wifi_config->sta.bssid, evt->bssid, sizeof(wifi_config->sta.bssid));
        }
        ESP_LOGI(TAG, "SSID:%s", wifi_config->sta.ssid);
        ESP_LOGI(TAG, "PASSWORD:%s", wifi_config->sta.password);
        info_wifi_smart(wifi_config);
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE)
    {
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}

static void initialise_wifi(void)
{
    s_wifi_event_group = xEventGroupCreate();
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_sc_got_if;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_sc_got_if));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           ESPTOUCH_DONE_BIT,
                                           pdTRUE,
                                           pdFAIL,
                                           portMAX_DELAY);
    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & ESPTOUCH_DONE_BIT)
    {
        ESP_LOGI(TAG, "smartconfig over");
        esp_smartconfig_stop();
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
    /* The event will not be processed after unregister */
    // cancel  handler
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(SC_EVENT, ESP_EVENT_ANY_ID, instance_sc_got_if));
    vEventGroupDelete(s_wifi_event_group);
}

void smart_config_connect(wifi_config_t* _wifi_config)
{
    wifi_config = _wifi_config;
    initialise_wifi();
}
void set_callback_infowifi_smart(void *cb)
{
    info_wifi_smart = cb;
}