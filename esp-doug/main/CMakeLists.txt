idf_component_register(SRCS "http_server_manager.c" "main.c" "mqtt_manager.c" "wifi_manager.c" "led_control.c" "timer_control.c"
                    INCLUDE_DIRS "."
                    EMBED_FILES "assets/index.html" "assets/styles.css" "assets/eye-icon.png" "assets/eye-icon-open.png")