#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/semphr.h"

#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "led_control.h"
#include "timer_control.h"

SemaphoreHandle_t conexaoWifiSemaphore;
SemaphoreHandle_t conexaoMQTTSemaphore;
SemaphoreHandle_t ledSemaphore;

void conectadoWifi(void * params)
{
    while (true)
    {
        // Aguarda a conexão Wi-Fi
        if (xSemaphoreTake(conexaoWifiSemaphore, portMAX_DELAY))
        {
            // Inicializa o MQTT quando conectado ao Wi-Fi
            mqtt_app_start();
        }
    }
}

void trataComunicacaoComServidor(void * params)
{
  if(xSemaphoreTake(conexaoMQTTSemaphore, portMAX_DELAY))
  {
    while(true)
    {
       publish_led_state();
       vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
  }
}

void app_main(void)
{
    // Inicializa o NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Cria os semáforos binários
    conexaoWifiSemaphore = xSemaphoreCreateBinary();
    conexaoMQTTSemaphore = xSemaphoreCreateBinary();
    ledSemaphore = xSemaphoreCreateBinary();

    // Descomente para se necessário limpar da NVS as configs de Wi-Fi
    // clear_wifi_config();

    // Inicializa o Wi-Fi
    wifi_init_sta();

    // Inicializa o controle do LED
    init_led();

    // Inicializa o Timer do DHT
    init_timer(3000000ULL);

    // Cria tarefas FreeRTOS para gerenciar conexão e comunicação
    xTaskCreate(&conectadoWifi,  "Conexão ao MQTT", 4096, NULL, 1, NULL);
    xTaskCreate(&trataComunicacaoComServidor, "Comunicação com Broker", 4096, NULL, 1, NULL);
}
