idf_component_register(SRCS "web_driver.c" "Web_driver_json.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES  nvs_flash esp_http_server spiffs littlefs json
                    #EMBED_FILES "data/index.html" "data/dt.txt" )
                    )

