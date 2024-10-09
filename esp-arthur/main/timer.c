#include "driver/timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"

timer_group_t timer_group = TIMER_GROUP_0; // Timer Group
timer_idx_t timer_idx = TIMER_0;           // Timer Index

// Função para pausar o contador do Timer do DHT
void stop_timer()
{
    timer_pause(TIMER_GROUP_0, TIMER_0);
}

// Função para resetar o contador do Timer do DHT
void reset_timer_counter()
{
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL);
}

// Função para reiniciar o Timer do DHT
void restart_timer()
{
    stop_timer();
    reset_timer_counter();
    timer_start(TIMER_GROUP_0, TIMER_0);
}

// Inicialização do Timer do DHT
void init_timer(timer_isr_t callback, uint64_t tempo)
{
    timer_config_t config = {
        .divider = 80, // 80 MHz / 80 = 1 MHz -> 1 tick por microsegundo
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
    };

    timer_init(TIMER_GROUP_0, TIMER_0, &config);
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, tempo);
    timer_isr_callback_add(TIMER_GROUP_0, TIMER_0, callback, NULL, 0);
    timer_start(TIMER_GROUP_0, TIMER_0);
}