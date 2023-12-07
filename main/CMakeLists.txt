idf_component_register(SRC_DIRS "."
                       INCLUDE_DIRS  "."
                       REQUIRED_IDF_TARGETS esp32s3)
                       

set(STORAGE_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/storage_fs")

if(CONFIG_FS_LITTLEFS)
	littlefs_create_partition_image(storage STORAGE_SRC_DIR)
elseif(CONFIG_FS_SPIFFS)
	if(EXISTS ${STORAGE_SRC_DIR})
		spiffs_create_partition_image(storage ${STORAGE_SRC_DIR} FLASH_IN_PROJECT)
	else()
	    message(FATAL_ERROR "${STORAGE_SRC_DIR} doesn't exit. Please run 'npm run build' in ${STORAGE_SRC_DIR}")
	endif()
endif()

