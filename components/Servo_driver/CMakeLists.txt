idf_component_register(SRCS "Servo_driver.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_driver_mcpwm esp_driver_gpio BOARD_cfg)

