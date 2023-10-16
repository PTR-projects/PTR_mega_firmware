#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_log.h"
#include "SimpleFS_driver.h"
#include "Storage_driver.h"

FILE* f_meas = NULL;
uint8_t meas_file_lock = 0;

esp_err_t Storage_initFile				();

#if defined(CONFIG_FS_SPIFFS)
esp_err_t Storage_init_Spiffs			();
esp_err_t Storage_erase_Spiffs			(uint32_t key);
esp_err_t Storage_writePacket_Spiffs	(void * buf, uint16_t len);
esp_err_t Storage_readFile_Spiffs		(void * buf);
size_t    Storage_getFreeMem_Spiffs		(void);

#elif defined(CONFIG_FS_LITTLEFS)
esp_err_t Storage_init_Littlefs			();
esp_err_t Storage_erase_Littlefs		(uint32_t key);
esp_err_t Storage_writePacket_Littlefs	(void * buf, uint16_t len);
esp_err_t Storage_readFile_Littlefs		(void * buf);
size_t    Storage_getFreeMem_Littlefs	(void);
#endif

static Storage_data_t Storage_data_d;

#if defined(CONFIG_FS_SPIFFS)
esp_vfs_spiffs_conf_t conf_spiffs = {
      .base_path = "/storage",
      .partition_label = "storage",
      .max_files = 256,
      .format_if_mount_failed = true
    };

#elif defined(CONFIG_FS_LITTLEFS)
esp_vfs_littlefs_conf_t conf_littlefs = {
		.base_path = "/storage",
		.partition_label = "storage",
		.format_if_mount_failed = true
    };
#endif

static const char *TAG = "Storage_driver";


/*!
 * @brief Initialize storage component by calling init functions for specified filesystem.
 * @param filesystem
 * @param key
 * Master key for unwanted data access prevention.
 * @return `ESP_OK` if initialized
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t Storage_init(){

	esp_err_t ret = ESP_FAIL;

    Storage_data_d.ReadyFlag 				= false;
    Storage_data_d.MasterKey 				= CONFIG_KPPTR_MASTERKEY;
    Storage_data_d.minFreeMem 				= 100;

    strcpy(Storage_data_d.path, "/storage/meas.bin");

#if defined(CONFIG_FS_SPIFFS)
	ret = Storage_init_Spiffs(STORAGE_KEY);

#elif defined(CONFIG_FS_LITTLEFS)
	ret = Storage_init_Littlefs(STORAGE_KEY);

#elif defined(CONFIG_FS_SIMPLEFS)
	ret = SimpleFS_init("storage");

#else
#error No File System selected!
#endif

    if(ret==ESP_OK){
    	Storage_data_d.ReadyFlag=true;
    }
    return ret;
}

/*!
 * @brief Initialize spiffs filesystem and check if the log file file can be created.
 * @return `ESP_OK` if initialized
 * @return `ESP_FAIL` otherwise.
 */
#if defined(CONFIG_FS_SPIFFS)
esp_err_t Storage_init_Spiffs(){
	esp_err_t ret = ESP_OK;

	if(!esp_spiffs_mounted("storage")){
		ret = esp_vfs_spiffs_register(&conf_spiffs);
		if(ret != ESP_OK){
			return ESP_FAIL;
		}
		ESP_LOGV(TAG, "SPIFFS mounted");
	} else {
		ESP_LOGI(TAG, "SPIFFS already mounted. Skip.");
	}

    ret = Storage_initFile();

    size_t freeSpace = Storage_getFreeMem_Spiffs();

    if(freeSpace < Storage_data_d.minFreeMem){
    	ESP_LOGE(TAG, "Not enough free memory");
    	ret = ESP_FAIL;
    }

	return ret;
}
#endif

/*!
 * @brief Initialize littlefs filesystem and check if the log file file can be created.
 * @return `ESP_OK` if initialized
 * @return `ESP_FAIL` otherwise.
 */
#if defined(CONFIG_FS_LITTLEFS)
esp_err_t Storage_init_Littlefs(){
	esp_err_t ret = ESP_OK;

	if(!esp_littlefs_mounted("storage")){
		ret = esp_vfs_littlefs_register(&conf_littlefs);
		if(ret != ESP_OK){
			return ESP_FAIL;
		}
		ESP_LOGV(TAG, "LittleFS mounted");
	} else {
		ESP_LOGI(TAG, "LittleFS already mounted. Skip.");
	}

    ret = Storage_initFile();

    size_t freeSpace = Storage_getFreeMem_Littlefs();

	if(freeSpace < Storage_data_d.minFreeMem){
		return ESP_FAIL;
	}


	return ret;
}
#endif

/*!
 * @brief Check if new file can be created
 * @return `ESP_OK` if initialized
 * @return `ESP_FAIL` otherwise.
 */
#if defined(CONFIG_FS_SPIFFS) || defined(CONFIG_FS_LITTLEFS)
esp_err_t Storage_initFile(){

	esp_err_t ret = ESP_FAIL;
	struct stat st;

	/*!
	 * @note	Keep in mind that `stat()` returns 0 if file is present, -1 otherwise.
	 */
	int8_t FileStatus = stat(Storage_data_d.path, &st);

	if(FileStatus == -1){
		ESP_LOGI(TAG, "File not present, formating...");
		Storage_erase(Storage_data_d.MasterKey);

		f_meas = fopen(Storage_data_d.path, "a+");
		ESP_LOGI(TAG, "File created successfully");
		//fclose(f_meas);

		ret = ESP_OK;
	}
	else if(st.st_size < 20){
		ESP_LOGW(TAG, "File present but empty, formating...");

		Storage_erase(Storage_data_d.MasterKey);
		f_meas = fopen(Storage_data_d.path, "a+");
		ESP_LOGI(TAG, "File created successfully");
		//fclose(f_meas);

		ret = ESP_OK;
	}
	else{
		ESP_LOGE(TAG, "File present and not empty!");
		ret = ESP_FAIL;
	}

	return ret;
}
#endif


/*!
 * @brief Erase memory by calling filesystem specific function
 * @param key
 * @return `ESP_OK` if initialized
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t Storage_erase(uint32_t key){

	if(!Storage_data_d.ReadyFlag){
		return ESP_FAIL;
	}

#if defined(CONFIG_FS_SPIFFS)
        return Storage_erase_Spiffs(key);

#elif defined(CONFIG_FS_LITTLEFS)
        return Storage_erase_Littlefs(key);

#elif defined(CONFIG_FS_SPIFFS)
    	return SimpleFS_formatMemory(SFS_MAGIC_KEY);
#endif

    return ESP_FAIL;
}


#if defined(CONFIG_FS_SPIFFS)
esp_err_t Storage_erase_Spiffs(uint32_t key){
	if(key != Storage_data_d.MasterKey){
		ESP_LOGE(TAG, "Erase - wrong key");
		return ESP_FAIL;
	}

    return esp_spiffs_format(conf_spiffs.partition_label);
}
#endif

#if defined(CONFIG_FS_LITTLEFS)
esp_err_t Storage_erase_Littlefs(uint32_t key){
	if(key != Storage_data_d.MasterKey){
		ESP_LOGE(TAG, "Erase - wrong key");
		return ESP_FAIL;
	}

    return esp_littlefs_format(conf_littlefs.partition_label);
}
#endif

//######################################################################################################################
//													WRITING FILES
//######################################################################################################################

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
esp_err_t Storage_writePacket(void * buf, uint16_t len){
	esp_err_t res = ESP_FAIL;

	if(!Storage_data_d.ReadyFlag){
		ESP_LOGE(TAG, "Initialization failed, cannot proceed!");
		return ESP_FAIL;
	}

#if defined(CONFIG_FS_SPIFFS)
		res = Storage_writePacket_Spiffs(buf, len);

#elif defined(CONFIG_FS_LITTLEFS)
		res = Storage_writePacket_Littlefs(buf, len);

#elif defined(CONFIG_FS_SIMPLEFS)
		res = SimpleFS_appendPacket(buf, len);
#endif

	return res;
}

/*!
 * @brief Write packet of given size in spiffs
 * @param buff
 * Pointer to a buffer
 * @param len
 * Length of buffer in Bytes
 * @return `ESP_OK` if initialized
 * @return `ESP_ERR_NOT_FOUND` if file is not found
 */
#if defined(CONFIG_FS_SPIFFS)
esp_err_t Storage_writePacket_Spiffs(void * buf, uint16_t len){

    FILE* f = fopen(Storage_data_d.path, "a");

    if(f == NULL){
    	ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_ERR_NOT_FOUND;
    }
	
    if(fwrite(buf, len, 1, f) != 1){
		ESP_LOGE(TAG,"File write failed");
		return ESP_FAIL;
	}
    
	if(fclose(f) != 0){
		ESP_LOGE(TAG,"File write failed");
		return ESP_FAIL;
	}

	return ESP_OK;
}
#endif

/*!
 * @brief Write packet of given size in littlefs
 * @param buff
 * Pointer to a buffer
 * @param len
 * Length of buffer in Bytes
 * @return `ESP_OK` if initialized
 * @return `ESP_ERR_NOT_FOUND` if file is not found
 */
#if defined(CONFIG_FS_LITTLEFS)
esp_err_t Storage_writePacket_Littlefs(void * buf, uint16_t len){
	ESP_RETURN_ON_FALSE(!(len % CONFIG_LITTLEFS_WRITE_SIZE), ESP_ERR_INVALID_SIZE,
			TAG, "Wrong packet size. Must be multiple of %i", CONFIG_LITTLEFS_WRITE_SIZE);

    //f_meas = fopen(Storage_data_d.path, "a+");

	if(meas_file_lock)
		return ESP_OK;

    if(f_meas == NULL){
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_ERR_NOT_FOUND;
    }

    if(fwrite(buf, len, 1, f_meas) != 1){
		ESP_LOGE(TAG,"File write failed");
		return ESP_FAIL;
	}
    
//	if(fclose(f_meas) != 0){
//		ESP_LOGE(TAG,"File write failed");
//		return ESP_FAIL;
//	}

	return ESP_OK;
}
#endif

#if defined(CONFIG_FS_SPIFFS) || defined(CONFIG_FS_LITTLEFS)
esp_err_t Storage_blockMeasFile(){
	meas_file_lock = 1;
	if(fclose(f_meas) != 0){
		ESP_LOGE(TAG,"File write failed");
		return ESP_FAIL;
	}

	return ESP_OK;
}

esp_err_t Storage_unblockMeasFile(){
	f_meas = fopen(Storage_data_d.path, "a+");
	meas_file_lock = 0;

	return ESP_OK;
}
#endif

/*!
 * @brief Read whole file by calling filesystem specific function
 * @param buff
 * Pointer to an output buffer
 * @return `ESP_OK` if initialized
 * @return `ESP_ERR_NOT_FOUND` if file is not found
 * @return `ESP_FAIL` otherwise
 */
esp_err_t Storage_readFile(void * buf){

	if(!Storage_data_d.ReadyFlag){
		ESP_LOGE(TAG, "Initialization failed, cannot proceed!");
		return ESP_FAIL;
	}

#if defined(CONFIG_FS_SPIFFS)
		return Storage_readFile_Spiffs(buf);

#elif defined(CONFIG_FS_SPIFFS)
		return Storage_readFile_Littlefs(buf);

#elif defined(CONFIG_FS_SPIFFS)
		ESP_LOGE(TAG, "Read from SimpleFS not implemented here!");
		return ESP_FAIL;
#endif

	return ESP_FAIL;
}



/*!
 * @brief Read whole file from spiffs
 * @param buff
 * Pointer to an output buffer
 * @return `ESP_OK` if initialized
 * @return `ESP_ERR_NOT_FOUND` if file is not found
 */
#if defined(CONFIG_FS_SPIFFS)
esp_err_t Storage_readFile_Spiffs(void * buf){

    FILE* f = fopen(Storage_data_d.path, "r");

    if(f == NULL){
    	ESP_LOGE(TAG, "Failed to open file for reading");
    	return ESP_ERR_NOT_FOUND;
    }

    fseek(f, 0L, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0L, SEEK_SET);

    fread(buf, size , 1, f);

    fclose(f);

	return ESP_OK;
}
#endif

/*!
 * @brief Read whole file from littlefs
 * @param buff
 * Pointer to an output buffer
 * @return `ESP_OK` if initialized
 * @return `ESP_ERR_NOT_FOUND` if file is not found
 */
#if defined(CONFIG_FS_LITTLEFS)
esp_err_t Storage_readFile_Littlefs(void * buf){
	 FILE* f = fopen(Storage_data_d.path, "r");

	 if(f == NULL){
		 ESP_LOGE(TAG, "Failed to open file for reading");
		 return ESP_ERR_NOT_FOUND;
	 }

	fseek(f, 0L, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0L, SEEK_SET);

	fread(buf, size , 1, f);

	fclose(f);

	return ESP_OK;
}
#endif

//######################################################################################################################
//													GETTING FREE SPACE
//######################################################################################################################

/*!
 * @brief Get amount of free memory space by calling filesystem specific function
 * @return Amount of free memory available in `kB`.
 */
size_t Storage_getFreeMem()
{
	if(!Storage_data_d.ReadyFlag){
		ESP_LOGE(TAG, "Initialization failed, cannot proceed!");
		return ESP_FAIL;
	}

#if defined(CONFIG_FS_SPIFFS)
        return Storage_getFreeMem_Spiffs();

#elif defined(CONFIG_FS_LITTLEFS)
    	return Storage_getFreeMem_Littlefs();

#elif defined(CONFIG_FS_SIMPLEFS)
    	ESP_LOGE(TAG, "getFreeMem not implemented for SimpleFS!");
    	return ESP_FAIL;
#endif

     return ESP_FAIL;
}

/*!
 * @brief Get amount of free memory from spiifs
 * @return Amount of free memory available in `kB`.
 */
#if defined(CONFIG_FS_SPIFFS)
size_t Storage_getFreeMem_Spiffs(void){
    size_t total_bytes = 0,
    		used_bytes = 0;

    esp_spiffs_info(conf_spiffs.partition_label,  &total_bytes,  &used_bytes);

	return (total_bytes-used_bytes)/1024;
}
#endif

/*!
 * @brief Get amount of free memory from littlefs
 * @return Amount of free memory available in `kB`.
 */
#if defined(CONFIG_FS_LITTLEFS)
size_t Storage_getFreeMem_Littlefs(void){
    size_t total_bytes = 0,
    		used_bytes = 0;

    esp_littlefs_info(conf_littlefs.partition_label,  &total_bytes,  &used_bytes);

	return (total_bytes-used_bytes)/1024;
}
#endif

//######################################################################################################################
//													UTILITY FUNCTIONS
//######################################################################################################################



/*!
 * @brief Retrieve Storage_driver configuration files
 * @return Structure with `Storage_driver` configuration parameters
 */
Storage_data_t Storage_listParams(void){
	return Storage_data_d;
}
