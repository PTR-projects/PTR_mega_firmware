idf_component_register(SRCS "SPI_driver.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver esp_timer freertos BOARD_cfg)

