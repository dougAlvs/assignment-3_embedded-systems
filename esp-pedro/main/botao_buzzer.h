#ifndef BOTAO_BUZZER_H
#define BOTAO_BUZZER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

void IRAM_ATTR tratar_interrupcao_botao(void* arg);
void tarefa_gpio(void* arg);
void configurar_interrupcao_botao(void);
void inicializar_buzzer(void);
void beepar_buzzer(void);

#endif // BOTAO_BUZZER_H
