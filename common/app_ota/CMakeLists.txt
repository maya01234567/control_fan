set(app_src "app_ota.c")
set(pri_req app_update esp_http_client esp_https_ota)
idf_component_register(SRCS ${app_src}
                       INCLUDE_DIRS "."
                       REQUIRES ${pri_req})
