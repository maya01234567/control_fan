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
#include "esp_eth.h"
#include "esp_http_server.h"
#include "http_sever_sta.h"

get_infowifi_callback_t get_infowifi_callback = NULL;
/*
 * handlers for the web server.
 */

static const char *TAG = "HTTP_SEVER_START";
static httpd_handle_t server = NULL;

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");

/* An HTTP GET handler */
static esp_err_t get_index_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);
    return ESP_OK;
}

static const httpd_uri_t get_index_wedserver = {
    .uri = "/start",
    .method = HTTP_GET,
    .handler = get_index_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx = NULL};

static esp_err_t get_infowifi_handler(httpd_req_t *req)
{
    char *buf = (char *)malloc(req->content_len + 1);
    if (buf == NULL)
    {
        ESP_LOGI(TAG,"unable to allocate memoris buf.");
        return ESP_FAIL;
    }
    char *ssid = (char *)malloc(50);
    if (ssid == NULL)
    {
        ESP_LOGI(TAG,"unable to allocate memoris ssid.");
        return ESP_FAIL;
    }
    char *pass = (char *)malloc(50);
    if (pass == NULL)
    {
        ESP_LOGI(TAG,"unable to allocate memoris pass.");
        return ESP_FAIL;
    }
    /* Read the data for the request */
    httpd_req_recv(req, (char *)buf, req->content_len);
    char *token = strtok((char *)buf, "&");
    while (token != NULL)
    {
        if (strstr(token, "ssid"))
        {
            token += 5;
            strcpy(ssid, token);
        }
        else if (strstr(token, "pass"))
        {
            token += 5;
            strcpy(pass, token);
        }
        token = strtok(NULL, "&");
    }
    get_infowifi_callback(ssid, strlen(ssid), pass, strlen(pass));
    httpd_resp_send_chunk(req, NULL, 0);
    free(buf);
    free(ssid);
    free(pass);
    return ESP_OK;
}

static const httpd_uri_t get_infowifi = {
    .uri = "/wifiinfo",
    .method = HTTP_POST,
    .handler = get_infowifi_handler,
    .user_ctx = NULL};

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    if (strcmp("/start", req->uri) == 0)
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/start URI is not available");
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
    if (!server)
    {
        if (httpd_start(&server, &config) == ESP_OK)
        {
            // Set URI handlers
            ESP_LOGI(TAG, "Registering URI handlers");
            httpd_register_uri_handler(server, &get_index_wedserver);
            httpd_register_uri_handler(server, &get_infowifi);
            httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
        }
        else
        {
            httpd_stop(server);
            ESP_LOGI(TAG, "Error starting server!");
        }
    }
}

void stop_wedsever_connect_sta(void)
{
    httpd_stop(server);
}
void http_set_callback_infowifi(void *cb)
{
    get_infowifi_callback = cb; // gán địa chỉ của hàm cho con trỏ input_callback
}
