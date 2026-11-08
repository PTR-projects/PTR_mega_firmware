/*
 * sfs_api.c - SimpleFS memory backend (internal flash or external SPI NOR)
 */
#include "esp_err.h"
#include "esp_log.h"
#include "BOARD_cfg.h"
#include "sfs_api.h"

#if defined(SFS_USE_INTERNAL_FLASH)
#include "esp_partition.h"
#include "esp_vfs.h"
#include "esp_flash.h"
#elif defined(SFS_USE_SPI_FLASH)
#include "SPIFLASH_driver.h"
#else
#error "No SimpleFS memory backend selected (SFS_USE_INTERNAL_FLASH / SFS_USE_SPI_FLASH)"
#endif

#define SFS_PAGE_SIZE 256
#define SFS_CHUNK_SIZE 64
#if (SFS_PAGE_SIZE % SFS_CHUNK_SIZE)
#error "Chunk size is not aligned!"
#endif

const char ESP_SFS_TAG[] = "SFS";

static uint32_t partition_size_B = 0;
static void (*erase_done_cb)(void) = NULL;

#if defined(SFS_USE_INTERNAL_FLASH)
static esp_partition_t *partition = NULL;
#endif

void simplefs_api_register_erase_done_cb(void (*cb)(void))
{
	erase_done_cb = cb;
}

#if defined(SFS_USE_SPI_FLASH)
static void sfs_spi_erase_done(void)
{
	if(erase_done_cb != NULL){
		erase_done_cb();
	}
}
#endif

esp_err_t simplefs_api_init(sfs_info_t * partition_info, const char * label)
{
#if defined(SFS_USE_INTERNAL_FLASH)
	if(label == NULL){
		ESP_LOGE(ESP_SFS_TAG, "Storage init - name = NULL");
		return ESP_FAIL;
	}
	partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, label);
	if(partition == NULL){
		ESP_LOGE(ESP_SFS_TAG, "Partition '%s' not found", label);
		return ESP_FAIL;
	}
	partition_size_B = partition->size;
#elif defined(SFS_USE_SPI_FLASH)
	(void)label;
	esp_err_t err = SPIFLASH_init();
	if(err != ESP_OK){
		ESP_LOGE(ESP_SFS_TAG, "SPIFLASH_init failed: %s", esp_err_to_name(err));
		return ESP_FAIL;
	}
	partition_size_B = SPIFLASH_getSize();
	if(partition_size_B == 0){
		ESP_LOGE(ESP_SFS_TAG, "SPIFLASH reported zero size");
		return ESP_FAIL;
	}
#endif

	partition_info->partition_size_B = partition_size_B;
	partition_info->partition_page_B = SFS_PAGE_SIZE;

	ESP_LOGI(ESP_SFS_TAG, "Backend ready, size=%lu B", (unsigned long)partition_size_B);
	return ESP_OK;
}

esp_err_t IRAM_ATTR simplefs_api_read(uint32_t position, void *buffer, uint32_t size)
{
#if defined(SFS_USE_SPI_FLASH)
	if(SPIFLASH_isBusy()){
		ESP_LOGE(ESP_SFS_TAG, "Storage read blocked - erase in progress");
		return ESP_FAIL;
	}
#endif

	if(position > partition_size_B){
		ESP_LOGE(ESP_SFS_TAG, "Storage read position out of range");
		return ESP_FAIL;
	}

	if(buffer == NULL){
		ESP_LOGE(ESP_SFS_TAG, "Storage read NULL buffer");
		return ESP_FAIL;
	}

	if((size == 0) || (size > (partition_size_B - position))){
		ESP_LOGE(ESP_SFS_TAG, "Storage read invalid size. Partition size: %i, read ptr: %i, size: %i",
				(int)partition_size_B, (int)position, (int)size);
		return ESP_FAIL;
	}

#if defined(SFS_USE_INTERNAL_FLASH)
	esp_err_t err = esp_flash_read(partition->flash_chip, buffer, partition->address + position, size);
#elif defined(SFS_USE_SPI_FLASH)
	esp_err_t err = SPIFLASH_read(position, buffer, size);
#endif

	if(err){
		ESP_LOGE(ESP_SFS_TAG, "Storage read error = %i", (int)err);
		return ESP_FAIL;
	}
	return ESP_OK;
}

esp_err_t IRAM_ATTR simplefs_api_prog(uint32_t position, void *buffer, uint32_t size)
{
#if defined(SFS_USE_SPI_FLASH)
	if(SPIFLASH_isBusy()){
		ESP_LOGE(ESP_SFS_TAG, "Storage write blocked - erase in progress");
		return ESP_FAIL;
	}
#endif

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

	uintptr_t address = (uintptr_t)buffer;
	if(address % 32 != 0){
		ESP_LOGV(ESP_SFS_TAG, "The buffer is not aligned to a 32-byte boundary.");
	}

#if defined(SFS_USE_INTERNAL_FLASH)
	esp_err_t err = esp_flash_write(partition->flash_chip, buffer, partition->address + position, size);
#elif defined(SFS_USE_SPI_FLASH)
	esp_err_t err = SPIFLASH_prog(position, buffer, size);
#endif

	if(err){
		ESP_LOGE(ESP_SFS_TAG, "Storage write error = %i", (int)err);
		return ESP_FAIL;
	}
	return ESP_OK;
}

esp_err_t IRAM_ATTR simplefs_api_erase(uint32_t range_end_B)
{
#if defined(SFS_USE_SPI_FLASH)
	(void)range_end_B;
	if(SPIFLASH_isBusy()){
		ESP_LOGE(ESP_SFS_TAG, "Erase already in progress");
		return ESP_FAIL;
	}
	/* Full chip erase is asynchronous; completion invokes erase_done_cb. */
	esp_err_t err = SPIFLASH_eraseAll(sfs_spi_erase_done);
	if(err != ESP_OK){
		ESP_LOGE(ESP_SFS_TAG, "SPIFLASH_eraseAll failed: %s", esp_err_to_name(err));
		return ESP_FAIL;
	}
	return ESP_OK;

#elif defined(SFS_USE_INTERNAL_FLASH)
	esp_err_t err = ESP_OK;

	if((range_end_B == 0) || (range_end_B > partition_size_B)){
		range_end_B = partition_size_B;
	}

	uint32_t chunk = 512 * 1024;	/* must be divisible by 4kB (4096B) */
	uint32_t N = 0;

	ESP_LOGI(ESP_SFS_TAG, "Erase progress: %i %%", 0);
	if(chunk < range_end_B){
		N = range_end_B / chunk;

		for(uint8_t i = 0; i < N; i++){
			uint32_t start = i * chunk;
			esp_partition_erase_range(partition, start, chunk);
			ESP_LOGI(ESP_SFS_TAG, "Erase progress: %i %%", (100 * (i + 1)) / (N + 1));
			vTaskDelay(50);
		}
	}

	uint32_t last_chunk = 0;
	uint32_t chunk_remainder = range_end_B % chunk;
	uint32_t aligned_remainder = range_end_B % 4096;
	if((chunk_remainder - aligned_remainder + 4096 + N * chunk) < partition_size_B){
		last_chunk = chunk_remainder - aligned_remainder + 4096;
	}
	else {
		last_chunk = chunk_remainder - aligned_remainder;
	}

	esp_partition_erase_range(partition, N * chunk, last_chunk);

	ESP_LOGI(ESP_SFS_TAG, "Erase progress: %i %%", 100);
	vTaskDelay(20);

	if(err){
		ESP_LOGE(ESP_SFS_TAG, "Storage formating error = %i", (int)err);
		return ESP_FAIL;
	}

	ESP_LOGI(ESP_SFS_TAG, "Memory erased successfully");
	if(erase_done_cb != NULL){
		erase_done_cb();
	}
	return ESP_OK;
#endif
}
