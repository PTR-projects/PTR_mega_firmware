idf_component_register(SRCS "SPIFLASH_driver.c"
                    INCLUDE_DIRS "include"
                    REQUIRES SPI_driver BOARD_cfg)
