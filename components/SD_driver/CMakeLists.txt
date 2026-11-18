idf_component_register(SRCS "SD_driver.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES fatfs sdmmc driver esp_rom BOARD_cfg DataManager SimpleFS_driver)
