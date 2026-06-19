idf_component_register(SRCS "Storage_driver.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver BOARD_cfg spiffs SimpleFS_driver)

