/* OTA example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "string.h"

static const char *TAG = "OTA_APP";
// extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
// extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

#define OTA_URL_SIZE 256
#define LOG_PROCESS_OTA_DATA 1

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    }
    return ESP_OK;
}
esp_http_client_config_t my_config = {
    .url = "http://192.168.2.6/app_ota.bin",
    //.cert_pem = (char *)server_cert_pem_start,
    .cert_pem = NULL,
    .event_handler = _http_event_handler,
    .keep_alive_enable = true,
};
static int esp_https_ota_1(const esp_http_client_config_t *config)
{
    uint8_t tem = 0;
    if (!config)
    {
        ESP_LOGE(TAG, "esp_http_client config not found");
        return ESP_ERR_INVALID_ARG;
    }
    esp_https_ota_config_t ota_config = {
        .http_config = config,
    };
    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (https_ota_handle == NULL)
    {
        return ESP_FAIL;
    }

    // init other object for get length
    esp_https_ota_handle_t __http_client = esp_http_client_init(ota_config.http_config); // khởi tạo 1 biến truy cập vào http_config của ota
    if (__http_client == NULL)
    {
        ESP_LOGE(TAG, "Failed to initialise HTTP connecttion");
        goto END;
    }
    err = esp_http_client_open(__http_client, 0); // mở kết nối
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialise HTTP connecttion : %s", esp_err_to_name(err));
        goto END;
    }
    int length_image_firmware = esp_http_client_fetch_headers(__http_client); // nhận kích thước tệp tin từ http trước khi tải
    ESP_LOGI(TAG, "Length Image : %d", length_image_firmware);

    while (1)
    {
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS)
        {
            break;
        }
#if (LOG_PROCESS_OTA_DATA == 1)
        int process_len = esp_https_ota_get_image_len_read(https_ota_handle); // lấy kích thước đã nạp cho tới hiện tại đã load
        uint8_t duty = (process_len * 100 / length_image_firmware);
        if (tem != duty)
        {
            ESP_LOGI(TAG, "process OTA : %d%%", duty);
            tem = duty;
        }
#endif
    }
    esp_err_t ota_finish_err = esp_https_ota_finish(https_ota_handle);
    if (err != ESP_OK)
    {
        /* If there was an error in esp_https_ota_perform(),
           then it is given more precedence than error in esp_https_ota_finish()
         */
        return err;
    }
    else if (ota_finish_err != ESP_OK)
    {
        return ota_finish_err;
    }
    else
    {
        return ESP_OK;
    }
END:
    ESP_LOGI(TAG, "ERROR");
    return ESP_FAIL;
}
void app_ota_start(const char* url_link)
{
    ESP_LOGI(TAG, "Starting OTA");
    esp_http_client_config_t my_config = {
    .url = url_link,
    //.cert_pem = (char *)server_cert_pem_start,
    .cert_pem = NULL,
    .event_handler = _http_event_handler,
    .keep_alive_enable = true,
    };

#ifdef CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL_FROM_STDIN // nếu nhãn này được define thì thực hiện lấy url từ UART
    char url_buf[OTA_URL_SIZE];
    if (strcmp(config.url, "FROM_STDIN") == 0)
    {
        example_configure_stdin_stdout();
        fgets(url_buf, OTA_URL_SIZE, stdin);
        int len = strlen(url_buf);
        url_buf[len - 1] = '\0';
        config.url = url_buf;
    }
    else
    {
        ESP_LOGE(TAG, "Configuration mismatch: wrong firmware upgrade image url");
        abort();
    }
#endif

#ifdef CONFIG_EXAMPLE_SKIP_COMMON_NAME_CHECK // nếu nhãn này được define thì thực hiện lệnh trong đó
    config.skip_cert_common_name_check = true;
#endif
    const esp_http_client_config_t *config = &my_config;
    esp_err_t ret = esp_https_ota_1(config);
    if (ret == ESP_OK)
    {
        esp_restart();
    }
    else
    {
        ESP_LOGE(TAG, "Firmware upgrade failed");
    }
    // while (1)
    // {
        // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // }
}