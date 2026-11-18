idf_component_register(SRC_DIRS "."
                       INCLUDE_DIRS  "."
                       REQUIRED_IDF_TARGETS esp32s3 esp32c3
                       PRIV_REQUIRES soc nvs_flash driver hal esp_event esp_driver_mcpwm vfs esp_timer
                                     CONFIG SPI_driver LED_driver LORA_driver TMTC SX126x_driver
                                     GNSS_driver Analog_driver Storage_driver Sensors AHRS_driver
                                     FlightStateDetector Wifi_driver Web_driver Preferences DataManager
                                     PTR_DataPacket SysMgr Servo_driver SBUS_driver Cansat_driver
                                     Effector_driver BOARD_cfg SD_driver)