#ifndef WIFI_H
#define WIFI_H

#include "esp_event.h"

void inicializar_wifi(void);
void tratar_eventos_wifi(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

#endif // WIFI_MANAGER_H
