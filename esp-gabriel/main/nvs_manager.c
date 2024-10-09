#include "nvs_manager.h"
#include "esp_log.h"
#include "global.h"

void inicializar_nvs(void) {
    ESP_LOGI(NVS_TAG, "Inicializando NVS...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(NVS_TAG, "NVS sem páginas livres ou versão nova encontrada. Apagando NVS...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    if (ret == ESP_OK) {
        ESP_LOGI(NVS_TAG, "NVS inicializado com sucesso.");
    } else {
        ESP_LOGE(NVS_TAG, "Erro ao inicializar o NVS: %s", esp_err_to_name(ret));
    }

    ESP_ERROR_CHECK(ret);
}
