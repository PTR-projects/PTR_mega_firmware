idf_component_register(SRCS "LSM6DSO32_driver.c" "LSM6DSO32_fifo.c"
                    INCLUDE_DIRS "include"
                    REQUIRES SPI_driver BOARD_cfg)

