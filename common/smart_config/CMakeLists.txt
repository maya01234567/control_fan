set(app_src "smart_config.c")
set(pri_req wpa_supplicant nvs_flash)
idf_component_register(SRCS ${app_src}
                       INCLUDE_DIRS "."
                       REQUIRES ${pri_req})
