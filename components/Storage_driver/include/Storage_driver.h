#pragma once
#include "esp_err.h"
#include "esp_spiffs.h"
#include "esp_littlefs.h"
#include "esp_flash.h"

typedef enum{
    Storage_fiilesystem_spiffs = 1,
    Storage_fiilesystem_littlefs = 2
} Storage_filesystem_t;


typedef struct{
	Storage_filesystem_t Storage_filestystem_d;
	uint32_t MasterKey;
	uint8_t fileCount;
	size_t minFreeMem;
} Storage_data_t;



esp_err_t Storage_init(Storage_filesystem_t filesys, uint32_t key);
esp_err_t Storage_erase(uint32_t key);
esp_err_t Storage_writePacket(void * data, uint16_t length);
esp_err_t Storage_readFile();
size_t Storage_getFreeMem(void);
esp_err_t Storage_readAll();


esp_err_t Storage_init_Spiffs(uint32_t key);
esp_err_t Storage_init_Littlefs(uint32_t key);

esp_err_t Storage_erase_Spiffs(uint32_t key);
esp_err_t Storage_erase_Littlefs(uint32_t key);

esp_err_t Storage_writePacket_Spiffs(void * data, uint16_t length);
esp_err_t Storage_writePacket_Littlefs(void * data, uint16_t length);

esp_err_t Storage_readFile_Spiffs();
esp_err_t Storage_readFile_Littlefs();

size_t Storage_getFreeMem_Spiffs(void);
size_t Storage_getFreeMem_Littlefs(void);







