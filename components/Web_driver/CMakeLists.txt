idf_component_register(SRCS "web_driver.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES  nvs_flash esp_http_server spiffs)

