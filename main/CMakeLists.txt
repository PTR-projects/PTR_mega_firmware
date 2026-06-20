idf_component_register(SRC_DIRS "."
                       INCLUDE_DIRS  "."
                       REQUIRED_IDF_TARGETS esp32s3
					   REQUIRES soc nvs_flash driver hal esp_event esp_driver_mcpwm vfs PTR_DataPacket TMTC)