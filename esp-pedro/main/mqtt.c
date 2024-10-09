#include "mqtt.h"
#include "esp_log.h"
#include "global.h" 

extern esp_mqtt_client_handle_t cliente_mqtt;
extern esp_mqtt_client_handle_t cliente_mqtt_interface;
extern TaskHandle_t tarefa_publicar_mensagem;

void inicializar_mqtt(void) {
    ESP_LOGI(MQTT_TAG, "Inicializando o módulo MQTT...");

    esp_mqtt_client_config_t configuracao_mqtt = {
        .broker.address.uri = BROKER_URI_COMUNICACAO_INTERNA,
    };

    cliente_mqtt = esp_mqtt_client_init(&configuracao_mqtt);
    esp_mqtt_client_register_event(cliente_mqtt, ESP_EVENT_ANY_ID, tratar_eventos_mqtt, cliente_mqtt);
    ESP_LOGI(MQTT_TAG, "Conectando ao broker MQTT...");
    esp_mqtt_client_start(cliente_mqtt);

    ESP_LOGI(MQTT_TAG, "Configuração do MQTT concluída.");
}

void inicializar_mqtt_interface(void) {
    ESP_LOGI(MQTT_TAG, "Inicializando o broker MQTT Interface...");

    esp_mqtt_client_config_t configuracao_mqtt_interface = {
        .broker.address.uri = BROKER_INTERFACE_URI,
        .credentials.username = "w3HgzeEGAjga9AT7VNVE",
    };

    cliente_mqtt_interface = esp_mqtt_client_init(&configuracao_mqtt_interface);
    esp_mqtt_client_register_event(cliente_mqtt_interface, ESP_EVENT_ANY_ID, tratar_eventos_mqtt, cliente_mqtt_interface);
    ESP_LOGI(MQTT_TAG, "Conectando ao broker MQTT Interface...");
    esp_mqtt_client_start(cliente_mqtt_interface);

    ESP_LOGI(MQTT_TAG, "Configuração do MQTT Interface concluída.");
}

void tratar_eventos_mqtt(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t evento = (esp_mqtt_event_handle_t) event_data;
    static int tentativas_reconexao = 0;
    const int max_tentativas_reconexao = 10;

    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(MQTT_TAG, "Conectado ao broker MQTT.");
            tentativas_reconexao = 0;

            if (tarefa_publicar_mensagem != NULL) {
                vTaskResume(tarefa_publicar_mensagem); 
            }

            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(MQTT_TAG, "Desconectado do broker MQTT.");
            
            if (tarefa_publicar_mensagem != NULL) {
                vTaskSuspend(tarefa_publicar_mensagem);
            }

            if (tentativas_reconexao < max_tentativas_reconexao) {
                tentativas_reconexao++;
                ESP_LOGI(MQTT_TAG, "Tentativa de reconexão %d de %d...", tentativas_reconexao, max_tentativas_reconexao);
                esp_mqtt_client_reconnect(evento->client);
                vTaskDelay(pdMS_TO_TICKS(3000));
            } else {
                ESP_LOGE(MQTT_TAG, "Máximo de tentativas de reconexão ao MQTT atingido. Reiniciando ESP32...");
                esp_restart();
            }
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(MQTT_TAG, "Inscrito no tópico com sucesso. msg_id=%d", evento->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(MQTT_TAG, "Cancelou a inscrição no tópico com sucesso. msg_id=%d", evento->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(MQTT_TAG, "Mensagem publicada com sucesso. msg_id=%d", evento->msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(MQTT_TAG, "Dados recebidos no tópico: %.*s, mensagem: %.*s", 
                     evento->topic_len, evento->topic, 
                     evento->data_len, evento->data);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(MQTT_TAG, "Erro no MQTT.");
            if (evento->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGE(MQTT_TAG, "Erro de transporte: %s", strerror(evento->error_handle->esp_tls_last_esp_err));
            }
            break;

        default:
            ESP_LOGI(MQTT_TAG, "Evento MQTT não tratado. ID: %" PRIi32, event_id);
            break;
    }
}

void publicar_mensagem(void *pvParameters) {
    gpio_set_direction(LED_INTERNO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_INTERNO, 0);
    
    char json_string[50];
    char json_interface_string[50];

    while (true) {
        gpio_set_level(LED_INTERNO, 1);

        snprintf(json_string, sizeof(json_string), 
                 "{\"modo_atual_nome\": \"%s\"}", 
                 modos[modo_funcionamento_atual].nome_modo);

        ESP_LOGI(MQTT_TAG, "Publicando no broker interno: %s", json_string);
        esp_mqtt_client_publish(cliente_mqtt, ENDERECO_MODO_ATUAL, json_string, 0, 1, 0);

        snprintf(json_interface_string, sizeof(json_interface_string), 
                 "{\"modo_atual_nome\": \"%s\"}", 
                 modos[modo_funcionamento_atual].nome_modo);

        ESP_LOGI(MQTT_TAG, "Publicando no broker Interface: %s", json_interface_string);
        esp_mqtt_client_publish(cliente_mqtt_interface, TOPICO_INTERFACE, json_interface_string, 0, 1, 0);

        gpio_set_level(LED_INTERNO, 0);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}