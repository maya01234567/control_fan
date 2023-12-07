
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi_netif.h"
#include "smart_config.h"
#include "app_config.h"
#include "wifi_iot.h"
#include "http_sever_sta.h"
#include "wifi_sofAP.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
static wifi_config_t wifi_config1;
static provision_type_t provisition_type = PROVISITION_SMARTCONFIG;
status_connect_wifi_t status_connect;
static const char *TAG = "app config";
void connect_wifi()
{
    wifi_connect(wifi_config1.sta.ssid, wifi_config1.sta.password, 3, WIFI_MODE_STA, ESP_IF_WIFI_STA);
}

void set_callback_wifiinfo(char *ssid, int lengssid, char *pass, int lengpass)
{
    printf("=>>ssid:%s\n", ssid);
    printf("=>>ssid:%s\n", pass);
    ESP_ERROR_CHECK(esp_wifi_stop());
    delete_netif_sofAP();
    strncpy((char *)wifi_config.sta.ssid,ssid, lengssid);
    strncpy((char *)wifi_config.sta.password, pass, lengpass);
    status_connect = CONNECT_WIFI;
    // wifi_connect(ssid, pass, 3, WIFI_MODE_STA, ESP_IF_WIFI_STA);
}
int is_provisioned()
{
    static int provisioned;
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    esp_wifi_get_config(WIFI_IF_STA, &wifi_config1);
    ESP_LOGI(TAG, "infor to ap SSID:%s password:%s",
             (uint8_t *)wifi_config1.sta.ssid, (uint8_t *)wifi_config1.sta.password);
    if (wifi_config1.sta.ssid[0] == 0x00) // hien tai = 0 khong cos gi o flash
    {
        provisioned = 1;
    }
    else
    {
        provisioned = 0;
    }
    printf("%d\n", provisioned);
    return provisioned;
}
void select_mode()
{
}
void app_config(void)
{
    int x = is_provisioned();
    if (x == 1)
    {
        if (provisition_type == PROVISITION_SMARTCONFIG)
        {
            // printf("===========>smart\n");
            smart_config_connect();
            status_connect = SELECTTION_MODE_CONNECT
        }
        else if (provisition_type == PROVISITION_ACCESSPOIN)
        {
            wifi_init_softap();
            start_wedsever_connect_sta();
            http_set_callback_infowifi(&set_callback_wifiinfo);
            // printf("===========>\n");
        }
    }
    else
    {
        // printf("===========> direct\n");
        connect_direction(wifi_config1.sta.ssid, wifi_config1.sta.password);
    }
}
void handle_connect_wifi(void)
{
    switch (status_connect)
    {
    // case CHECK_INFO_WIFI:
    // is_provisioned();
    // status_connect = SELECTTION_MODE_CONNECT;
    // break;
    case SELECTTION_MODE_CONNECT:
        app_config();
        break;
    case CONNECT_WIFI:
        connect_wifi();
        break;
    case ERASE_FLASH:
        /* code */
        break;
    case CONNECT_ERROR:
        /* code */
        break;
    default:
        break;
    }
}