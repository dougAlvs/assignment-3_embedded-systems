#ifndef TIMER_CONTROL_H
#define TIMER_CONTROL_H
#include "driver/timer.h"
#include "led_control.h"

// Função para pausar o contador do Timer do DHT
void stop_timer();
// Função para resetar o contador do Timer do DHT
void reset_timer_counter();
// Função para reiniciar o Timer do DHT
void restart_timer();
// Callback para o Timer do DHT
bool led_callback(void *args);
// Inicialização do Timer do DHT
void init_timer(uint64_t alarm_value);
// Inicia o Timer
void start_timer();

#endif