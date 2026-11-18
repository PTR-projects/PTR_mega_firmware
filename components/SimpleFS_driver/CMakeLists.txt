idf_component_register(SRCS "SimpleFS_driver.c" "sfs_api.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES spi_flash esp_partition vfs esp_rom BOARD_cfg SPIFLASH_driver)

