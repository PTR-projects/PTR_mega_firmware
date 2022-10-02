#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"

#include "Storage_driver.h"

static Storage_data_t Storage_data_d;

esp_vfs_spiffs_conf_t conf_spiffs = {
      .base_path = "/spiffs",
      .partition_label = "spiffs",
      .max_files = 5,
      .format_if_mount_failed = false
    };

esp_vfs_littlefs_conf_t conf_littlefs = {
            .base_path = "/littlefs",
            .partition_label = "littlefs",
            .format_if_mount_failed = false,
            .dont_mount = false,
        };




esp_err_t Storage_init(Storage_filesystem_t filesys , uint32_t key){
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
    char path[18]="/spiffs/Meas00.csv";
    while(stat(path, &st)){
        fileCnt++;

        itoa(fileCnt/10, path+12, 10);
        itoa(fileCnt%10, path+13, 10);
        strcat(path, ".csv");
    }

    Storage_data_d.fileCount = fileCnt;

    size_t freeSpace = Storage_getFreeMem_Spiffs();
    if(freeSpace < Storage_data_d.minFreeMem){
        return ESP_FAIL;
    }
    

    //Add file with index fileCount+1 and write config data
    FILE* f = fopen(path, "w");
    //tutaj ma być funkcja dodająca naglowek pliku typu nazwy kolumn, konfiguracja itd.
    fclose(f);

	return ESP_OK;
}

esp_err_t Storage_init_Littlefs(uint32_t key){
    return ESP_OK;
}




esp_err_t Storage_erase_Spiffs(uint32_t key){
	if(key != Storage_data_d.MasterKey){
		return ESP_FAIL;
	}
        
    return esp_spiffs_format(conf_spiffs.partition_label);
}

esp_err_t Storage_writePacket_Spiffs(void * data, uint16_t length){
	return ESP_OK;

}
esp_err_t Storage_readFile_Spiffs(){
	return ESP_OK;
}


esp_err_t Storage_readAll(){	
	return ESP_OK;
}

size_t Storage_getFreeMem_Spiffs(void){
    size_t total_bytes = 0, used_bytes = 0;

    esp_spiffs_info(conf_spiffs.partition_label,  &total_bytes,  &used_bytes);

	return (total_bytes-used_bytes)/1000;
	
}





