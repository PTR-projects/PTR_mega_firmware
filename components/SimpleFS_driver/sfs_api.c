/*
 * esp_partition_api.c
 *
 *  Created on: 7 paź 2023
 *      Author: Bartek
 */
#include "esp_err.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_vfs.h"
#include "esp_flash.h"
#include "sfs_api.h"

#define SFS_PAGE_SIZE 256
#define SFS_CHUNK_SIZE 64
#if (SFS_PAGE_SIZE % SFS_CHUNK_SIZE)
#error "Chunk size is not aligned!"
#endif

const char ESP_SFS_TAG[] = "SFS";

esp_partition_t *partition = NULL;
uint32_t partition_size_B = 0;
uint32_t partition_blocks = 0;
uint32_t partition_block_size_B = 0;

esp_err_t simplefs_api_init(sfs_info_t * partition_info, const char * label){
	if(label == NULL){
		ESP_LOGE(ESP_SFS_TAG, "Storage init - name = NULL");
		return ESP_FAIL;
	}
	partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, label);
	partition_size_B = partition->size;

	partition_info->partition_size_B = partition_size_B;
	partition_info->partition_page_B = SFS_PAGE_SIZE;

	return ESP_OK;
}

esp_err_t IRAM_ATTR simplefs_api_read(uint32_t position, void *buffer, uint32_t size) {
	if(position > partition_size_B){
		ESP_LOGE(ESP_SFS_TAG, "Storage read position out of range");
		return ESP_FAIL;
	}

	if(buffer == NULL){
		ESP_LOGE(ESP_SFS_TAG, "Storage read NULL buffer");
		return ESP_FAIL;
	}

	if((size == 0) || (size > (partition_size_B - position))){
		ESP_LOGE(ESP_SFS_TAG, "Storage read invalid size. Partition size: %i, read ptr: %i, size: %i", partition_size_B, position, size);
		return ESP_FAIL;
	}

	// Low level read
	esp_err_t err = esp_flash_read(partition->flash_chip, buffer, partition->address+position, size);

    if (err) {
    	ESP_LOGE(ESP_SFS_TAG, "Storage read error = %i", err);
        return ESP_FAIL;
    }
    return 0;
}

esp_err_t IRAM_ATTR simplefs_api_prog(uint32_t position, void *buffer, uint32_t size) {
	if(position > partition_size_B){
		ESP_LOGE(ESP_SFS_TAG, "Storage write position out of range");
		return ESP_FAIL;
	}

	if(position % SFS_CHUNK_SIZE){
		ESP_LOGE(ESP_SFS_TAG, "Storage write position not aligned");
		return ESP_FAIL;
	}

	if(buffer == NULL){
		ESP_LOGE(ESP_SFS_TAG, "Storage write NULL buffer");
		return ESP_FAIL;
	}

	if((size == 0) || (size > (partition_size_B - position)) || (size % SFS_CHUNK_SIZE)){
		ESP_LOGE(ESP_SFS_TAG, "Storage write invalid size");
		return ESP_FAIL;
	}

	// Check if buffer is aligned to 32B
	uintptr_t address = (uintptr_t)buffer;
	if (address % 32 != 0) {
		ESP_LOGV(ESP_SFS_TAG, "The buffer is not aligned to a 32-byte boundary.\n");
		//return ESP_FAIL;
	}

	// Low level write
	esp_err_t err = esp_flash_write(partition->flash_chip, buffer, partition->address+position, size);

    if (err) {
    	ESP_LOGE(ESP_SFS_TAG, "Storage write error = %i", err);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t IRAM_ATTR simplefs_api_erase(uint32_t range_end_B) {
    esp_err_t err = ESP_OK;

    if((range_end_B == 0) || (range_end_B > partition_size_B)){
    	range_end_B = partition_size_B;
    }

    uint32_t chunk = 512*1024;	// must be divisible by 4kB (4096B)
    uint32_t N     = 0;

    ESP_LOGI(ESP_SFS_TAG, "Erase progress: %i %%", 0);
    if(chunk < range_end_B){
    	N = range_end_B / chunk;

    	for(uint8_t i=0; i<N; i++){
			uint32_t start = i*chunk;
			esp_partition_erase_range(partition, start, chunk);
			ESP_LOGI(ESP_SFS_TAG, "Erase progress: %i %%", (100*(i+1))/(N+1));
			vTaskDelay(50);
		}
    }

    // Format last chunk and make sure that it is aligned to 4kB
	uint32_t last_chunk		   = 0;
	uint32_t chunk_remainder   = range_end_B % chunk;
	uint32_t aligned_remainder = range_end_B % 4096;
	if((chunk_remainder - aligned_remainder + 4096 + N*chunk) < partition_size_B){
		last_chunk = chunk_remainder - aligned_remainder + 4096;	//Extend erase range
	}
	else {
		last_chunk = chunk_remainder - aligned_remainder;	//Trim erase range
	}

	esp_partition_erase_range(partition, N*chunk, last_chunk);

    ESP_LOGI(ESP_SFS_TAG, "Erase progress: %i %%", 100);

    vTaskDelay(20);

    if (err) {
        ESP_LOGE(ESP_SFS_TAG, "Storage formating error = %i", err);
        return ESP_FAIL;
    }

    ESP_LOGI(ESP_SFS_TAG, "Memory erased successfully");
    return 0;
}
