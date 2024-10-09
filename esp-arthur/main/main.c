#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_mac.h"

#include "esp_sleep.h"
#include "esp_timer.h"
#include "driver/timer.h"
#include "esp32/rom/uart.h"

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "freertos/semphr.h"

#include "wifi.h"
#include "mqtt.h"
#include "timer.h"
#include "sensor_pir.h"
#include "sensor_dht.h"
#include "sleep.h"

#define DHT_GPIO GPIO_NUM_25 // DHT11 - KY-015
#define PIR_GPIO GPIO_NUM_27 // PIR - HC-SR501
#define LED_GPIO GPIO_NUM_2  // LED Placa

SemaphoreHandle_t conexaoWifiSemaphore; // Semaforo para controle de conexão Wi-Fi
SemaphoreHandle_t conexaoMQTTSemaphore; // Semaforo para controle de conexão MQTT
EventGroupHandle_t sleepEventGroup;     // Event Group para controle de sleep

// =================================================================================================
// ||                                 DHT - Temperatura e Umidade                                 ||
// =================================================================================================

const int DHT_ACTIVE_BIT = BIT1; // Bit para controle de atividade do DHT

// =================================================================================================
// ||                                  PIR - Sensor de Presença                                   ||
// =================================================================================================

const int PIR_ACTIVE_BIT = BIT0; // Bit para controle de atividade do PIR

QueueHandle_t pir_evt_queue = NULL; // Fila de eventos do PIR

TaskHandle_t PIR_TaskHandle = NULL; // Handle da task do PIR

// Função de handler da fila do PIR
static void IRAM_ATTR pir_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(pir_evt_queue, &gpio_num, NULL);
}

void app_main(void)
{
    // Configuração dos GPIOs
    gpio_config_t dht_config = {
        .pin_bit_mask = (1ULL << DHT_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&dht_config);

    gpio_config_t pir_config = {
        .pin_bit_mask = (1ULL << PIR_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE};
    gpio_config(&pir_config);

    esp_rom_gpio_pad_select_gpio(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    // Configuração da fila de eventos do PIR
    pir_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIR_GPIO, pir_isr_handler, (void *)PIR_GPIO);

    // Inicializa o NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Inicializa os semáforos e o event group
    conexaoWifiSemaphore = xSemaphoreCreateBinary();
    conexaoMQTTSemaphore = xSemaphoreCreateBinary();
    sleepEventGroup = xEventGroupCreate();

    // Inicializa o Wi-Fi e MQTT
    wifi_start();
    mqtt_start();

    // Inicializa as tasks e o timer para o DHT
    xEventGroupClearBits(sleepEventGroup, PIR_ACTIVE_BIT);
    xEventGroupClearBits(sleepEventGroup, DHT_ACTIVE_BIT);

    xTaskCreate(&pir_task, "pir_task", configMINIMAL_STACK_SIZE * 3, NULL, 5, &PIR_TaskHandle);
    xTaskCreate(&dht_task, "dht_task", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
    init_timer(dht_callback, 10000000ULL);

    sleep(1);
    xTaskCreate(&sleep_management_task, "sleep_management_task", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
}