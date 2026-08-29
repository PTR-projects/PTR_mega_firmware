idf_component_register(SRCS "Guidance.c"
                    INCLUDE_DIRS "include"
                    REQUIRES Algorithms AHRS_driver Effector_driver FlightStateDetector Sensors)
