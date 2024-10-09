#include "global.h"
#include "nvs_manager.h"
#include "wifi.h"
#include "mqtt.h"
#include "sensores.h"

const char *TAG = "SENSORES";
const char *WIFI_TAG = "WIFI";
const char *MQTT_TAG = "MQTT";
const char *NVS_TAG = "NVS";

esp_adc_cal_characteristics_t adc1_chars;
esp_mqtt_client_handle_t cliente_mqtt;
esp_mqtt_client_handle_t cliente_mqtt_interface;
TaskHandle_t potenciometro_task_handle = NULL;

void app_main(void) {
    inicializar_nvs();

    inicializar_wifi();

    inicializar_mqtt();
    inicializar_mqtt_interface();

    configurar_leitura_analogica();

    xTaskCreate(ldr_task, "ldr_task", 2048, NULL, 5, NULL);
}

