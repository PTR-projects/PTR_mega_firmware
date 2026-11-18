idf_component_register(SRCS "LED_driver.c"
                    INCLUDE_DIRS "include"
                    REQUIRES BOARD_cfg
                    PRIV_REQUIRES driver esp_driver_rmt)
