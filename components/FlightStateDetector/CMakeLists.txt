idf_component_register(SRCS "FlightStateDetector.c"
                    INCLUDE_DIRS "include"
                    REQUIRES AHRS_driver
                    PRIV_REQUIRES Effector_driver Preferences)

