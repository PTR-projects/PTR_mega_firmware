idf_component_register(SRCS "Storage_driver.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES BOARD_cfg SimpleFS_driver)
