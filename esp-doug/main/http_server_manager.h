#ifndef HTTP_SERVER_MANAGER_H
#define HTTP_SERVER_MANAGER_H

#include "esp_http_server.h"


esp_err_t http_get_handler(httpd_req_t *req);
esp_err_t http_restart_handler(httpd_req_t *req);
esp_err_t http_css_handler(httpd_req_t *req);
esp_err_t http_eye_icon_handler(httpd_req_t *req);
esp_err_t http_eye_icon_open_handler(httpd_req_t *req);
void start_webserver();

#endif