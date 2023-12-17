set(app_src "app_config.c")
set(pri_req smart_config connec_wifi log http_sever_sta init_sofap esp_wifi ota_driver)
idf_component_register(SRCS ${app_src}
                       INCLUDE_DIRS "."
                       REQUIRES ${pri_req})
