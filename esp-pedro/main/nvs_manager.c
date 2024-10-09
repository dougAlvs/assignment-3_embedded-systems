#include "nvs_manager.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "global.h"

void inicializar_nvs(void) {
    esp_err_t ret = nvs_flash_init();
    ESP_LOGI(NVS_TAG, "Inicializando NVS...");

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(NVS_TAG, "NVS sem páginas livres ou versão nova encontrada. Apagando NVS...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_LOGI(NVS_TAG, "NVS apagado com sucesso. Tentando inicializar novamente...");
        ret = nvs_flash_init();
    }

    if (ret == ESP_OK) {
        ESP_LOGI(NVS_TAG, "NVS inicializado com sucesso.");
    } else {
        ESP_LOGE(NVS_TAG, "Erro ao inicializar o NVS: %s", esp_err_to_name(ret));
    }

    ESP_ERROR_CHECK(ret);
}

void salvar_modo_nvs(int modo) {
    nvs_handle handle_nvs;
    ESP_LOGI(NVS_TAG, "Abrindo NVS para escrita...");
    esp_err_t err = nvs_open(ESPACO_ARMAZENAMENTO_NVS, NVS_READWRITE, &handle_nvs);

    if (err == ESP_OK) {
        ESP_LOGI(NVS_TAG, "Escrevendo o modo %d na NVS...", modo);
        err = nvs_set_i32(handle_nvs, CHAVE_NVS, modo);
        if (err == ESP_OK) {
            err = nvs_commit(handle_nvs);
            if (err == ESP_OK) {
                ESP_LOGI(NVS_TAG, "Modo salvo com sucesso na NVS.");
            } else {
                ESP_LOGE(NVS_TAG, "Erro ao salvar o modo na NVS: %s", esp_err_to_name(err));
            }
        } else {
            ESP_LOGE(NVS_TAG, "Erro ao definir o modo na NVS: %s", esp_err_to_name(err));
        }
        nvs_close(handle_nvs);
    } else {
        ESP_LOGE(NVS_TAG, "Erro ao abrir a NVS para escrita: %s", esp_err_to_name(err));
    }
}

uint8_t carregar_modo_nvs(void) {
    nvs_handle handle_nvs;
    int32_t modo = 0;
    ESP_LOGI(NVS_TAG, "Abrindo NVS para leitura...");
    esp_err_t err = nvs_open(ESPACO_ARMAZENAMENTO_NVS, NVS_READONLY, &handle_nvs);

    if (err == ESP_OK) {
        ESP_LOGI(NVS_TAG, "Carregando o modo da NVS...");
        err = nvs_get_i32(handle_nvs, CHAVE_NVS, &modo);
        if (err == ESP_OK) {
            ESP_LOGI(NVS_TAG, "Modo carregado da NVS: %ld", modo);
        } else if (err == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGW(NVS_TAG, "Modo não encontrado na NVS, usando o valor padrão: %ld", modo);
        } else {
            ESP_LOGE(NVS_TAG, "Erro ao carregar o modo da NVS: %s", esp_err_to_name(err));
            modo = 0;
        }
        nvs_close(handle_nvs);
    } else {
        ESP_LOGE(NVS_TAG, "Erro ao abrir a NVS para leitura: %s", esp_err_to_name(err));
    }
    return modo;
}
