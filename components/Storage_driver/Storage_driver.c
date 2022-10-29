#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"


#include "Storage_driver.h"

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
//######################################################################################################################
//													INITIALIZING FILES
//######################################################################################################################

esp_err_t Storage_init(Storage_filesystem_t filesys , uint32_t key){

    Storage_data_d.Storage_filestystem_d = filesys;

    if(filesys == 1){
        return Storage_init_Spiffs(key);
    }
    else if(filesys == 2)
    {
        return Storage_init_Littlefs(key);
    }

    return ESP_FAIL;
}

esp_err_t Storage_init_Spiffs(uint32_t key){

    uint8_t fileCnt = 0;
	Storage_data_d.MasterKey = key;

    esp_err_t ret = esp_vfs_spiffs_register(&conf_spiffs);
    if(ret != ESP_OK){
        return ESP_FAIL;
    }

    //Count number of existing measurment files
    struct stat st;
    char path[20];
    sprintf(path, "/spiffs/Meas%02i.csv", fileCnt);

    while(!stat(path, &st)){
        fileCnt++;
        sprintf(path, "/spiffs/Meas%02i.csv", fileCnt);
    }

    Storage_data_d.fileCount = fileCnt;

    size_t freeSpace = Storage_getFreeMem_Spiffs();
    if(freeSpace < Storage_data_d.minFreeMem){
        return ESP_FAIL;
    }

    strncpy(Storage_data_d.path, path, 20);

    //Add file with index fileCount and write config data
    FILE* f = fopen(Storage_data_d.path, "w");
    //tutaj ma być funkcja dodająca naglowek pliku typu nazwy kolumn, konfiguracja itd.
    fclose(f);

	return ESP_OK;
}

esp_err_t Storage_init_Littlefs(uint32_t key){

    uint8_t fileCnt = 0;
	Storage_data_d.MasterKey = key;

	esp_err_t ret = esp_vfs_littlefs_register(&conf_littlefs);
    if(ret != ESP_OK){
        return ESP_FAIL;
    }

    //Count number of existing measurment files
    struct stat st;
    char path[22];
    sprintf(path, "/littlefs/Meas%02i.csv", fileCnt);

    while(!stat(path, &st)){
    	ESP_LOGI(TAG, "File %s exist.", path);
        fileCnt++;
        sprintf(path, "/littlefs/Meas%02i.csv", fileCnt);
    }
    ESP_LOGI(TAG, "File %s created. Count = % i", path, fileCnt);
    Storage_data_d.fileCount = fileCnt;

    size_t freeSpace = Storage_getFreeMem_Littlefs();
    if(freeSpace < Storage_data_d.minFreeMem){
        return ESP_FAIL;
    }

    strncpy(Storage_data_d.path, path, 22);

    //Add file with index fileCount and write config data

    FILE* f = fopen(Storage_data_d.path, "w");
    //tutaj ma być funkcja dodająca naglowek pliku typu nazwy kolumn, konfiguracja itd.
    fclose(f);

	return ESP_OK;
}

//######################################################################################################################
//													CLEARING FILES
//######################################################################################################################

esp_err_t Storage_erase(uint32_t key){

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
		return ESP_FAIL;
	}

    return esp_spiffs_format(conf_spiffs.partition_label);
}

esp_err_t Storage_erase_Littlefs(uint32_t key){
	if(key != Storage_data_d.MasterKey){
		return ESP_FAIL;
	}

    return esp_littlefs_format(conf_littlefs.partition_label);
}


//######################################################################################################################
//													WRITING FILES
//######################################################################################################################

esp_err_t Storage_writePacket(void * buf, uint16_t len){

    if(Storage_data_d.Storage_filestystem_d == 1){
        return Storage_writePacket_Spiffs(buf, len);
    }
    else if(Storage_data_d.Storage_filestystem_d == 2){
    	return Storage_writePacket_Littlefs(buf, len);
    }

	return ESP_FAIL;
}

esp_err_t Storage_writePacket_Spiffs(void * buf, uint16_t len){

    FILE* f = fopen(Storage_data_d.path, "a");

    if(f == NULL){
        return ESP_FAIL;
    }

    fwrite(buf, len, 1, f);
    fclose(f);


	return ESP_OK;
}

esp_err_t Storage_writePacket_Littlefs(void * buf, uint16_t len){
	ESP_LOGI(TAG, "Writing to file %s, ID %i", Storage_data_d.path, Storage_data_d.fileCount);
    FILE* f = fopen(Storage_data_d.path, "a");

    if(f == NULL){
        //ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }

    fwrite(buf, len, 1, f);
    fclose(f);


	return ESP_OK;
}

//######################################################################################################################
//													READING FILES
//######################################################################################################################

esp_err_t Storage_readFile(void * buf){
	if(Storage_data_d.Storage_filestystem_d == 1){
		return Storage_readFile_Spiffs(buf);
	}
	else if(Storage_data_d.Storage_filestystem_d == 2){
		return Storage_readFile_Littlefs(buf);
	}

	return ESP_FAIL;
}


esp_err_t Storage_readFile_Spiffs(void * buf){

    FILE* f = fopen(Storage_data_d.path, "r");

    if(f == NULL){
        return ESP_FAIL;
    }

    fseek(f, 0L, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0L, SEEK_SET);

    fread(buf, size , 1, f);

    fclose(f);

	return ESP_OK;
}

esp_err_t Storage_readFile_Littlefs(void * buf){
	 FILE* f = fopen(Storage_data_d.path, "r");

	 if(f == NULL){
		 return ESP_FAIL;
	 }

	fseek(f, 0L, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0L, SEEK_SET);

	fread(buf, size , 1, f);

	fclose(f);

	return ESP_OK;
}




esp_err_t Storage_readAll(){
	return ESP_OK;
}


//######################################################################################################################
//													GETTING FREE SPACE
//######################################################################################################################

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

size_t Storage_getFreeMem_Spiffs(void){
    size_t total_bytes = 0, used_bytes = 0;

    esp_spiffs_info(conf_spiffs.partition_label,  &total_bytes,  &used_bytes);

	return (total_bytes-used_bytes)/1000;

}


size_t Storage_getFreeMem_Littlefs(void){
    size_t total_bytes = 0, used_bytes = 0;

    esp_littlefs_info(conf_littlefs.partition_label,  &total_bytes,  &used_bytes);

	return (total_bytes-used_bytes)/1000;

}



//######################################################################################################################
//													UTILITY FUNCTIONS
//######################################################################################################################

Storage_data_t Storage_listParams(void){
	return Storage_data_d;
}

esp_err_t Storage_listFiles(void){

	 uint8_t fileCnt = 0;
	 struct stat st;
	 char path[22], filename[12];

	 sprintf(path, "/littlefs/Meas%02i.csv", fileCnt);
	 sprintf(filename, "Meas%02i.csv", fileCnt);


	 while(stat(path, &st) == 0){

		 FILE* f = fopen(path, "r");


		 fseek(f, 0L, SEEK_END);
		 size_t size = ftell(f);
		 fseek(f, 0L, SEEK_SET);
		 files[fileCnt].size = size;


		 //ESP_LOGI(TAG, "Size: %s, %d Bytes", filename, size);

		 strncpy(files[fileCnt].filename, filename, 12);

		 fileCnt++;
		 sprintf(path, "/littlefs/Meas%02i.csv", fileCnt);
		 sprintf(filename, "Meas%02i.csv", fileCnt);

	 }


	 return ESP_OK;

}
