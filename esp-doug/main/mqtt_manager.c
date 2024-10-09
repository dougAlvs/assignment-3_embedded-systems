#include "cJSON.h"
#include "esp_log.h"

#include "mqtt_manager.h"
#include "led_control.h"
#include "timer_control.h"

#define MQ_MQTT_BROKER_URI "mqtt://test.mosquitto.org"
#define TB_MQTT_BROKER_URI "mqtt://164.41.98.25"
#define TB_MQTT_USER "q8EftKGNdKNNjuYbFWGl"

static const char *TAG = "MQTT_Manager";

extern SemaphoreHandle_t conexaoMQTTSemaphore;


esp_mqtt_client_handle_t clientTB;
esp_mqtt_client_handle_t clientMQ;

// Tabela de modos
// {0, "Presenca"}
// {1, "Potenciometro"}
// {2, "Interface"}

char  *CURRENT_MODE = "Interface";


static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, (int) event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        xSemaphoreGive(conexaoMQTTSemaphore);
        if (client == clientMQ)
        {
            ESP_LOGI(TAG, "Connected to Mosquitto Broker");
            // Topico para o potenciometro
            msg_id = esp_mqtt_client_subscribe(client, "/EMBARCADOS/Sensores/Potenciometro", 0);
            ESP_LOGI(TAG, "Sent subscribe successful, msg_id=%d", msg_id);

            // Topico para o sensor de presenca
            msg_id = esp_mqtt_client_subscribe(client, "/EMBARCADOS/Sensores/Presenca", 0);
            ESP_LOGI(TAG, "Sent subscribe successful, msg_id=%d", msg_id);

            // Topico para o modo de operacao
            msg_id = esp_mqtt_client_subscribe(client, "/EMBARCADOS/Iluminacao/modo_atual", 0);
            ESP_LOGI(TAG, "Sent subscribe successful, msg_id=%d", msg_id);
        }
        else if (client == clientTB)
        {
            ESP_LOGI(TAG, "Connected to ThingsBoard Broker");
            msg_id = esp_mqtt_client_subscribe(client, "v1/devices/me/rpc/request/+", 0);
            ESP_LOGI(TAG, "Sent subscribe successful, msg_id=%d", msg_id);
        }
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);

        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        handle_mqtt_message(event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void mqtt_start_client(esp_mqtt_client_handle_t *client, const char *broker_uri, const char *username)
{
    esp_mqtt_client_config_t mqtt_config = {
        .broker.address.uri = broker_uri,
    };

    if (username != NULL)
    {
        mqtt_config.credentials.username = username;
    }

    *client = esp_mqtt_client_init(&mqtt_config);
    esp_mqtt_client_register_event(*client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(*client);
}

void mqtt_app_start(void)
{
    mqtt_start_client(&clientMQ, MQ_MQTT_BROKER_URI, NULL);
    mqtt_start_client(&clientTB, TB_MQTT_BROKER_URI, TB_MQTT_USER);
}

void mqtt_publish_message(const esp_mqtt_client_handle_t client, const char *topic, const char *message) {
    ESP_LOGI(TAG, "Publicando mensagem: '%s' no tópico: '%s'", message, topic);
    // Publica a mensagem no tópico especificado
    esp_mqtt_client_publish(client, topic, message, 0, 1, 0);
}

void publish_led_state() {
    ESP_LOGI(TAG, "Publicando estado do LED: %d", LED_STATE);
    char msg[100];
    sprintf(msg, "{\"led_state\":%d}", LED_STATE);
    mqtt_publish_message(clientTB, "v1/devices/me/attributes", msg);
}

void handle_interface_command(const cJSON *json) {
    cJSON *method = cJSON_GetObjectItemCaseSensitive(json, "method");
    if (cJSON_IsString(method) && (method->valuestring != NULL)) {
        ESP_LOGI(TAG, "Method: %s\n", method->valuestring);
        if(strcmp(method->valuestring, "setLedPwm") == 0){
            cJSON *params = cJSON_GetObjectItemCaseSensitive(json, "params");
            int led_pwm_value = params->valueint;
            if (led_pwm_value == 0)
            {
                ESP_LOGI(TAG, "Recebido comando para desligar o LED");
            }
            else
            {
                ESP_LOGI(TAG, "Recebido comando para ligar o LED em %d", led_pwm_value);
            }
            set_led_pwm(led_pwm_value);
            publish_led_state();
        ESP_LOGI(TAG, "Led_state: %d\n", LED_STATE);
        }
    }
}

void handle_operation_mode_command(const cJSON *json) {
    // Verificação mudanças do modo de operação
    cJSON *mode = cJSON_GetObjectItemCaseSensitive(json, "modo_atual_nome");
    if (cJSON_IsString(mode) && (mode->valuestring != NULL)) {
        ESP_LOGI(TAG, "Received Mode: %s\n", mode->valuestring);

        if (strcmp(mode->valuestring, CURRENT_MODE) == 0)
        {
            return;
        }

        if(strcmp(mode->valuestring, "Presenca") == 0){
            ESP_LOGI(TAG, "Ativando modo Presenca");
            CURRENT_MODE = "Presenca";
        }
        else if(strcmp(mode->valuestring, "Potenciometro") == 0){
            ESP_LOGI(TAG, "Ativando modo Potenciometro");
            CURRENT_MODE = "Potenciometro";
        }
        else if(strcmp(mode->valuestring, "Interface") == 0){
            ESP_LOGI(TAG, "Ativando modo Interface");
            CURRENT_MODE = "Interface";
        }
        stop_timer();
        reset_timer_counter();
    }
}

void handle_potentiometer_command(const cJSON *json) {
    // Verificação valor potenciometro
    cJSON *pot = cJSON_GetObjectItemCaseSensitive(json, "potenciometro");
    if (cJSON_IsNumber(pot)) {
        int pot_pwm_value = pot->valueint;
        ESP_LOGI(TAG, "Recebido valor do potenciômetro: %d\n", pot_pwm_value);
        set_led_pwm(pot_pwm_value);
        publish_led_state();
    }
}

void handle_moviment_command(const cJSON *json) {
    // Verificação estado sensor de movimento
    cJSON *mov = cJSON_GetObjectItemCaseSensitive(json, "movimento");
    if (cJSON_IsBool(mov)) {
        ESP_LOGI(TAG, "Recebido estado do sensor de movimento: %d\n", 255);
        set_led_pwm(255);
        publish_led_state();

        restart_timer();
    }
}

void handle_mqtt_message(const char *data) {
    cJSON *json = cJSON_Parse(data);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Erro no JSON: %s\n", error_ptr);
        }
    }

    // Verificação de qual comando foi recebido  
    if (cJSON_HasObjectItem(json, "modo_atual_nome")) {
        ESP_LOGI(TAG, "Recebido comando do modo de operação");
        handle_operation_mode_command(json);
    }
    else if (cJSON_HasObjectItem(json, "method") && strcmp(CURRENT_MODE, "Interface") == 0) {
        ESP_LOGI(TAG, "Recebido comando da interface");
        handle_interface_command(json);
    }
    else if (cJSON_HasObjectItem(json, "potenciometro") && strcmp(CURRENT_MODE, "Potenciometro") == 0) {
        ESP_LOGI(TAG, "Recebido comando do valor potenciometro");
        handle_potentiometer_command(json);
    }
    else if (cJSON_HasObjectItem(json, "movimento") && strcmp(CURRENT_MODE, "Presenca") == 0) {
        ESP_LOGI(TAG, "Recebido comando do sensor de movimento");
        handle_moviment_command(json);
    }

    cJSON_Delete(json);
}

