
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "app_nvs.h"

#define USER_NAMESPACE "app"

static nvs_handle_t my_handle;

void app_nvs_set_value(char *key, int *value)
{
    int err;
    err = nvs_open(USER_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else
    {
        printf("Done\n");
    }
    err = nvs_set_i32(my_handle, key, *value);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
    err = nvs_commit(my_handle);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
    nvs_close(my_handle);
}
void app_nvs_get_value(char *key, int *value)
{
    int err;
    err = nvs_open(USER_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else
    {
        printf("Done\n");
    }
    err = nvs_get_i32(my_handle, key, value);
    switch (err)
    {
    case ESP_OK:
        printf("Done\n");
        printf("%s = %d\n", key, *value);
        break;
    case ESP_ERR_NVS_NOT_FOUND:
        printf("The value is not initialized yet!\n");
        break;
    default:
        printf("Error (%s) reading!\n", esp_err_to_name(err));
    }
    nvs_close(my_handle);
}
void app_nvs_set_str(const char *key, const char *str)
{
    int err = 0;
    err = nvs_open(USER_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else
    {
        printf("Done\n");
    }
    err = nvs_set_str(my_handle, key, str);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
    err = nvs_commit(my_handle);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
    nvs_close(my_handle);
}
void app_nvs_get_str(const char *key, char* out_str)
{
    size_t leng;
    //size_t leng = 0;
    int err = 0;
    err = nvs_open(USER_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else
    {
        printf("Done\n");
    }
    nvs_get_str(my_handle, key, out_str,&leng);
    err = nvs_get_str(my_handle, key, out_str,&leng);
    switch (err)
    {
    case ESP_OK:
        printf("Done\n");
        printf("%s = %s,leng = %d\n", key, out_str, leng);
        break;
    case ESP_ERR_NVS_NOT_FOUND:
        printf("The value is not initialized yet!\n");
        break;
    default:
        printf("Error (%s) reading!\n", esp_err_to_name(err));
    }
    nvs_close(my_handle);
}