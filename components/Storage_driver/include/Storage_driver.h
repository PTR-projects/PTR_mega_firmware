#pragma once
#include "esp_err.h"
#include "esp_spiffs.h"

typedef enum{
	FILESYSTEM_LITTLEFS = 1,
	FILESYSTEM_SPIFFS = 2
} Storage_filesystem_t; 



esp_err_t Storage_init(Storage_filesystem_t FS, uint32_t key);
esp_err_t Storage_erase(uint32_t key);

esp_err_t Storage_writePacket(void * data, uint16_t length);
esp_err_t Storage_readFile();
esp_err_t Storage_readAll();

uint16_t Storage_getFreeMem(void);



