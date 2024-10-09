#include "botao_buzzer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_rom_sys.h"
#include "esp_log.h"
#include "global.h"
#include "display.h"
#include "nvs_manager.h"

extern QueueHandle_t fila_eventos_gpio;
extern uint8_t modo_funcionamento_atual;
extern uint32_t ultimo_tempo_interrupcao;

void IRAM_ATTR tratar_interrupcao_botao(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    uint32_t tempo_atual = xTaskGetTickCountFromISR();

    if ((tempo_atual - ultimo_tempo_interrupcao) * portTICK_PERIOD_MS > DEBOUNCE_TIME_MS) {
        ultimo_tempo_interrupcao = tempo_atual;
        xQueueSendFromISR(fila_eventos_gpio, &gpio_num, NULL);
    }
}

void tarefa_gpio(void* arg) {
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(fila_eventos_gpio, &io_num, portMAX_DELAY)) {
            beepar_buzzer();
            modo_funcionamento_atual = (modo_funcionamento_atual + 1) % 3;
            ESP_LOGI(BOTAO_TAG, "Modo alterado para: %s", modos[modo_funcionamento_atual].nome_modo);
            salvar_modo_nvs(modo_funcionamento_atual);
            atualizar_display();
        }
    }
}

void configurar_interrupcao_botao(void) {
    gpio_set_direction(BOTAO_ALTERAR_MODO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BOTAO_ALTERAR_MODO, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(BOTAO_ALTERAR_MODO, GPIO_INTR_NEGEDGE);

    fila_eventos_gpio = xQueueCreate(10, sizeof(uint32_t));

    gpio_install_isr_service(0);

    gpio_isr_handler_add(BOTAO_ALTERAR_MODO, tratar_interrupcao_botao, (void*) BOTAO_ALTERAR_MODO);

    xTaskCreate(tarefa_gpio, "tarefa_gpio", 2048, NULL, 10, NULL);
}

void inicializar_buzzer(void) {
    ESP_LOGI(BUZZER_TAG, "Inicializando o BUZZER...");

    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000, 
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t ledc_conf = {
        .gpio_num = BUZZER,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0x0,
        .hpoint = 0
    };
    ledc_channel_config(&ledc_conf);
}

void beepar_buzzer(void) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0xFF); 
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    esp_rom_delay_us(100000); 
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}
