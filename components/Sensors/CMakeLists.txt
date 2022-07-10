idf_component_register(SRCS "Sensors.c"
                    INCLUDE_DIRS "include"
                    REQUIRES MS5607_driver LIS331_driver LSM6DSO32_driver MMC5983MA_driver)

