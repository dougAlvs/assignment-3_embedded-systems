#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_mac.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "mqtt.h"

#include "cJSON.h"

#define TAG "MQTT"

#define TOPICO_MODO_ATUAL "/EMBARCADOS/Iluminacao/modo_atual"
#define TOPICO_RPC_TB "v1/devices/me/rpc/request/+"

extern SemaphoreHandle_t conexaoMQTTSemaphore;

esp_mqtt_client_handle_t clientTB;
esp_mqtt_client_handle_t clientMQ;

int modo_movimento = false;

int leu_modo = false;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, (int)event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        xSemaphoreGive(conexaoMQTTSemaphore);
        if (client == clientMQ)
            msg_id = esp_mqtt_client_subscribe(client, TOPICO_MODO_ATUAL, 0);
        else if (client == clientTB)
            msg_id = esp_mqtt_client_subscribe(client, TOPICO_RPC_TB, 0);
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

        if (strncmp(event->topic, TOPICO_MODO_ATUAL, event->topic_len) == 0)
        {
            // Parse da mensagem JSON recebida
            cJSON *json = cJSON_Parse(event->data);
            if (json == NULL)
            {
                ESP_LOGE(TAG, "Erro ao fazer o parse do JSON");
                break;
            }

            cJSON *modo_atual_nome = cJSON_GetObjectItem(json, "modo_atual_nome");
            if (cJSON_IsString(modo_atual_nome))
            {
                ESP_LOGI(TAG, "RECEBIDO");
                leu_modo = true;
                if ((strcmp(modo_atual_nome->valuestring, "Potenciometro") == 0) || (strcmp(modo_atual_nome->valuestring, "Interface") == 0))
                {
                    ESP_LOGI(TAG, "Entrando em modo %s", modo_atual_nome->valuestring);
                    modo_movimento = false;
                }
                else if (strcmp(modo_atual_nome->valuestring, "Presenca") == 0)
                {
                    ESP_LOGI(TAG, "Entrando em modo %s", modo_atual_nome->valuestring);
                    modo_movimento = true;
                }
            }
            else
                ESP_LOGE(TAG, "Erro ao receber o valor de modo_atual_nome");

            cJSON_Delete(json);
        }

        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
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

void mqtt_start()
{
    mqtt_start_client(&clientMQ, "mqtt://164.41.98.24", NULL);
    mqtt_start_client(&clientTB, "mqtt://164.41.98.25", "obBySyvCjz47hm91vJwX");
}

void mqtt_envia_mensagem(int client, char *topico, char *mensagem)
{
    int message_id;
    if (client)
        message_id = esp_mqtt_client_publish(clientTB, topico, mensagem, 0, 1, 0);
    else
        message_id = esp_mqtt_client_publish(clientMQ, topico, mensagem, 0, 1, 0);

    ESP_LOGI(TAG, "Mensagem enviada, ID: %d", message_id);
}

void mqtt_stop()
{
    leu_modo = false;
    esp_mqtt_client_stop(clientTB);
    esp_mqtt_client_destroy(clientTB);
    esp_mqtt_client_stop(clientMQ);
    esp_mqtt_client_destroy(clientMQ);
}