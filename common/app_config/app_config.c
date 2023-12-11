
#include <string.h>
#include "esp_wifi.h"
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

static uint8_t provisioned;
static uint8_t sta = 0;
static uint8_t block_init = 1;
static wifi_config_t wifi_config;
static wifi_config_t *wifi_config_cate;
static provision_type_t provisition_type = PROVISITION_SMARTCONFIG;
static status_connect_wifi_t status_connect = SELECTTION_MODE_CONNECT;
static const char *TAG = "app config";


static void set_callback_wifiinfo(char *ssid, int lengssid, char *pass, int lengpass)
{
    printf("=>>ssid:%s\n", ssid);
    printf("=>>ssid:%s\n", pass);
    ESP_ERROR_CHECK(esp_wifi_stop());
    delete_netif_sofAP();
    strncpy((char *)wifi_config_cate->sta.ssid, ssid, lengssid);
    strncpy((char *)wifi_config_cate->sta.password, pass, lengpass);
}
static void set_callback_wifiinfo_smart(wifi_config_t* __wifi_config)
{
    printf("=>>ssid:%s\n", __wifi_config->sta.ssid);
    printf("=>>ssid:%s\n", __wifi_config->sta.password);
}
static void connect_wifi()
{
    printf("connect_ted\n");
    wifi_connect_iot(&wifi_config, 3, WIFI_MODE_STA, ESP_IF_WIFI_STA);
}
static void wifi_init(void)
{
    printf("init\n");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&cfg) == ESP_OK)
    {
        esp_wifi_get_config(WIFI_IF_STA, &wifi_config);
        ESP_LOGI(TAG, "infor to ap SSID:%s password:%s", (uint8_t *)wifi_config.sta.ssid, (uint8_t *)wifi_config.sta.password);
        sta = 1;
    }
    else
    {
        ESP_LOGI(TAG, "init wifi error");
    }
}

static int is_provisioned()
{
    if (block_init == 1)
    {
        block_init = 0;
        wifi_init();
    }
    if (wifi_config.sta.ssid[0] == 0x00) // hien tai = 0 khong cos gi o flash
    {
        provisioned = 1;
    }
    else
    {
        provisioned = 0;
    }
    return provisioned;
}
static void app_config(void)
{
    provisioned = is_provisioned();
    if (provisioned == 1)
    {
        if (provisition_type == PROVISITION_SMARTCONFIG && sta == 1)
        {
            sta = 0;
            set_callback_infowifi_smart(&set_callback_wifiinfo_smart);
            smart_config_connect(&wifi_config);
            status_connect = CONNECT_WIFI;
        }
        else if (provisition_type == PROVISITION_ACCESSPOIN)
        {
            wifi_init_softap();
            start_wedsever_connect_sta();
            http_set_callback_infowifi(&set_callback_wifiinfo);
            status_connect = CONNECT_WIFI;
        }
    }
    else
    {
        status_connect = CONNECT_WIFI;
    }
}
void handle_connect_wifi(void)
{
    switch (status_connect)
    {
    case NORMAL:
        break;
    case SELECTTION_MODE_CONNECT:
        app_config();
        break;
    case CONNECT_WIFI:
    {
        connect_wifi();
        status_connect = NORMAL;
        break;
    }
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