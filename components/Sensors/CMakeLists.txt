idf_component_register(SRCS "Sensors.c"
                    INCLUDE_DIRS "include"
                    REQUIRES LIS331_driver LSM6DSO32_driver MMC5983MA_driver MS5607_driver
                    PRIV_REQUIRES BMP581_driver BOARD_cfg)

