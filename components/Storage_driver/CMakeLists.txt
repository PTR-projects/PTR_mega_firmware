idf_component_register(SRCS "Storage_driver.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver spi_flash esp_partition BOARD spiffs joltwallet__littlefs)

