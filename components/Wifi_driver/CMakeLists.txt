idf_component_register(SRCS "Wifi_driver.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_wifi nvs_flash Preferences BOARD_cfg)
