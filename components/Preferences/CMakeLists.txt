idf_component_register(SRCS "Preferences.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_wifi nvs_flash spiffs json BOARD_cfg)
					 
