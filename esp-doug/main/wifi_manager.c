#include "wifi_manager.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_mac.h"
#include "http_server_manager.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t s_wifi_event_group;

static int s_retry_num = 0;
extern SemaphoreHandle_t conexaoWifiSemaphore;

static const char *TAG = "WiFi_Manager";

static bool netif_initialized = false;
static bool event_loop_created = false;


static void sta_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
    {
        if (s_retry_num < CONFIG_WIFI_MAXIMUM_RETRY) 
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Tentando conectar ao AP novamente (%d)/%d", s_retry_num, CONFIG_WIFI_MAXIMUM_RETRY);
        } 
        else 
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"Falha ao conectar ao AP");
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Endereço IP recebido: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        xSemaphoreGive(conexaoWifiSemaphore);
    }
}


void load_wifi_config(char* ssid, size_t ssid_len, char* password, size_t password_len) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err == ESP_OK) {
        err = nvs_get_str(nvs_handle, "ssid", ssid, &ssid_len);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Erro ao carregar SSID da NVS: %s", esp_err_to_name(err));
        }
        err = nvs_get_str(nvs_handle, "password", password, &password_len);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Erro ao carregar senha da NVS: %s", esp_err_to_name(err));
        }
        nvs_close(nvs_handle);
        ESP_LOGI(TAG, "Configuração carregada: SSID: %s, Senha: %s", ssid, password);
    } else {
        ESP_LOGE(TAG, "Falha ao carregar configuração WiFi: %s", esp_err_to_name(err));
    }
}

esp_err_t clear_wifi_config() {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle for clearing: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_erase_key(nvs_handle, "ssid");
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Error erasing SSID in NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_erase_key(nvs_handle, "password");
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Error erasing password in NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing erase in NVS: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    return err;
}

static void ap_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) 
    {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "Station " MACSTR " conectado, AID=%d", MAC2STR(event->mac), event->aid);
    } 
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) 
    {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "Station " MACSTR " desconectado, AID=%d", MAC2STR(event->mac), event->aid);
    }
}

void wifi_init_ap(void) {
    // Inicializa o NVS se ainda não estiver inicializado
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Inicializa a rede e o loop de eventos somente se ainda não estiverem inicializados
    if (!netif_initialized) {
        ESP_ERROR_CHECK(esp_netif_init());
        netif_initialized = true;
    }

    // Checa se o loop de eventos padrão já foi criado
    if (!event_loop_created) {
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        event_loop_created = true;
    }

    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &ap_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = CONFIG_ESP_CONFIG_WIFI_SSID,
            .ssid_len = strlen(CONFIG_ESP_CONFIG_WIFI_SSID),
            .password = CONFIG_ESP_CONFIG_WIFI_PASSWORD,
            .max_connection = 2,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    if (strlen(CONFIG_ESP_CONFIG_WIFI_PASSWORD) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi AP iniciado. SSID:%s senha:%s",
             CONFIG_ESP_CONFIG_WIFI_SSID, CONFIG_ESP_CONFIG_WIFI_PASSWORD);

    start_webserver();  // Inicia o servidor HTTP
}



void wifi_init_sta(void) {
    // Certifique-se de inicializar NVS se ainda não estiver inicializado
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    s_wifi_event_group = xEventGroupCreate();

    // Inicializa a rede e o loop de eventos somente se ainda não estiverem inicializados
    if (!netif_initialized) {
        ESP_ERROR_CHECK(esp_netif_init());
        netif_initialized = true;
    }

    // Checa se o loop de eventos padrão já foi criado
    if (!event_loop_created) {
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        event_loop_created = true;
    }

    // ESP_ERROR_CHECK(esp_netif_init());
    // ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &sta_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &sta_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = ""
        },
    };

    // Buffers para armazenar SSID e senha
    char ssid[32] = {0};
    char password[64] = {0};

    // Carregar configurações de Wi-Fi do NVS
    load_wifi_config(ssid, sizeof(ssid), password, sizeof(password));

    if (strlen(ssid) == 0) 
    {
        ESP_LOGI(TAG, "Nenhum SSID salvo encontrado. Iniciando modo AP.");

        // Inicializar o Wi-Fi em modo AP (Access Point)
        wifi_init_ap();
        return;
    }

    ESP_LOGI(TAG, "Configurando Wi-Fi com SSID: %s", ssid);

    // Copia o ssid e a senha dos buffers para a struct de config
    strncpy((char*) wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*) wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finalizado.");

    // Espera até que a conexão seja estabelecida (WIFI_CONNECTED_BIT) ou falhe (WIFI_FAIL_BIT)
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    // Checa o evento que ocorreu
    if (bits & WIFI_CONNECTED_BIT) 
    {
        ESP_LOGI(TAG, "Conectado ao AP de SSID: %s", ssid);
    } 
    else if (bits & WIFI_FAIL_BIT) 
    {
        ESP_LOGI(TAG, "Falha ao conectar ao AP de SSID: %s", ssid);

        // Desligar o modo STA
        ESP_ERROR_CHECK(esp_wifi_stop());
        ESP_ERROR_CHECK(esp_wifi_deinit());

        ESP_LOGI(TAG, "Religando o modo AP da ESP");
        // Iniciar o modo AP
        wifi_init_ap();
    } 
    else 
    {
        ESP_LOGE(TAG, "Evento inesperado");
    }

    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &sta_event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &sta_event_handler));
    vEventGroupDelete(s_wifi_event_group);
}

esp_err_t save_wifi_config(const char* ssid, const char* password) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao abrir o handle NVS: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_str(nvs_handle, "ssid", ssid);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao salvar SSID na NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_set_str(nvs_handle, "password", password);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao salvar senha na NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao cometer NVS: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    return err;
}
