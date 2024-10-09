#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_mac.h"

#include "dht.h"

#include "wifi.h"
#include "mqtt.h"
#include "timer.h"

static const char *TAG = "DHT";

#define DHT_GPIO GPIO_NUM_25 // DHT11 - KY-015

extern SemaphoreHandle_t conexaoMQTTSemaphore; // Semaforo para controle de conexão Wi-Fi
extern EventGroupHandle_t sleepEventGroup; // Event Group para controle de sleep

extern int DHT_ACTIVE_BIT;

float ultima_temp = __FLT32_MAX__; // Última temperatura lida
float ultima_umi = __FLT32_MAX__;  // Última umidade lida

// Task para leitura do sensor DHT
void dht_task(void *pvParameter)
{
    xEventGroupSetBits(sleepEventGroup, DHT_ACTIVE_BIT);
    char mensagem[200];
    float temp = 0.0f;
    float umi = 0.0f;

    if (dht_read_float_data(DHT_TYPE_DHT11, DHT_GPIO, &umi, &temp) == ESP_OK)
    {
        ESP_LOGI(TAG, "Temperatura: %.1f°C, Umidade: %.1f%%", temp, umi);

        if (ultima_temp != __FLT32_MAX__ && ultima_umi != __FLT32_MAX__)
            sprintf(mensagem, "{\"temperatura\": %f, \"umidade\": %f}", (temp + ultima_temp) / 2.0f, (umi + ultima_umi) / 2.0f);
        else
            sprintf(mensagem, "{\"temperatura\": %f, \"umidade\": %f}", temp, umi);

        ultima_temp = temp;
        ultima_umi = umi;

        if (xSemaphoreTake(conexaoMQTTSemaphore, portMAX_DELAY))
        {
            mqtt_envia_mensagem(THINGSBOARD, "v1/devices/me/telemetry", mensagem); // Manda mensagem para Thingsboard
            xSemaphoreGive(conexaoMQTTSemaphore);
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to read data from DHT11 sensor");
    }

    xEventGroupClearBits(sleepEventGroup, DHT_ACTIVE_BIT);
    vTaskDelete(NULL); // Encerra a Task
}

// Callback para o Timer do DHT
bool dht_callback(void *args)
{
    xTaskCreate(&dht_task, "dht_task", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
    restart_timer();
    return true;
}