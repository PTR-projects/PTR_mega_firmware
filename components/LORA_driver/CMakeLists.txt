idf_component_register(SRCS "LORA_driver.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES SX126x_driver Preferences CONFIG)

