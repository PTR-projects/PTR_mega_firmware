idf_component_register(SRCS "Cansat_driver.c"
                    INCLUDE_DIRS "include"
                    REQUIRES AHRS_driver Servo_driver PTR_DataPacket)
