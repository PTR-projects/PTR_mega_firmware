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

const char ESP_SIMPLEFS_TAG[] = "simplefs";

esp_partition_t *partition = NULL;
uint32_t partition_size_B = 0;
uint32_t partition_blocks = 0;
uint32_t partition_block_size_B = 0;

esp_err_t simplefs_api_init(sfs_info_t * partition_info, const char * label){
	if(label == NULL){
		ESP_LOGE(ESP_SIMPLEFS_TAG, "Storage init - name = NULL");
		return ESP_FAIL;
	}
	partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, label);
	partition_size_B = partition->size;

	partition_info->partition_size_B = partition_size_B;
	partition_info->partition_page_B = SFS_PAGE_SIZE;

	return ESP_OK;
}

esp_err_t simplefs_api_read(uint32_t position, void *buffer, uint32_t size) {
	if(position > partition_size_B){
		ESP_LOGE(ESP_SIMPLEFS_TAG, "Storage read position out of range");
		return ESP_FAIL;
	}

	if(buffer == NULL){
		ESP_LOGE(ESP_SIMPLEFS_TAG, "Storage read NULL buffer");
		return ESP_FAIL;
	}

	if((size == 0) || (size > (partition_size_B - position))){
		ESP_LOGE(ESP_SIMPLEFS_TAG, "Storage read invalid size");
		return ESP_FAIL;
	}

	// Low level read
	esp_err_t err = esp_flash_read(partition->flash_chip, buffer, position, size);

    if (err) {
    	ESP_LOGE(ESP_SIMPLEFS_TAG, "Storage read error = %i", err);
        return ESP_FAIL;
    }
    return 0;
}

esp_err_t simplefs_api_prog(uint32_t position, void *buffer, uint32_t size) {
	if(position > partition_size_B){
		ESP_LOGE(ESP_SIMPLEFS_TAG, "Storage write position out of range");
		return ESP_FAIL;
	}

	if(position % SFS_CHUNK_SIZE){
		ESP_LOGE(ESP_SIMPLEFS_TAG, "Storage write position not aligned");
		return ESP_FAIL;
	}

	if(buffer == NULL){
		ESP_LOGE(ESP_SIMPLEFS_TAG, "Storage write NULL buffer");
		return ESP_FAIL;
	}

	if((size == 0) || (size > (partition_size_B - position)) || (size % SFS_CHUNK_SIZE)){
		ESP_LOGE(ESP_SIMPLEFS_TAG, "Storage write invalid size");
		return ESP_FAIL;
	}

	// Check if buffer is aligned to 32B
	uintptr_t address = (uintptr_t)buffer;
	if (address % 32 != 0) {
		ESP_LOGE(ESP_SIMPLEFS_TAG, "The buffer is aligned to a 32-byte boundary.\n");
		return ESP_FAIL;
	}

	// Low level write
	esp_err_t err = esp_flash_write(partition->flash_chip, buffer, position, size);

    if (err) {
    	ESP_LOGE(ESP_SIMPLEFS_TAG, "Storage write error = %i", err);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t simplefs_api_erase() {
    esp_err_t err = esp_partition_erase_range(partition, 0, partition_size_B);

    if (err) {
        ESP_LOGE(ESP_SIMPLEFS_TAG, "Storage formating error = %i", err);
        return ESP_FAIL;
    }
    return 0;

}
