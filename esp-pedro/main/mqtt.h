#ifndef MQTT_H
#define MQTT_H

#include "mqtt_client.h"

void inicializar_mqtt(void);
void inicializar_mqtt_interface(void);
void tratar_eventos_mqtt(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
void publicar_mensagem(void *pvParameters);

#endif // MQTT_H
