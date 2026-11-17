#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#define STORAGE_KEY 0xAABBCCDD

/*!
 * @brief Structure to hold important parameters for `Storage_driver`
 */
typedef struct {
	uint32_t MasterKey; /*!< Master key used to erase measurement memory */
	size_t minFreeMem;  /*!< Minimum free memory in kB (legacy field) */
	char path[20];      /*!< Logical path name for the measurement log */
	bool ReadyFlag;     /*!< True when SimpleFS is ready for logging */
} Storage_data_t;

esp_err_t Storage_init(void);
esp_err_t Storage_erase(uint32_t key);
esp_err_t Storage_writePacket(void *buf, uint16_t len);
size_t Storage_getFreeMem(void);
Storage_data_t Storage_listParams(void);

esp_err_t Storage_readMode(void);
esp_err_t Storage_writeMode(void);
void Storage_resetReadPointer(void);
uint32_t Storage_getFileSize(void);
int32_t Storage_readMemory(uint32_t chunk_size, void *buffer);
