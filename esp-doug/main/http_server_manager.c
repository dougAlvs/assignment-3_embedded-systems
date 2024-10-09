#include "esp_http_server.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "wifi_manager.h"

static const char *TAG = "HTTP_Server";

// Função para lidar com a requisição HTTP
esp_err_t http_get_handler(httpd_req_t *req) {
    // Defina os ponteiros para o início e o fim do HTML embutido
    extern const uint8_t page_html_start[] asm("_binary_index_html_start");
    extern const uint8_t page_html_end[] asm("_binary_index_html_end");
    size_t page_html_len = page_html_end - page_html_start;

    httpd_resp_send(req, (const char *)page_html_start, page_html_len);
    return ESP_OK;
}

esp_err_t http_post_handler(httpd_req_t *req) {
    char buf[100];
    int ret;
    if ((ret = httpd_req_recv(req, buf, sizeof(buf) - 1)) <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    buf[ret] = '\0';
    ESP_LOGI(TAG, "Received POST data: %s", buf);

    // Processar os dados recebidos (SSID e senha)
    char ssid[32] = {0};
    char password[64] = {0};

    // Extraindo SSID e senha do buffer (simplificado)
    sscanf(buf, "ssid=%31[^&]&password=%63s", ssid, password);
    ESP_LOGI(TAG, "Parsed SSID: %s, Password: %s", ssid, password);

    if (save_wifi_config(ssid, password) == ESP_OK) {
        // Redirecionar para a página de sucesso com um parâmetro para exibir o popup
        httpd_resp_set_status(req, "303 See Other");
        httpd_resp_set_hdr(req, "Location", "/?success=true");
        httpd_resp_send(req, NULL, 0);
    } else {
        // Redirecionar para a página de erro com um parâmetro para exibir o popup
        httpd_resp_set_status(req, "303 See Other");
        httpd_resp_set_hdr(req, "Location", "/?success=false");
        httpd_resp_send(req, NULL, 0);
    }

    return ESP_OK;
}

esp_err_t http_restart_handler(httpd_req_t *req) {
    // Apenas redireciona para a URL de reinício
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Esperar um pouco antes de reiniciar
    esp_restart();
    return ESP_OK;
}

esp_err_t http_css_handler(httpd_req_t *req) {
    extern const uint8_t styles_css_start[] asm("_binary_styles_css_start");
    extern const uint8_t styles_css_end[] asm("_binary_styles_css_end");
    size_t styles_css_len = styles_css_end - styles_css_start;

    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char *)styles_css_start, styles_css_len);
    return ESP_OK;
}

esp_err_t http_eye_icon_handler(httpd_req_t *req) {
    extern const uint8_t eye_icon_png_start[] asm("_binary_eye_icon_png_start");
    extern const uint8_t eye_icon_png_end[] asm("_binary_eye_icon_png_end");
    size_t image_len = eye_icon_png_end - eye_icon_png_start;

    httpd_resp_set_type(req, "image/png");
    httpd_resp_send(req, (const char *)eye_icon_png_start, image_len);
    return ESP_OK;
}

esp_err_t http_eye_icon_open_handler(httpd_req_t *req) {
    extern const uint8_t eye_icon_open_png_start[] asm("_binary_eye_icon_open_png_start");
    extern const uint8_t eye_icon_open_png_end[] asm("_binary_eye_icon_open_png_end");
    size_t image_len = eye_icon_open_png_end - eye_icon_open_png_start;

    httpd_resp_set_type(req, "image/png");
    httpd_resp_send(req, (const char *)eye_icon_open_png_start, image_len);
    return ESP_OK;
}


void start_webserver() {
    ESP_LOGI(TAG, "Iniciando servidor HTTP");
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.server_port = 80;
    config.ctrl_port = 32768;
    config.max_uri_handlers = 10;

    if (httpd_start(&server, &config) == ESP_OK) {

        // Registrar o manipulador do index
        httpd_uri_t uri_get = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = http_get_handler
        };
        httpd_register_uri_handler(server, &uri_get);

        // Registrar o manipulador para o endpoint de configuração
        httpd_uri_t uri_post = {
            .uri = "/set_wifi",
            .method = HTTP_POST,
            .handler = http_post_handler
        };
        httpd_register_uri_handler(server, &uri_post);

        // Registrar o manipulador do CSS
        httpd_uri_t uri_css = {
            .uri = "/styles.css",
            .method = HTTP_GET,
            .handler = http_css_handler
        };
        httpd_register_uri_handler(server, &uri_css);

        // Registrar o manipulador para o endpoint de reinício
        httpd_uri_t uri_restart = {
            .uri = "/restart",
            .method = HTTP_GET,
            .handler = http_restart_handler
        };
        httpd_register_uri_handler(server, &uri_restart);

        // Registrar os manipuladores para os ícones
        httpd_uri_t uri_eye_icon = {
            .uri = "/eye-icon.png",
            .method = HTTP_GET,
            .handler = http_eye_icon_handler
        };
        httpd_register_uri_handler(server, &uri_eye_icon);

        httpd_uri_t uri_eye_icon_open = {
            .uri = "/eye-icon-open.png",
            .method = HTTP_GET,
            .handler = http_eye_icon_open_handler
        };
        httpd_register_uri_handler(server, &uri_eye_icon_open);
    }
}