#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H
#include "mqtt_client.h"

void mqtt_app_start(void);
void publish_led_state();
void handle_mqtt_message(const char *data);
void mqtt_publish_message(const esp_mqtt_client_handle_t client, const char *topic, const char *message);

#endif
