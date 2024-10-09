#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"


void wifi_init_sta(void);
void wifi_init_ap(void);
esp_err_t save_wifi_config(const char* ssid, const char* password);
esp_err_t clear_wifi_config();

#endif