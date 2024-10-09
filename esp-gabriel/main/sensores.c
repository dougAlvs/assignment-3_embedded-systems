#include "sensores.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "global.h" 
#include "esp_log.h"
#include "mqtt_client.h"
#include <stdio.h>

extern esp_adc_cal_characteristics_t adc1_chars;
extern esp_mqtt_client_handle_t cliente_mqtt;
extern esp_mqtt_client_handle_t cliente_mqtt_interface;

long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - out_min) + out_min;
}

void configurar_leitura_analogica(void) {
    // Caracterização do ADC para melhorar as leituras
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_DEFAULT, 0, &adc1_chars);

    // Configuração dos ADCs
    adc1_config_width(ADC_WIDTH_BIT_DEFAULT);
    adc1_config_channel_atten(POTENTIOMETER_CHANNEL, ADC_ATTEN_DB_11);  // Atenuação de 11dB
    adc1_config_channel_atten(LDR_CHANNEL, ADC_ATTEN_DB_11);  // Atenuação de 11dB
}

int ler_potenciometro(void) {
    int potSum = 0;
    for (int i = 0; i < 1000; i++) {
        int raw_value = adc1_get_raw(POTENTIOMETER_CHANNEL);
        potSum += raw_value;
    }
    int potAverage = potSum / 1000;
    return map(potAverage, 0, 4095, 0, 100);
}

bool ler_ldr(void) {
    int ldrValue = adc1_get_raw(LDR_CHANNEL);

    if (ldrValue > 2000) {
        return false;
    } else {
        return true;
    }
}

void potenciometro_task(void *pvParameters) {
    char mensagem[30];

    while (1) {
        int potenciometro = ler_potenciometro();
        
        snprintf(mensagem, sizeof(mensagem), "{\"potenciometro\": %d}", (int) map(potenciometro, 0, 100, 0, 255));
        
        printf("%s\n", mensagem);

        esp_mqtt_client_publish(cliente_mqtt, TOPICO_POTENCIOMETRO, mensagem, 0, 1, 0);

        snprintf(mensagem, sizeof(mensagem), "{\"potenciometro\": %d}", potenciometro);

        printf("%s\n", mensagem);

        esp_mqtt_client_publish(cliente_mqtt_interface, TOPICO_INTERFACE, mensagem, 0, 1, 0);

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void ldr_task(void *pvParameters) {
    char mensagem[25];

    while (1) {
        bool ldr = ler_ldr();

        snprintf(mensagem, sizeof(mensagem), "{\"ldr\": %s}", ldr ? "true" : "false");
        //printf("%s\n", mensagem);
        esp_mqtt_client_publish(cliente_mqtt_interface, TOPICO_INTERFACE, mensagem, 0, 1, 0);

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}