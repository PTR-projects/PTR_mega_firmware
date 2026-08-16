idf_component_register(SRCS "SD_driver.c"
                    INCLUDE_DIRS "include"
                    REQUIRES fatfs sdmmc driver BOARD_cfg DataManager SimpleFS_driver esp_rom)
