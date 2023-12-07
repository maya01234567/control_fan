set(app_src "wifi_sofAP.c")
set(pri_req)
idf_component_register(SRCS ${app_src}
                       INCLUDE_DIRS "."
                       REQUIRES ${pri_req})
