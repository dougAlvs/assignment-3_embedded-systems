#ifndef GLOBAL_H
#define GLOBAL_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "mqtt_client.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "ssd1306.h"
#include "esp_http_client.h"
#include "time.h"
#include "cJSON.h"
#include <inttypes.h>
#include "esp_pm.h"
#include <time.h>
#include <sys/time.h>
#include "driver/ledc.h"
#include "esp_rom_sys.h"

#define SSID "Pedro S"
#define PASSWORD "991842828"

#define BROKER_URI_COMUNICACAO_INTERNA "ws://test.mosquitto.org:8080"
#define BROKER_INTERFACE_URI "mqtt://164.41.98.25"
#define TOPICO_INTERFACE "v1/devices/me/telemetry"
#define ENDERECO_MODO_ATUAL "/EMBARCADOS/Iluminacao/modo_atual"

#define LED_INTERNO 2
#define BOTAO_ALTERAR_MODO 0
#define BUZZER 13

#define OLED_ADDR 0x3C
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0

#define DEBOUNCE_TIME_MS 500

extern const char *WIFI_TAG;
extern const char *MQTT_TAG;
extern const char *NVS_TAG;
extern const char *DISPLAY_TAG;
extern const char *I2C_TAG;
extern const char *BOTAO_TAG;
extern const char *BUZZER_TAG;
extern const char *ESPACO_ARMAZENAMENTO_NVS;
extern const char *CHAVE_NVS;

typedef struct {
    int numero_modo;
    const char *nome_modo;
} modos_de_operacao_t;

extern modos_de_operacao_t modos[];
extern uint8_t modo_funcionamento_atual;

extern esp_mqtt_client_handle_t cliente_mqtt;
extern esp_mqtt_client_handle_t cliente_mqtt_interface;
extern TaskHandle_t tarefa_publicar_mensagem;
extern ssd1306_handle_t display_handler;
extern TaskHandle_t tarefa_atualizar_display;
extern QueueHandle_t fila_eventos_gpio;

extern uint32_t ultimo_tempo_interrupcao;

#endif // GLOBAL_H
