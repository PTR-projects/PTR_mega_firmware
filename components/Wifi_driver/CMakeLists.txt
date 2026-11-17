idf_component_register(SRCS "Wifi_driver.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver BOARD_cfg nvs_flash Preferences)
