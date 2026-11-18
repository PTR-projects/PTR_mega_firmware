idf_component_register(SRCS "GNSS_driver.c" "GNSS_nmea.c" "GNSS_ubx.c" "ubx_parser.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_driver_uart esp_timer BOARD_cfg)
