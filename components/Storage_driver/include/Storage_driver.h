#pragma once
#include "esp_err.h"
#include "esp_spiffs.h"
#include "esp_littlefs.h"
#include "esp_flash.h"

typedef enum{
	FILESYSTEM_LITTLEFS = 1,
	FILESYSTEM_SPIFFS = 2
} Storage_filesystem_t; 

typedef struct{
	uint32_t MasterKey;
	uint8_t fileCount;
} Storage_data_t;


esp_err_t Storage_init(Storage_filesystem_t FS, uint32_t key);
esp_err_t Storage_erase(uint32_t key);

esp_err_t Storage_writePacket(void * data, uint16_t length);
esp_err_t Storage_readFile();


esp_err_t Storage_writePacket_Spiffs(void * data, uint16_t length);
esp_err_t Storage_readFile_Spiffs();
uint16_t Storage_getFreeMem_Spiffs(void);

esp_err_t Storage_writePacket_Littlefs(void * data, uint16_t length);
esp_err_t Storage_readFile_Littlefs();
uint16_t Storage_getFreeMem_Littlefs(void);

esp_err_t Storage_readAll();





