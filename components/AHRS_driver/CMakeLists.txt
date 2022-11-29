idf_component_register(SRCS "AHRS_driver.c" "KF_AltitudeAscent.c" "quaternion.c"
                    INCLUDE_DIRS "include"
                    REQUIRES Sensors)

