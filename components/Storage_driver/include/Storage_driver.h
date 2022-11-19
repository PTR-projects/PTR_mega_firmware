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
	char path[20];
	bool flag;

} Storage_data_t;






esp_err_t Storage_init(Storage_filesystem_t filesys, uint32_t key);
esp_err_t Storage_erase(uint32_t key);
esp_err_t Storage_writePacket(void * buf, uint16_t len);
esp_err_t Storage_readFile(void * buf);
size_t Storage_getFreeMem(void);


Storage_data_t Storage_listParams(void);

