idf_component_register(SRCS "FlightStateDetector.c"
                    INCLUDE_DIRS "include"
                    REQUIRES Effector_driver AHRS_driver Preferences)

