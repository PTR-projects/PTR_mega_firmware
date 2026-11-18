idf_component_register(SRCS "Cansat_driver.c"
                    INCLUDE_DIRS "include"
                    REQUIRES AHRS_driver PTR_DataPacket
                    PRIV_REQUIRES Effector_driver Storage_driver BOARD_cfg)
