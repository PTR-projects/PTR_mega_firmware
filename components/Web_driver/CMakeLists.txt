idf_component_register(SRCS "Web_driver.c" "Web_driver.c" "Web_driver_json.c" "Web_driver_cmd.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES  nvs_flash esp_http_server spiffs json IGN_driver Preferences DataManager Storage_driver SimpleFS_driver AHRS_driver FlightStateDetector  BOARD_cfg
                    )


set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/www")
if(EXISTS ${WEB_SRC_DIR})
	spiffs_create_partition_image(www ${WEB_SRC_DIR} FLASH_IN_PROJECT)
else()
    message(FATAL_ERROR "${WEB_SRC_DIR} doesn't exit. Please run 'npm run build' in ${WEB_SRC_DIR}")
endif()
