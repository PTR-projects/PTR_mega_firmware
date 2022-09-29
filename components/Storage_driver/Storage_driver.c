#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"

#include "Storage_driver.h"

uint32_t MasterKey;

esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };


esp_err_t Storage_init(Storage_filesystem_t FS, uint32_t key){

	MasterKey = key;
	
	esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
           
        } else if (ret == ESP_ERR_NOT_FOUND) {
            
        } else {
           
        }
       
    }
	return ESP_OK;
}
esp_err_t Storage_erase(uint32_t key){
	if(key != MasterKey){
		return ESP_FAIL;
	}
	return ESP_OK;
}

esp_err_t Storage_writePacket(void * data, uint16_t length){
	return ESP_OK;

}
esp_err_t Storage_readFile(){
	return ESP_OK;
}
esp_err_t Storage_readAll(){	
	return ESP_OK;
}

uint16_t Storage_getFreeMem(void){
	return 0;
	
}





