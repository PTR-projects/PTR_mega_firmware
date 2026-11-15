idf_component_register(SRCS "LED_driver.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver esp_driver_rmt BOARD_cfg)
