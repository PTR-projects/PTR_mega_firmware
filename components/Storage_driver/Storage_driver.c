#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "BOARD_cfg.h"
#include "SimpleFS_driver.h"
#include "Storage_driver.h"

static Storage_data_t Storage_data_d;
static const char *TAG = "Storage_driver";

/*!
 * @brief Initialize storage component (SimpleFS).
 * @return `ESP_OK` if initialized
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t Storage_init(void)
{
	esp_err_t ret = ESP_FAIL;

	Storage_data_d.ReadyFlag = false;
	Storage_data_d.MasterKey = CONFIG_KPPTR_MASTERKEY;
	Storage_data_d.minFreeMem = 100;

	strncpy(Storage_data_d.path, "/storage/meas.bin", sizeof(Storage_data_d.path) - 1);
	Storage_data_d.path[sizeof(Storage_data_d.path) - 1] = '\0';

	ret = SimpleFS_init("storage");

	if(ret == ESP_OK){
		Storage_data_d.ReadyFlag = true;
	}
	return ret;
}

/*!
 * @brief Erase measurement memory.
 * Allowed before ReadyFlag — storage stays not-ready until flash is empty.
 * SPI NOR uses full-chip erase; internal flash erases the used range.
 */
esp_err_t Storage_erase(uint32_t key)
{
	if(key != Storage_data_d.MasterKey){
		ESP_LOGE(TAG, "Erase - wrong key");
		return ESP_FAIL;
	}

#if defined(SFS_USE_SPI_FLASH)
	return SimpleFS_formatMemory(SFS_MAGIC_KEY, SFS_FORMAT_ALL);
#else
	return SimpleFS_formatMemory(SFS_MAGIC_KEY, SFS_FORMAT_RANGE);
#endif
}

/*!
 * @brief Append a measurement packet to SimpleFS.
 */
esp_err_t Storage_writePacket(void *buf, uint16_t len)
{
	if(!Storage_data_d.ReadyFlag){
		ESP_LOGE(TAG, "Initialization failed, cannot proceed!");
		return ESP_FAIL;
	}

	return SimpleFS_appendPacket(buf, len);
}

/*!
 * @brief Get free memory available for logging in kB.
 */
size_t Storage_getFreeMem(void)
{
	uint32_t size_B = SimpleFS_getMemorySize();
	if(size_B == 0){
		ESP_LOGE(TAG, "SimpleFS size unavailable");
		return 0;
	}

	uint8_t used_pct = SimpleFS_memoryUsedPercentage();
	if(used_pct > 100){
		used_pct = 100;
	}

	uint32_t free_B = (size_B * (100U - used_pct)) / 100U;
	return (size_t)(free_B / 1024U);
}

/*!
 * @brief Retrieve Storage_driver configuration parameters.
 */
Storage_data_t Storage_listParams(void)
{
	return Storage_data_d;
}

esp_err_t Storage_readMode(void)
{
	return SimpleFS_readMode();
}

esp_err_t Storage_writeMode(void)
{
	return SimpleFS_writeMode();
}

void Storage_resetReadPointer(void)
{
	SimpleFS_resetReadPointer();
}

uint32_t Storage_getFileSize(void)
{
	return SimpleFS_getFileSize();
}

int32_t Storage_readMemory(uint32_t chunk_size, void *buffer)
{
	return SimpleFS_readMemory(chunk_size, buffer);
}
