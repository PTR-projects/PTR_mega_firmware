idf_component_register(SRCS "DataManager.c" "LogSchema.c"
                    INCLUDE_DIRS "include"
                    REQUIRES IGN_driver Sensors Servo_driver Analog_driver AHRS_driver FlightStateDetector GNSS_driver PTR_DataPacket Cansat_driver
                    PRIV_REQUIRES BOARD_cfg CONFIG)