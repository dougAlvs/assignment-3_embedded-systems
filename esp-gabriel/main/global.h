#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include <stdbool.h>
#include "cJSON.h"

#define POTENTIOMETER_CHANNEL ADC1_CHANNEL_0  // GPIO 36 (ADC1 CH0)
#define LDR_CHANNEL ADC1_CHANNEL_6            // GPIO 34 (ADC1 CH6)

#define SSID "Pedro S"
#define PASSWORD "991842828"

#define BROKER_URI "ws://test.mosquitto.org:8080"
#define BROKER_INTERFACE_URI "mqtt://164.41.98.25"
#define ENDERECO_MODO_ATUAL "/EMBARCADOS/Iluminacao/modo_atual"
#define TOPICO_POTENCIOMETRO "/EMBARCADOS/Sensores/Potenciometro"
#define TOPICO_LDR "/EMBARCADOS/Sensores/LDR"
#define TOPICO_INTERFACE "v1/devices/me/telemetry"

extern const char *TAG;
extern const char *WIFI_TAG;
extern const char *MQTT_TAG;
extern const char *NVS_TAG;

extern esp_adc_cal_characteristics_t adc1_chars;
extern esp_mqtt_client_handle_t cliente_mqtt;
extern esp_mqtt_client_handle_t cliente_mqtt_interface;
extern TaskHandle_t potenciometro_task_handle;

#endif // GLOBAL_H
