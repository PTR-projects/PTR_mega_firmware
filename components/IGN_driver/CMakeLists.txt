idf_component_register(SRCS "IGN_driver.c"
                    INCLUDE_DIRS "include"
                    REQUIRES BOARD_cfg
                    PRIV_REQUIRES esp_driver_gpio)

