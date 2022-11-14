idf_component_register(SRCS "DataManager.c"
                    INCLUDE_DIRS "include"
                    REQUIRES BOARD IGN_driver Sensors Servo_driver Analog_driver AHRS_driver FlightStateDetector GNSS_driver)