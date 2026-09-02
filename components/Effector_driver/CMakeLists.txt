idf_component_register(SRCS "Effector_driver.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_timer Servo_driver SBUS_driver IGN_driver)
