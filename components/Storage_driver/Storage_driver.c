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
esp_err_t Storage_createNextFile		();
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
      .base_path = "/spiffs",
      .partition_label = "storage",
      .max_files = 256,
      .format_if_mount_failed = true
    };

esp_vfs_littlefs_conf_t conf_littlefs = {
		.base_path = "/littlefs",
		.partition_label = "storage",
		.format_if_mount_failed = true
    };

static const char *TAG = "Storage_driver";


/*!
 * @brief Initialize storage component by calling init functions for specified filesystem.
 * @param filesystem
 * @param key
 * @return Esp_err_t.
 */
esp_err_t Storage_init(Storage_filesystem_t filesys , uint32_t key){

	esp_err_t ret=ESP_FAIL;
    Storage_data_d.Storage_filestystem_d = filesys;
    Storage_data_d.flag=false;
    Storage_data_d.MasterKey = key;

    if(filesys == 1){
        ret = Storage_init_Spiffs(key);
    }
    else if(filesys == 2){
        ret = Storage_init_Littlefs(key);
    }


    if(ret==ESP_OK){
    	Storage_data_d.flag=true;
    }
    return ret;
}

/*!
 * @brief Initialize spiffs filesystem and check if the log file file can be created.
 * @return Esp_err_t.
 */
esp_err_t Storage_init_Spiffs(){
	strcpy(Storage_data_d.path, "/spiffs/meas.bin");

    esp_err_t ret = esp_vfs_spiffs_register(&conf_spiffs);
    if(ret != ESP_OK){
        return ESP_FAIL;
    }

    ret = Storage_createNextFile();

    size_t freeSpace = Storage_getFreeMem_Spiffs();

    if(freeSpace < Storage_data_d.minFreeMem){
    	ret = ESP_FAIL;
    }


	return ret;
}

/*!
 * @brief Initialize littlefs filesystem and check if the log file file can be created.
 * @return Esp_err_t.
 */
esp_err_t Storage_init_Littlefs(){

	strcpy(Storage_data_d.path, "/littlefs/meas.bin");

	esp_err_t ret = esp_vfs_littlefs_register(&conf_littlefs);
    if(ret != ESP_OK){
        return ESP_FAIL;
    }

    ret = Storage_createNextFile();

    size_t freeSpace = Storage_getFreeMem_Littlefs();

	if(freeSpace < Storage_data_d.minFreeMem){
		return ESP_FAIL;
	}


	return ret;
}

/*!
 * @brief Check if new file can be created
 * @return Esp_err_t.
 */
esp_err_t Storage_createNextFile(){

	struct stat st;


	if(stat(Storage_data_d.path, &st)){
		FILE* f = fopen(Storage_data_d.path, "w");
		ESP_LOGE(TAG, "SS: %ld", st.st_size);
		fclose(f);
	}
	else if(!stat(Storage_data_d.path, &st) && st.st_size>20){//do sprawdzenia wartoś pamięci minimalnej
		ESP_LOGE(TAG, "Szie: %ld", st.st_size);
		return ESP_FAIL;
	}
	else{
		ESP_LOGE(TAG, "ie: %ld", st.st_size);
		Storage_erase(Storage_data_d.MasterKey);
		FILE* f = fopen(Storage_data_d.path, "w");
		ESP_LOGI(TAG, "SS: %ld", st.st_size);
		fclose(f);
	}




	return ESP_OK;
}
//######################################################################################################################
//													CLEARING FILES
//######################################################################################################################

/*!
 * @brief Erase memory by calling filesystem specific function
 * @param key
 * @return Esp_err_t.
 */
esp_err_t Storage_erase(uint32_t key){

	if(!Storage_data_d.flag){
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
 * @param buff pointer to a buffer
 * @param len length of buffer in Bytes
 * @return Esp_err_t.
 */
esp_err_t Storage_writePacket(void * buf, uint16_t len){

	esp_err_t res = ESP_FAIL;

	if(!Storage_data_d.flag){
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
 * @param buff pointer to a buffer
 * @param len length of buffer in Bytes
 * @return Esp_err_t.
 */
esp_err_t Storage_writePacket_Spiffs(void * buf, uint16_t len){

    FILE* f = fopen(Storage_data_d.path, "a");

    if(f == NULL){
        return ESP_FAIL;
    }

    fwrite(buf, len, 1, f);
    fclose(f);


	return ESP_OK;
}

/*!
 * @brief Write packet of given size in littlefs
 * @param buff pointer to a buffer
 * @param len length of buffer in Bytes
 * @return Esp_err_t.
 */
esp_err_t Storage_writePacket_Littlefs(void * buf, uint16_t len){
    FILE* f = fopen(Storage_data_d.path, "a");

    if(f == NULL){
        //ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }

    fwrite(buf, len, 1, f);
    fclose(f);


	return ESP_OK;
}


/*!
 * @brief Read whole file by calling filesystem specific function
 * @param buff pointer to an output buffer
 * @return Esp_err_t.
 */
esp_err_t Storage_readFile(void * buf){

	if(!Storage_data_d.flag){
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
 * @param buff pointer to an output buffer
 * @return Esp_err_t.
 */
esp_err_t Storage_readFile_Spiffs(void * buf){

    FILE* f = fopen(Storage_data_d.path, "r");

    if(f == NULL){
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
 * @param buff pointer to an output buffer
 * @return Esp_err_t.
 */
esp_err_t Storage_readFile_Littlefs(void * buf){
	 FILE* f = fopen(Storage_data_d.path, "r");

	 if(f == NULL){
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
 * @return size_t amount of free memory available
 */
size_t Storage_getFreeMem()
{
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
 * @return size_t amount of free memory available
 */
size_t Storage_getFreeMem_Spiffs(void){
    size_t total_bytes = 0, used_bytes = 0;

    esp_spiffs_info(conf_spiffs.partition_label,  &total_bytes,  &used_bytes);

	return (total_bytes-used_bytes)/1000;
}

/*!
 * @brief Get amount of free memory from littlefs
 * @return size_t amount of free memory available
 */
size_t Storage_getFreeMem_Littlefs(void){
    size_t total_bytes = 0, used_bytes = 0;

    esp_littlefs_info(conf_littlefs.partition_label,  &total_bytes,  &used_bytes);

	return (total_bytes-used_bytes)/1000;
}


//######################################################################################################################
//													UTILITY FUNCTIONS
//######################################################################################################################



/*!
 * @brief Retrieve Storage_driver configuration files
 * @return storage_data_t struct with configuration parameters
 */
Storage_data_t Storage_listParams(void){
	return Storage_data_d;
}


