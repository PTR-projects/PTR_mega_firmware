idf_component_register(SRCS "Cansat_driver.c"
                    INCLUDE_DIRS "include"
                    REQUIRES AHRS_driver Effector_driver PTR_DataPacket Storage_driver BOARD_cfg)
