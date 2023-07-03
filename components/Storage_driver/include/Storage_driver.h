#pragma once
#include "esp_err.h"
#include "esp_spiffs.h"
#include "esp_littlefs.h"
#include "esp_flash.h"

/*!
 * @brief Enumerator holding types of filesystems used.
 */
typedef enum{
    Storage_filesystem_spiffs = 1, /*!< Spiffs filesystem */
    Storage_filesystem_littlefs = 2/*!< Littlefs filesystem */
} Storage_filesystem_t;

/*!
 * @brief Structure to hold important parameters for `Storage_driver`
 */
typedef struct{
	Storage_filesystem_t Storage_filestystem_d;
	uint32_t MasterKey; /*!<Contains master key used to erase data from partition*/
	size_t minFreeMem;	/*!<Minimum amount of memory in kB needed for program to start*/
	char path[20];		/*!<Path used to write or read files*/
	bool ReadyFlag;		/*!<Boolean value used to check if `Storage_driver` initialized properly. Prevents from unwanted storage access.*/

} Storage_data_t;
 




/*!
 * @brief Initialize storage component by calling init functions for specified filesystem.
 * @param filesystem
 * @param key
 * Master key for unwanted data access prevention.
 * @return `ESP_OK` if initialized
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t Storage_init(Storage_filesystem_t filesys, uint32_t key);

/*!
 * @brief Erase memory by calling filesystem specific function
 * @param key
 * @return `ESP_OK` if initialized
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t Storage_erase(uint32_t key);

/*!
 * @brief Write packet of given size by calling filesystem specific function
 * @param buff
 * Pointer to a buffer
 * @param len
 * Length of buffer in Bytes
 * @return `ESP_OK` if initialized
 * @return `ESP_ERR_NOT_FOUND` if file is not found
 * @return `ESP_FAIL` otherwise
 */
esp_e
esp_err_t Storage_writePacket(void * buf, uint16_t len);

/*!
 * @brief Read whole file by calling filesystem specific function
 * @param buff
 * Pointer to an output buffer
 * @return `ESP_OK` if initialized
 * @return `ESP_ERR_NOT_FOUND` if file is not found
 * @return `ESP_FAIL` otherwise
 */
esp_err_t Storage_readFile(void * buf);

/*!
 * @brief Get amount of free memory space by calling filesystem specific function
 * @return size_t
 *	- Amount of free memory available in `kB`.
 */
size_t Storage_getFreeMem(void);

/*!
 * @brief Retrieve Storage_driver configuration files
 * @return Storage_data_t
 *	- Structure with `Storage_driver` configuration parameters
 */
Storage_data_t Storage_listParams(void);

