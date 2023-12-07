/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "esp_eth.h"

#include "esp_http_server.h"

#include "http_sever_sta.h"

get_infowifi_callback_t get_infowifi_callback = NULL;
/*
 * handlers for the web server.
 */

static const char *TAG = "HTTP_SEVER_START";
static httpd_handle_t server = NULL;

extern const uint8_t index_html_start[] asm("_binary_indexSTART_html_start");
extern const uint8_t index_html_end[] asm("_binary_indexSTART_html_end");

/* An HTTP GET handler */
static esp_err_t hello_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);
    return ESP_OK;
}

static const httpd_uri_t get_data_dht11 = {
    .uri = "/start",
    .method = HTTP_GET,
    .handler = hello_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx = NULL};

static esp_err_t get_infowifi_handler(httpd_req_t *req)
{
    printf("-->\n");
    char *buf;
    size_t buf_len;
    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1)
    {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
        {
            printf("Found URL query => %s\n", buf);
            char ssid[32]="";
            char pass[32]="";
            httpd_query_key_value(buf, "ssid", ssid, sizeof(ssid));
            httpd_query_key_value(buf, "pass", pass, sizeof(pass));
            get_infowifi_callback(ssid,strlen(ssid),pass,strlen(pass)); 
        }
        free(buf);
    }
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t get_infowifi = {
    .uri = "/wifiinfo",
    .method = HTTP_GET,
    .handler = get_infowifi_handler,
    .user_ctx = NULL};

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    if (strcmp("/dht11", req->uri) == 0)
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/dht11 URI is not available");
        return ESP_OK;
    }
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

void start_wedsever_connect_sta(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &get_data_dht11);
        httpd_register_uri_handler(server, &get_infowifi);
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
    }
    else
    {
        ESP_LOGI(TAG, "Error starting server!");
    }
    printf("==> done\n");
}

void stop_wedsever_connect_sta(void)
{
    httpd_stop(server);
    printf("==> done\n");
}
void http_set_callback_infowifi(void *cb)
{
    get_infowifi_callback = cb; // gán địa chỉ của hàm cho con trỏ input_callback
}
