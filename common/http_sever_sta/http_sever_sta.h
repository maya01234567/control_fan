#ifndef __HTTP_SEVER_APP_CONNECT_H
#define __HTTP_SEVER_APP_CONNECT_H

typedef void (*get_infowifi_callback_t) (char* ssid,int lengssid,char* pass,int lengpass);

void stop_wedsever_connect_sta(void);
void start_wedsever_connect_sta(void);
void http_set_callback_infowifi(void *cb);
#endif