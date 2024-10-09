#include "display.h"
#include "ssd1306.h"
#include "esp_log.h"
#include "global.h"

extern ssd1306_handle_t display_handler;

void inicializar_display(void) {
    ESP_LOGI(DISPLAY_TAG, "Inicializando o display OLED...");

    display_handler = ssd1306_create(I2C_MASTER_NUM, OLED_ADDR);

    if (display_handler == NULL) {
        ESP_LOGE(DISPLAY_TAG, "Erro ao inicializar o display OLED.");
        return;
    }

    ssd1306_clear_screen(display_handler, 0);
    ssd1306_refresh_gram(display_handler);

    ESP_LOGI(DISPLAY_TAG, "Display OLED inicializado com sucesso.");
}

void atualizar_display(void) {
    ssd1306_clear_screen(display_handler, 0);
    
    const char *texto = modos[modo_funcionamento_atual].nome_modo;
    int comprimento_texto = strlen(texto);

    int largura_tela = 128;
    int altura_tela = 64;
    
    int tamanho_fonte = 16;  

    int pos_x = (largura_tela - (comprimento_texto * tamanho_fonte / 2)) / 2;
    
    int pos_y = (altura_tela - tamanho_fonte) / 2;

    ssd1306_draw_string(display_handler, pos_x, pos_y, (const uint8_t *)texto, tamanho_fonte, 1);

    ssd1306_refresh_gram(display_handler);
}
