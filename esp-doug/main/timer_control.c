#include "timer_control.h"

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

// Callback para o Timer do DHT
bool led_callback(void *args)
{
    // Quando o timer acabar, desliga o LED
    set_led_pwm(0);
    return true;
}

// Inicialização do Timer do DHT
void init_timer(uint64_t alarm_value)
{
    timer_config_t config = {
        .divider = 80, // 80 MHz / 80 = 1 MHz -> 1 tick por microsegundo
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = true};

    timer_init(TIMER_GROUP_0, TIMER_0, &config);
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, alarm_value);
    timer_isr_callback_add(TIMER_GROUP_0, TIMER_0, led_callback, NULL, 0);
}
