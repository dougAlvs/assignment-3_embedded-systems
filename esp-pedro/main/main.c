#include "global.h"
#include "wifi.h"
#include "mqtt.h"
#include "nvs_manager.h"
#include "i2c.h"
#include "display.h"
#include "botao_buzzer.h"

const char *WIFI_TAG = "WIFI";
const char *MQTT_TAG = "MQTT";
const char *NVS_TAG = "NVS";
const char *DISPLAY_TAG = "DISPLAY";
const char *I2C_TAG = "I2C";
const char *BOTAO_TAG = "BOTAO";
const char *BUZZER_TAG = "BUZZER";

const char *ESPACO_ARMAZENAMENTO_NVS = "armazenamento";
const char *CHAVE_NVS = "estado_atual";

modos_de_operacao_t modos[] = {
    {0, "Presenca"},
    {1, "Potenciometro"},
    {2, "Interface"}
};

uint8_t modo_funcionamento_atual = 0;

esp_mqtt_client_handle_t cliente_mqtt;
esp_mqtt_client_handle_t cliente_mqtt_interface;
TaskHandle_t tarefa_publicar_mensagem;
ssd1306_handle_t display_handler;
TaskHandle_t tarefa_atualizar_display;
QueueHandle_t fila_eventos_gpio = NULL;

uint32_t ultimo_tempo_interrupcao = 0;

void app_main(void) {
    inicializar_nvs();

    inicializar_wifi();

    inicializar_mqtt();
    inicializar_mqtt_interface();

    modo_funcionamento_atual = carregar_modo_nvs();

    inicializar_i2c();
    inicializar_display();
    inicializar_buzzer();

    atualizar_display();

    xTaskCreate(publicar_mensagem, "tarefa_publicar_mensagem", 4096, NULL, 5, &tarefa_publicar_mensagem);
    vTaskSuspend(tarefa_publicar_mensagem);

    configurar_interrupcao_botao();
}
