idf_component_register(SRCS "Preferences.c"
                    INCLUDE_DIRS "include"
                    REQUIRES nvs_flash esp_wifi vfs spiffs joltwallet__littlefs json)
					 
