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

extern SemaphoreHandle_t conexaoMQTTSemaphore; // Semaforo para controle de conexão Wi-Fi
extern EventGroupHandle_t sleepEventGroup; // Event Group para controle de sleep

extern int DHT_ACTIVE_BIT;
extern int PIR_ACTIVE_BIT;

extern TaskHandle_t PIR_TaskHandle;

// Tasl para gerenciamento do sleep
void sleep_management_task(void *pvParameters)
{
    while (1)
    {
        if (!modo_movimento)
        {
            EventBits_t bits = xEventGroupWaitBits(sleepEventGroup, PIR_ACTIVE_BIT | DHT_ACTIVE_BIT, pdFALSE, pdFALSE, 0);
            if ((bits & (PIR_ACTIVE_BIT | DHT_ACTIVE_BIT)) == 0)
            {
                // Suspende a task do PIR
                if (PIR_TaskHandle != NULL)
                {
                    vTaskSuspend(PIR_TaskHandle);
                    PIR_task_suspended = pdTRUE;
                }
                else
                    ESP_LOGE("SLEEP", "PIR_TaskHandle é NULL");

                uart_tx_wait_idle(CONFIG_ESP_CONSOLE_UART_NUM); // Espera a UART terminar de transmitir

                mqtt_stop(); // Desconecta do MQTT

                wifi_stop(); // Desconecta do Wi-Fi

                ESP_LOGI("SLEEP", "Dormindo...");

                vTaskDelay(pdMS_TO_TICKS(1000));

                // Reseta o contador do Timer do DHT e liga o timer de sleep
                stop_timer();
                reset_timer_counter();

                esp_sleep_enable_timer_wakeup(4000000); // 4 segundos (pq ele gasta uns 6 ligando o Wi-Fi e MQTT)

                int64_t tempo_antes_de_dormir = esp_timer_get_time(); // Pega o tempo antes de dormir

                // Dorme
                esp_light_sleep_start();

                esp_sleep_wakeup_cause_t causa = esp_sleep_get_wakeup_cause(); // Pega a causa do acordar

                int64_t tempo_apos_acordar = esp_timer_get_time(); // Pega o tempo após acordar

                if (causa == ESP_SLEEP_WAKEUP_TIMER)
                    ESP_LOGI("SLEEP", "Acordou por timer, dormiu por %lld ms", (tempo_apos_acordar - tempo_antes_de_dormir) / 1000);
                else
                    ESP_LOGI("SLEEP", "Acordou por causa desconhecida, dormiu por %lld ms", (tempo_apos_acordar - tempo_antes_de_dormir) / 1000);

                esp_wifi_start(); // Inicializa o Wi-Fi

                // Espera conectar no Wi-Fi
                while (!wifi_is_connected())
                    vTaskDelay(pdMS_TO_TICKS(100));

                ESP_LOGI("SLEEP", "Restaurou Wi-Fi");

                mqtt_start(); // Inicializa o MQTT
                vTaskDelay(pdMS_TO_TICKS(500));
                ESP_LOGI("SLEEP", "Restaurou MQTT");

                xTaskCreate(&dht_task, "dht_task", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL); // Mede a temperatura e umidade

                // Espera ler o modo atual
                for (int i = 0; !leu_modo && i < 50; i++)
                    vTaskDelay(pdMS_TO_TICKS(100));

                if (modo_movimento) 
                    restart_timer(); // Reinicia o timer do DHT
            }
        }
        else
        {
            if (PIR_task_suspended)
            {
                // Volta a task do PIR
                if (PIR_TaskHandle != NULL)
                {
                    vTaskResume(PIR_TaskHandle);
                    PIR_task_suspended = pdFALSE;
                }
                else
                    ESP_LOGE("SLEEP", "PIR_TaskHandle é NULL");
            }
        }
    }
    vTaskDelay(pdMS_TO_TICKS(500));
}