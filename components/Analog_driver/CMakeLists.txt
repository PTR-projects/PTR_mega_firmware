idf_component_register(SRCS "Analog_driver.c"
                       INCLUDE_DIRS "include"
                       REQUIRES BOARD_cfg
                       PRIV_REQUIRES soc esp_adc esp_driver_tsens driver)
