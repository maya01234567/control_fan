set(app_src "mqtt_app.c")
set(pri_req init_sofap mqtt jSON_APP)
idf_component_register(SRCS ${app_src}
                       INCLUDE_DIRS "."
                       REQUIRES ${pri_req})
