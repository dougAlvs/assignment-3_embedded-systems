#include "led_control.h"

#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"

#define LED_R_PIN (GPIO_NUM_12)
#define LED_G_PIN (GPIO_NUM_14)
#define LED_B_PIN (GPIO_NUM_27)
#define BUTTON_PIN (GPIO_NUM_0)  // GPIO do botão, ex: GPIO 0 (BOOT)

// Frequência e resolução PWM
#define LEDC_FREQ_HZ (5000) 
#define LEDC_RESOLUTION LEDC_TIMER_8_BIT

// Canais PWM
#define LEDC_CHANNEL_R LEDC_CHANNEL_0
#define LEDC_CHANNEL_G LEDC_CHANNEL_1
#define LEDC_CHANNEL_B LEDC_CHANNEL_2

#define LED_ON_THRESHOLD 20

int LED_STATE = 0;

static const char *TAG = "Led_Control";

extern SemaphoreHandle_t ledSemaphore;

void init_led(void) {
    // Configuração do timer PWM
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_RESOLUTION,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = LEDC_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Configuração dos canais PWM
    ledc_channel_config_t ledc_channel_r = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_R,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = LED_R_PIN,
        .duty = 255,
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_r));

    ledc_channel_config_t ledc_channel_g = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_G,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = LED_G_PIN,
        .duty = 255,
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_g));

    ledc_channel_config_t ledc_channel_b = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_B,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = LED_B_PIN,
        .duty = 255,
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_b));

    xSemaphoreGive(ledSemaphore);
}

void set_led_pwm(int duty) {
    if (xSemaphoreTake(ledSemaphore, portMAX_DELAY))
    {
        // ESP_LOGI(TAG, "Setando potencia do LED: %d", duty);
        // Verifica se estado do LED deve estar ligado ou desligado
        LED_STATE = duty >= LED_ON_THRESHOLD;

        // Inverte o valor da potencia pelo comum ser o 3v3
        duty = 255 - duty;


        // ESP_LOGI(TAG, "LED_STATE: %d", LED_STATE);

        // Define o valor do PWM para cada cor
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_R, duty);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_G, duty);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_B, duty);

        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_R);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_G);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_B);
        xSemaphoreGive(ledSemaphore);
    }
}