set(app_src "jSON_APP.c")
set(pri_req )
idf_component_register(SRCS ${app_src}
                       INCLUDE_DIRS "."
                       REQUIRES ${pri_req})
