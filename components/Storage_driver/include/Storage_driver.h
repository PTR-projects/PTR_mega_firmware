#pragma once
#include "esp_err.h"
#include "esp_spiffs.h"
#include "esp_littlefs.h"
#include "esp_flash.h"

#define STORAGE_KEY 0xAABBCCDD

/*!
 * @brief Structure to hold important parameters for `Storage_driver`
 */
typedef struct{
	uint32_t MasterKey; /*!<Contains master key used to erase data from partition*/
	size_t minFreeMem;	/*!<Minimum amount of memory in kB needed for program to start*/
	char path[20];		/*!<Path used to write or read files*/
	bool ReadyFlag;		/*!<Boolean value used to check if `Storage_driver` initialized properly. Prevents from unwanted storage access.*/

} Storage_data_t;






esp_err_t Storage_init();
esp_err_t Storage_erase(uint32_t key);
esp_err_t Storage_writePacket(void * buf, uint16_t len);
esp_err_t Storage_readFile(void * buf);
size_t Storage_getFreeMem(void);
esp_err_t Storage_blockMeasFile();
esp_err_t Storage_unblockMeasFile();


Storage_data_t Storage_listParams(void);

