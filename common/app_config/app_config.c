
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
#include <esp_event.h>
#include "otadrive_esp.h"

#include "lwip/err.h"
#include "lwip/sys.h"

static uint8_t provisioned;
info_config_wifi_t info_config_wifi;
const char sample_err[] = "ssiderr";

static const char *TAG = "app config";

static void set_callback_wifiinfo(char *ssid, int lengssid, char *pass, int lengpass)
{
    strncpy((char *)info_config_wifi.wifi_config.sta.ssid, ssid, lengssid);
    strncpy((char *)info_config_wifi.wifi_config.sta.password, pass, lengpass);
}
static void set_callback_wifiinfo_smart(wifi_config_t *__wifi_config)
{
    printf("=>>ssid:%s\n", __wifi_config->sta.ssid);
    printf("=>>ssid:%s\n", __wifi_config->sta.password);
}
static void connect_wifi()
{
    printf("connect_ted\n");
    int i = wifi_connect_iot(&info_config_wifi, 3, WIFI_MODE_STA, ESP_IF_WIFI_STA);
    if (i == ESP_OK)
    {
        ESP_LOGI(TAG,"SUCESS");
        ESP_ERROR_CHECK(esp_event_post(OTADRIVE_EVENTS, WIFI_CONNECTTED, NULL, 0, portMAX_DELAY));
        info_config_wifi.status_connect = NORMAL;
    }
    else if (i == ESP_FAIL)
    {
        ESP_LOGI(TAG,"FAILED");
        strcpy((char *)info_config_wifi.wifi_config.sta.ssid, sample_err);
        info_config_wifi.status_connect = SELECTTION_MODE_CONNECT;
        info_config_wifi.sta = 1;
    }
}
static void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&cfg) == ESP_OK)
    {
        esp_wifi_get_config(WIFI_IF_STA, &info_config_wifi.wifi_config);
        ESP_LOGI(TAG, "infor to ap SSID:%s password:%s", (uint8_t *)info_config_wifi.wifi_config.sta.ssid, (uint8_t *)info_config_wifi.wifi_config.sta.password);
        info_config_wifi.sta = 1;
    }
    else
    {
        ESP_LOGI(TAG, "init wifi error");
    }
}

static int is_provisioned()
{
    if (info_config_wifi.block_init == 1)
    {
        info_config_wifi.block_init = 0;
        wifi_init();
    }
    if ((info_config_wifi.wifi_config.sta.ssid[0] == 0X00) || (strcmp((char*)info_config_wifi.wifi_config.sta.ssid, sample_err) == 0)) // hien tai = 0 khong cos gi o flash
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
        if (info_config_wifi.provision_type == PROVISITION_SMARTCONFIG && info_config_wifi.sta == 1)
        {
            info_config_wifi.sta = 0;
            set_callback_infowifi_smart(&set_callback_wifiinfo_smart);
            smart_config_connect(&info_config_wifi.wifi_config);
        }
        else if (info_config_wifi.provision_type == PROVISITION_ACCESSPOIN && info_config_wifi.sta == 1)
        {
            info_config_wifi.sta = 0;
            wifi_init_softap();
            start_wedsever_connect_sta();
            http_set_callback_infowifi(&set_callback_wifiinfo);
        }
    }
    else
    {
        info_config_wifi.status_connect = CONNECT_WIFI;
    }
}
void handle_connect_wifi(void)
{
    // printf("pro : %d \n", provisioned);
    // printf("status_connect : %d \n", info_config_wifi.status_connect);
    // printf("sta : %d \n", info_config_wifi.sta);
    // printf("BLOCK INIT : %d \n", info_config_wifi.block_init);
    switch (info_config_wifi.status_connect)
    {
    case NORMAL:
        break;
    case SELECTTION_MODE_CONNECT:
        app_config();
        break;
    case CONNECT_WIFI:
    {
        connect_wifi();
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
void app_config_init()
{
    printf("enter \n");
    info_config_wifi.provision_type = PROVISITION_SMARTCONFIG;
    info_config_wifi.status_connect = SELECTTION_MODE_CONNECT;
    info_config_wifi.sta = 1;
    info_config_wifi.block_init = 1;
}