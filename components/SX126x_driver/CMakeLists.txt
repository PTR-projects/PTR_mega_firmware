idf_component_register(SRCS "SX126x_driver.c" "sx126x_hal.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES driver BOARD_cfg SPI_driver)