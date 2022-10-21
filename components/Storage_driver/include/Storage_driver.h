#pragma once
#include "esp_err.h"
#include "esp_spiffs.h"
#include "esp_littlefs.h"
#include "esp_flash.h"

typedef enum{
    Storage_filesystem_spiffs = 1,
    Storage_filesystem_littlefs = 2
} Storage_filesystem_t;


typedef struct{
	Storage_filesystem_t Storage_filestystem_d;
	uint32_t MasterKey;
	uint8_t fileCount;
	size_t minFreeMem;
	char path[22];
} Storage_data_t;

typedef struct{
    char filename[12];
    uint32_t start_pos;
    size_t size; //in kB
} file_stat_t;

file_stat_t files[256];



esp_err_t Storage_init(Storage_filesystem_t filesys, uint32_t key);
esp_err_t Storage_erase(uint32_t key);
esp_err_t Storage_writePacket(void * buf, uint16_t len);
esp_err_t Storage_readFile(void * buf);
size_t Storage_getFreeMem(void);
esp_err_t Storage_readAll();


esp_err_t Storage_init_Spiffs(uint32_t key);
esp_err_t Storage_init_Littlefs(uint32_t key);

esp_err_t Storage_erase_Spiffs(uint32_t key);
esp_err_t Storage_erase_Littlefs(uint32_t key);

esp_err_t Storage_writePacket_Spiffs(void * buf, uint16_t len);
esp_err_t Storage_writePacket_Littlefs(void * buf, uint16_t len);

esp_err_t Storage_readFile_Spiffs(void * buf);
esp_err_t Storage_readFile_Littlefs(void * buf);

size_t Storage_getFreeMem_Spiffs(void);
size_t Storage_getFreeMem_Littlefs(void);

Storage_data_t Storage_listParams(void);
esp_err_t Storage_listFiles(void);



