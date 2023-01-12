#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "Storage_driver.h"

esp_err_t Storage_init_Spiffs			();
esp_err_t Storage_init_Littlefs			();
esp_err_t Storage_initFile				();
esp_err_t Storage_erase_Spiffs			(uint32_t key);
esp_err_t Storage_erase_Littlefs		(uint32_t key);
esp_err_t Storage_writePacket_Spiffs	(void * buf, uint16_t len);
esp_err_t Storage_writePacket_Littlefs	(void * buf, uint16_t len);
esp_err_t Storage_readFile_Spiffs		(void * buf);
esp_err_t Storage_readFile_Littlefs		(void * buf);
size_t    Storage_getFreeMem_Spiffs		(void);
size_t    Storage_getFreeMem_Littlefs	(void);



static Storage_data_t Storage_data_d;

esp_vfs_spiffs_conf_t conf_spiffs = {
      .base_path = "/storage",
      .partition_label = "storage",
      .max_files = 256,
      .format_if_mount_failed = true
    };

esp_vfs_littlefs_conf_t conf_littlefs = {
		.base_path = "/storage",
		.partition_label = "storage",
		.format_if_mount_failed = true
    };

static const char *TAG = "Storage_driver";


/*!
 * @brief Initialize storage component by calling init functions for specified filesystem.
 * @param filesystem
 * @param key
 * Master key for unwanted data access prevention.
 * @return `ESP_OK` if initialized
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t Storage_init(Storage_filesystem_t filesys , uint32_t key){

	esp_err_t ret = ESP_FAIL;

    Storage_data_d.Storage_filestystem_d 	= filesys;
    Storage_data_d.ReadyFlag 				= false;
    Storage_data_d.MasterKey 				= key;
    Storage_data_d.minFreeMem 				= 100;

    strcpy(Storage_data_d.path, "/storage/meas.bin");

    if(filesys == 1){
        ret = Storage_init_Spiffs(key);
    }
    else if(filesys == 2){
        ret = Storage_init_Littlefs(key);
    }

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

/*!
 * @brief Initialize littlefs filesystem and check if the log file file can be created.
 * @return `ESP_OK` if initialized
 * @return `ESP_FAIL` otherwise.
 */
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

/*!
 * @brief Check if new file can be created
 * @return `ESP_OK` if initialized
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t Storage_initFile(){

	esp_err_t ret = ESP_FAIL;
	struct stat st;

	/*!
	 * @note	Keep in mind that `stat()` returns 0 if file is present, -1 otherwise.
	 */
	int8_t FileStatus = stat(Storage_data_d.path, &st);

	if(FileStatus == -1){
		ESP_LOGI(TAG, "File not present, created file successfully");

		FILE* f = fopen(Storage_data_d.path, "w");
		fclose(f);

		ret = ESP_OK;
	}
	else if(st.st_size < 20){
		ESP_LOGW(TAG, "File present but empty.");

		Storage_erase(Storage_data_d.MasterKey);
		FILE* f = fopen(Storage_data_d.path, "w");
		fclose(f);

		ret = ESP_OK;

	}
	else{
		ESP_LOGE(TAG, "File present and not empty!");
		ret = ESP_FAIL;
	}




	return ret;
}



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

	if(Storage_data_d.Storage_filestystem_d == 1){
        return Storage_erase_Spiffs(key);
    }
    else if(Storage_data_d.Storage_filestystem_d == 2){
        return Storage_erase_Littlefs(key);
    }

    return ESP_FAIL;
}



esp_err_t Storage_erase_Spiffs(uint32_t key){
	if(key != Storage_data_d.MasterKey){
		ESP_LOGE(TAG, "Erase - wrong key");
		return ESP_FAIL;
	}

    return esp_spiffs_format(conf_spiffs.partition_label);
}

esp_err_t Storage_erase_Littlefs(uint32_t key){
	if(key != Storage_data_d.MasterKey){
		ESP_LOGE(TAG, "Erase - wrong key");
		return ESP_FAIL;
	}

    return esp_littlefs_format(conf_littlefs.partition_label);
}


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

	if(Storage_data_d.Storage_filestystem_d == 1){
		res = Storage_writePacket_Spiffs(buf, len);
	}
	else if(Storage_data_d.Storage_filestystem_d == 2){
		res = Storage_writePacket_Littlefs(buf, len);
	}

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

/*!
 * @brief Write packet of given size in littlefs
 * @param buff
 * Pointer to a buffer
 * @param len
 * Length of buffer in Bytes
 * @return `ESP_OK` if initialized
 * @return `ESP_ERR_NOT_FOUND` if file is not found
 */
esp_err_t Storage_writePacket_Littlefs(void * buf, uint16_t len){
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

	if(Storage_data_d.Storage_filestystem_d == 1){
		return Storage_readFile_Spiffs(buf);
	}
	else if(Storage_data_d.Storage_filestystem_d == 2){
		return Storage_readFile_Littlefs(buf);
	}

	return ESP_FAIL;
}



/*!
 * @brief Read whole file from spiffs
 * @param buff
 * Pointer to an output buffer
 * @return `ESP_OK` if initialized
 * @return `ESP_ERR_NOT_FOUND` if file is not found
 */
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

/*!
 * @brief Read whole file from littlefs
 * @param buff
 * Pointer to an output buffer
 * @return `ESP_OK` if initialized
 * @return `ESP_ERR_NOT_FOUND` if file is not found
 */
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

    if(Storage_data_d.Storage_filestystem_d == 1){
        return Storage_getFreeMem_Spiffs();
    }
    else if(Storage_data_d.Storage_filestystem_d == 2){
    	return Storage_getFreeMem_Littlefs();
    }

     return ESP_FAIL;
}

/*!
 * @brief Get amount of free memory from spiifs
 * @return Amount of free memory available in `kB`.
 */
size_t Storage_getFreeMem_Spiffs(void){
    size_t total_bytes = 0,
    		used_bytes = 0;

    esp_spiffs_info(conf_spiffs.partition_label,  &total_bytes,  &used_bytes);

	return (total_bytes-used_bytes)/1000;
}

/*!
 * @brief Get amount of free memory from littlefs
 * @return Amount of free memory available in `kB`.
 */
size_t Storage_getFreeMem_Littlefs(void){
    size_t total_bytes = 0,
    		used_bytes = 0;

    esp_littlefs_info(conf_littlefs.partition_label,  &total_bytes,  &used_bytes);

	return (total_bytes-used_bytes)/1000;
}


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


