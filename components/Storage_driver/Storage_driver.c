#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"

#include "Storage_driver.h"

static Storage_data_t Storage_data_d;

esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = false
    };


esp_err_t Storage_init(Storage_filesystem_t FS, uint32_t key){

    uint8_t fileCnt = 0;
	Storage_data_d.MasterKey = key;
	
    struct stat st;
    while(stat("/spiffs/foo.txt", &st)){
        fileCnt++;
    }

    Storage_data_d.fileCount = fileCnt;

    


	return ESP_OK;
}

esp_err_t Storage_erase(uint32_t key){
	if(key != Storage_data_d.MasterKey){
		return ESP_FAIL;
	}
	return ESP_OK;
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

uint16_t Storage_getFreeMem_Spiffs(void){
    char partition_label;
    uint16_t total_bytes, used_bytes;

    esp_err_t esp_spiffs_info(partition_label,  total_bytes,  used_bytes);

	return total_bytes-used_bytes;
	
}





