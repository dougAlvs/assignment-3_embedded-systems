#include "i2c.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "global.h"

void inicializar_i2c(void) {
    ESP_LOGI(I2C_TAG, "Inicializando o I2C...");

    i2c_config_t configuracao = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &configuracao);
    if (err == ESP_OK) {
        ESP_LOGI(I2C_TAG, "Configuração do I2C aplicada com sucesso.");
    } else {
        ESP_LOGE(I2C_TAG, "Erro ao configurar o I2C: %s", esp_err_to_name(err));
    }

    err = i2c_driver_install(I2C_MASTER_NUM, configuracao.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    if (err == ESP_OK) {
        ESP_LOGI(I2C_TAG, "Driver I2C instalado com sucesso.");
    } else {
        ESP_LOGE(I2C_TAG, "Erro ao instalar o driver I2C: %s", esp_err_to_name(err));
    }
}
