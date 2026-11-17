idf_component_register(SRCS "BMP581_driver.c"
                    INCLUDE_DIRS "include"
                    REQUIRES SPI_driver BOARD_cfg)
