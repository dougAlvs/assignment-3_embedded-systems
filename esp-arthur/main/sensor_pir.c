#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_mac.h"

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "freertos/semphr.h"

#include "wifi.h"
#include "mqtt.h"

static const char *TAG = "PIR";

#define PIR_GPIO GPIO_NUM_27 // PIR - HC-SR501
#define LED_GPIO GPIO_NUM_2  // LED Placa

extern SemaphoreHandle_t conexaoMQTTSemaphore; // Semaforo para controle de conexão Wi-Fi
extern EventGroupHandle_t sleepEventGroup; // Event Group para controle de sleep

extern int PIR_ACTIVE_BIT;

extern QueueHandle_t pir_evt_queue; // Fila de eventos do PIR

volatile BaseType_t PIR_task_suspended = pdFALSE; // Flag para controle de suspensão de task

// Task para detectar presença com o PIR
void pir_task(void *pvParameter)
{
    char mensagem_led[50];
    char mensagem_on[50];
    // char mensagem_off[50];
    uint32_t io_num;
    while (1)
    {
        if (modo_movimento && xQueueReceive(pir_evt_queue, &io_num, portMAX_DELAY))
        {
            xEventGroupSetBits(sleepEventGroup, PIR_ACTIVE_BIT); // Sinaliza atividade do PIR
            int pir_status = gpio_get_level(io_num);
            if (pir_status)
            {
                ESP_LOGI(TAG, "Movimento detectado!");
                sprintf(mensagem_led, "{\"movimento\": true}");
                
                sprintf(mensagem_on, "{\"movimento\": \"Movimento Detectado\"}");
                
                if (xSemaphoreTake(conexaoMQTTSemaphore, portMAX_DELAY))
                {
                    gpio_set_level(LED_GPIO, 1); // Liga led interno
                    mqtt_envia_mensagem(MOSQUITTO, "/EMBARCADOS/Sensores/Presenca", mensagem_led); // Manda mensagem para ESP com led
                    mqtt_envia_mensagem(THINGSBOARD, "v1/devices/me/telemetry", mensagem_on); // Manda mensagem para Thingsboard
                    
                    xSemaphoreGive(conexaoMQTTSemaphore);
                    gpio_set_level(LED_GPIO, 0); // Desliga led interno
                    
                    // De-bounce
                    while (gpio_get_level(io_num))
                        vTaskDelay(pdMS_TO_TICKS(100));
                }
                xEventGroupClearBits(sleepEventGroup, PIR_ACTIVE_BIT);
            }
        }
    }
}
