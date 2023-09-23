idf_component_register(SRCS "GNSS_driver.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_event esp_timer BOARD driver)

