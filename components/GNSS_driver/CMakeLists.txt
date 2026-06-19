idf_component_register(SRCS "GNSS_driver.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_driver_uart esp_timer BOARD_cfg esp_event)

