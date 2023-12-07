set(app_src "wifi_iot.c")
set(pri_req nvs_flash)
idf_component_register(SRCS ${app_src}
                       INCLUDE_DIRS "."
                       REQUIRES ${pri_req})
