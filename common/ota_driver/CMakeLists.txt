set(app_src "otadrive_esp.c")
set(pri_req esp_http_client esp-tls bootloader_support app_update esp_https_ota)
idf_component_register(SRCS ${app_src}
                        INCLUDE_DIRS "."
                        REQUIRES ${pri_req})
