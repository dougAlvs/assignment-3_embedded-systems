#include "wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "global.h"


void inicializar_wifi(void) {
    ESP_LOGI(WIFI_TAG, "Inicializando o módulo Wi-Fi...");

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t configuracao_wifi = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&configuracao_wifi);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &tratar_eventos_wifi, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &tratar_eventos_wifi, NULL);

    wifi_config_t configuracao_rede = {
        .sta = {
            .ssid = SSID,
            .password = PASSWORD,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &configuracao_rede);
    esp_wifi_start();

    ESP_LOGI(WIFI_TAG, "Configuração do Wi-Fi concluída.");
}

void tratar_eventos_wifi(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    static int tentativas_reconexao = 0;
    const int max_tentativas_reconexao = 10;
    esp_err_t err;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(WIFI_TAG, "Iniciando conexão com a rede Wi-Fi...");
        err = esp_wifi_connect();
        if (err == ESP_OK) {
            ESP_LOGI(WIFI_TAG, "Conexão Wi-Fi iniciada com sucesso.");
        } else {
            ESP_LOGE(WIFI_TAG, "Falha ao iniciar a conexão Wi-Fi: %s", esp_err_to_name(err));
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(WIFI_TAG, "Desconectado da rede Wi-Fi.");
        if (tentativas_reconexao < max_tentativas_reconexao) {
            tentativas_reconexao++;
            ESP_LOGI(WIFI_TAG, "Tentativa de reconexão %d de %d...", tentativas_reconexao, max_tentativas_reconexao);
            err = esp_wifi_connect();
            if (err == ESP_OK) {
                ESP_LOGI(WIFI_TAG, "Conexão Wi-Fi iniciada com sucesso.");
            } else {
                ESP_LOGE(WIFI_TAG, "Falha ao iniciar a conexão Wi-Fi: %s", esp_err_to_name(err));
            }
            
            vTaskDelay(pdMS_TO_TICKS(3000));

        } else {
            ESP_LOGE(WIFI_TAG, "Falha ao reconectar após %d tentativas. Reiniciando ESP32...", max_tentativas_reconexao);
            esp_restart();
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        tentativas_reconexao = 0;
        ESP_LOGI(WIFI_TAG, "Conectado à rede Wi-Fi. Endereço IP obtido.");
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(WIFI_TAG, "Endereço IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}