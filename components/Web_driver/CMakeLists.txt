idf_component_register(SRCS "Web_driver.c" "Web_driver_json.c" "Web_driver_cmd.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES  nvs_flash esp_http_server spiffs json IGN_driver Preferences DataManager Storage_driver SimpleFS_driver AHRS_driver FlightStateDetector
                    )


set(MINIFY_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/helpers/minify_www.py)
set(WWW_SRC       ${CMAKE_CURRENT_SOURCE_DIR}/www_uncompressed)
set(WWW_DST       ${CMAKE_CURRENT_SOURCE_DIR}/www)

idf_build_get_property(python PYTHON)

message(WARNING "Python executable: ${python} ${MINIFY_SCRIPT}")

message(STATUS "Web assets minifing...")
execute_process(
    COMMAND           ${python} ${MINIFY_SCRIPT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE   MINIFY_RESULT
    TIMEOUT           30
)

if(NOT MINIFY_RESULT EQUAL 0)
    message(WARNING "minify_www.py failed (exit code ${MINIFY_RESULT})")
    message(WARNING "stdout: ${MINIFY_OUTPUT}")
    message(WARNING "stderr: ${MINIFY_ERROR}")
else()
    message(STATUS "Web assets minified OK")
endif()

if(EXISTS ${WWW_DST})
    spiffs_create_partition_image(www ${WWW_DST} FLASH_IN_PROJECT)
else()
    message(FATAL_ERROR "${WWW_DST} doesn't exist.")
endif()
