#pragma once
#include "esp_err.h"
#include "esp_spiffs.h"
#include "esp_littlefs.h"
#include "esp_flash.h"

/**
 * @brief Enumerator holding types of filesystems used.
 */
typedef enum{
    Storage_filesystem_spiffs = 1, /*!< Spiffs filesystem */
    Storage_filesystem_littlefs = 2/*!< Littlefs filesystem */
} Storage_filesystem_t;

/**
 * @brief Structure to hold important parameters for `Storage_driver`
 */
typedef struct{
	Storage_filesystem_t Storage_filestystem_d;
	uint32_t MasterKey; /*!<Contains master key used to erase data from partition*/
	size_t minFreeMem;	/*!<Minimum amount of memory in kB needed for program to start*/
	char path[20];		/*!<Path used to write or read files*/
	bool ReadyFlag;		/*!<Boolean value used to check if `Storage_driver` initialized properly. Prevents from unwanted storage access.*/

} Storage_data_t;





/**
 * @brief Initializes the storage subsystem
 * @param[in] filesys The filesystem to use for storage
 * @param[in] key The key to use for secure storage
 * @return ESP_OK if initialization was successful, error code otherwise
 */
esp_err_t Storage_init(Storage_filesystem_t filesys, uint32_t key);

/**
 * @brief Erases the storage subsystem
 * @param[in] key The key to use for secure storage
 * @return ESP_OK if erasure was successful, error code otherwise
 */
esp_err_t Storage_erase(uint32_t key);

/**
 * @brief Writes a packet of data to the storage subsystem
 * @param[in] buf Pointer to the buffer containing the data to write
 * @param[in] len The length of the data in the buffer
 * @return ESP_OK if the write was successful, error code otherwise
 */
esp_err_t Storage_writePacket(void * buf, uint16_t len);

/**
 * @brief Reads a file from the storage subsystem
 * @param[out] buf Pointer to the buffer where the file data will be stored
 * @return ESP_OK if the read was successful, error code otherwise
 */
esp_err_t Storage_readFile(void * buf);

/**
 * @brief Gets the amount of free memory available in the storage subsystem
 * @return The number of bytes of free memory available
 */
size_t Storage_getFreeMem(void);

/**
 * @brief Lists the parameters stored in the storage subsystem
 * @return A structure containing the list of parameters
 */
Storage_data_t Storage_listParams(void);

