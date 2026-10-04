#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"
#include "BOARD_cfg.h"
#include "SPI_driver.h"
#include "SPIFLASH_driver.h"

static const char *TAG = "SPIFLASH";

#if defined(SFS_USE_SPI_FLASH)

#define SPIFLASH_CMD_WREN   0x06
#define SPIFLASH_CMD_RDSR   0x05
#define SPIFLASH_CMD_READ   0x03
#define SPIFLASH_CMD_PP     0x02
#define SPIFLASH_CMD_RDID   0x9F
#define SPIFLASH_CMD_CE     0xC7
#define SPIFLASH_CMD_EN4B   0xB7

#define SPIFLASH_SR_WIP     0x01
#define SPIFLASH_PAGE_SIZE  256U
#define SPIFLASH_XFER_MAX   256U
#define SPIFLASH_16MB       (16U * 1024U * 1024U)
#define SPIFLASH_ERASE_POLL_MS 100
#define SPIFLASH_ERASE_TIMEOUT_MS (5U * 60U * 1000U)

static spi_dev_handle_t spi_dev = NULL;
static uint32_t flash_size_B = 0;
static uint8_t addr_bytes = 3;
static volatile bool busy = false;
static bool init_done = false;
static SPIFLASH_done_cb_t erase_done_cb = NULL;

static esp_err_t spiflash_cmd(uint8_t cmd);
static esp_err_t spiflash_read_sr(uint8_t *sr);
static esp_err_t spiflash_wait_wip(uint32_t timeout_ms);
static esp_err_t spiflash_write_enable(void);
static void spiflash_pack_addr(uint8_t *dst, uint32_t addr);
static void spiflash_erase_task(void *arg);

esp_err_t SPIFLASH_init(void)
{
	if(init_done){
		return ESP_OK;
	}

	ESP_RETURN_ON_ERROR(SPI_checkInit(), TAG, "SPI bus not initialized");

	/* Single device: address bytes are sent in the payload (supports 3/4-byte). */
	ESP_RETURN_ON_ERROR(SPI_registerDevice(&spi_dev, SPI_SLAVE_FLASH_PIN, SPI_SCK_10MHZ, 1, 8, 0),
			TAG, "Failed to register flash SPI device");

	uint8_t jedec[3] = {0};
	ESP_RETURN_ON_ERROR(SPI_transfer(spi_dev, SPIFLASH_CMD_RDID, 0, NULL, jedec, sizeof(jedec)),
			TAG, "RDID failed");

	uint8_t density = jedec[2];
	if((density < 0x10) || (density > 0x20)){
		ESP_LOGE(TAG, "Unexpected JEDEC density 0x%02X (mfr=0x%02X type=0x%02X)",
				density, jedec[0], jedec[1]);
		return ESP_ERR_NOT_SUPPORTED;
	}

	flash_size_B = 1u << density;
	addr_bytes = (flash_size_B > SPIFLASH_16MB) ? 4 : 3;

	ESP_LOGI(TAG, "JEDEC %02X %02X %02X, size=%lu bytes, addr=%u-byte",
			jedec[0], jedec[1], jedec[2],
			(unsigned long)flash_size_B,
			(unsigned)addr_bytes);

	if(addr_bytes == 4){
		ESP_RETURN_ON_ERROR(spiflash_cmd(SPIFLASH_CMD_EN4B), TAG, "EN4B failed");
	}

	busy = false;
	init_done = true;
	return ESP_OK;
}

uint32_t SPIFLASH_getSize(void)
{
	return flash_size_B;
}

bool SPIFLASH_isBusy(void)
{
	return busy;
}

esp_err_t SPIFLASH_read(uint32_t position, void *buffer, uint32_t size)
{
	if(!init_done){
		return ESP_ERR_INVALID_STATE;
	}
	if(busy){
		return ESP_ERR_INVALID_STATE;
	}
	if((buffer == NULL) || (size == 0) || ((position + size) > flash_size_B)){
		return ESP_ERR_INVALID_ARG;
	}

	uint8_t *out = (uint8_t *)buffer;
	uint32_t remaining = size;
	uint32_t addr = position;
	uint8_t tx[4 + SPIFLASH_XFER_MAX];
	uint8_t rx[4 + SPIFLASH_XFER_MAX];

	while(remaining > 0){
		uint32_t chunk = remaining > SPIFLASH_XFER_MAX ? SPIFLASH_XFER_MAX : remaining;
		uint32_t xfer_len = addr_bytes + chunk;

		memset(tx, 0, xfer_len);
		spiflash_pack_addr(tx, addr);

		esp_err_t err = SPI_transfer(spi_dev, SPIFLASH_CMD_READ, 0, tx, rx, (int)xfer_len);
		if(err != ESP_OK){
			ESP_LOGE(TAG, "Read failed at 0x%08lX", (unsigned long)addr);
			return err;
		}

		memcpy(out, &rx[addr_bytes], chunk);
		addr += chunk;
		out += chunk;
		remaining -= chunk;
	}

	return ESP_OK;
}

esp_err_t SPIFLASH_prog(uint32_t position, void *buffer, uint32_t size)
{
	if(!init_done){
		return ESP_ERR_INVALID_STATE;
	}
	if(busy){
		return ESP_ERR_INVALID_STATE;
	}
	if((buffer == NULL) || (size == 0) || ((position + size) > flash_size_B)){
		return ESP_ERR_INVALID_ARG;
	}

	uint8_t *in = (uint8_t *)buffer;
	uint32_t remaining = size;
	uint32_t addr = position;
	uint8_t tx[4 + SPIFLASH_PAGE_SIZE];

	while(remaining > 0){
		uint32_t page_off = addr % SPIFLASH_PAGE_SIZE;
		uint32_t page_space = SPIFLASH_PAGE_SIZE - page_off;
		uint32_t chunk = remaining > page_space ? page_space : remaining;
		uint32_t xfer_len = addr_bytes + chunk;

		spiflash_pack_addr(tx, addr);
		memcpy(&tx[addr_bytes], in, chunk);

		ESP_RETURN_ON_ERROR(spiflash_write_enable(), TAG, "WREN failed");
		ESP_RETURN_ON_ERROR(SPI_transfer(spi_dev, SPIFLASH_CMD_PP, 0, tx, NULL, (int)xfer_len),
				TAG, "Page program failed at 0x%08lX", (unsigned long)addr);
		ESP_RETURN_ON_ERROR(spiflash_wait_wip(100), TAG, "WIP timeout after page program");

		addr += chunk;
		in += chunk;
		remaining -= chunk;
	}

	return ESP_OK;
}

esp_err_t SPIFLASH_eraseAll(SPIFLASH_done_cb_t done_cb)
{
	if(!init_done){
		return ESP_ERR_INVALID_STATE;
	}
	if(busy){
		ESP_LOGE(TAG, "Erase already in progress");
		return ESP_ERR_INVALID_STATE;
	}

	erase_done_cb = done_cb;
	busy = true;

	esp_err_t err = spiflash_write_enable();
	if(err != ESP_OK){
		busy = false;
		erase_done_cb = NULL;
		return err;
	}

	err = spiflash_cmd(SPIFLASH_CMD_CE);
	if(err != ESP_OK){
		busy = false;
		erase_done_cb = NULL;
		ESP_LOGE(TAG, "Chip erase command failed");
		return err;
	}

	BaseType_t ok = xTaskCreate(spiflash_erase_task, "spiflash_erase", 2048, NULL, tskIDLE_PRIORITY + 1, NULL);
	if(ok != pdPASS){
		busy = false;
		erase_done_cb = NULL;
		ESP_LOGE(TAG, "Failed to create erase task");
		return ESP_ERR_NO_MEM;
	}

	ESP_LOGI(TAG, "Chip erase started");
	return ESP_OK;
}

static void spiflash_pack_addr(uint8_t *dst, uint32_t addr)
{
	if(addr_bytes == 4){
		dst[0] = (uint8_t)((addr >> 24) & 0xFF);
		dst[1] = (uint8_t)((addr >> 16) & 0xFF);
		dst[2] = (uint8_t)((addr >> 8) & 0xFF);
		dst[3] = (uint8_t)(addr & 0xFF);
	}
	else {
		dst[0] = (uint8_t)((addr >> 16) & 0xFF);
		dst[1] = (uint8_t)((addr >> 8) & 0xFF);
		dst[2] = (uint8_t)(addr & 0xFF);
	}
}

static esp_err_t spiflash_cmd(uint8_t cmd)
{
	return SPI_transfer(spi_dev, cmd, 0, NULL, NULL, 0);
}

static esp_err_t spiflash_read_sr(uint8_t *sr)
{
	return SPI_transfer(spi_dev, SPIFLASH_CMD_RDSR, 0, NULL, sr, 1);
}

static esp_err_t spiflash_write_enable(void)
{
	return spiflash_cmd(SPIFLASH_CMD_WREN);
}

static esp_err_t spiflash_wait_wip(uint32_t timeout_ms)
{
	TickType_t start = xTaskGetTickCount();
	TickType_t timeout = pdMS_TO_TICKS(timeout_ms);

	while(1){
		uint8_t sr = 0;
		esp_err_t err = spiflash_read_sr(&sr);
		if(err != ESP_OK){
			return err;
		}
		if((sr & SPIFLASH_SR_WIP) == 0){
			return ESP_OK;
		}
		if((xTaskGetTickCount() - start) > timeout){
			return ESP_ERR_TIMEOUT;
		}
		vTaskDelay(pdMS_TO_TICKS(1));
	}
}

static void spiflash_erase_task(void *arg)
{
	(void)arg;
	TickType_t start = xTaskGetTickCount();
	TickType_t timeout = pdMS_TO_TICKS(SPIFLASH_ERASE_TIMEOUT_MS);
	bool success = false;

	while(1){
		uint8_t sr = 0;
		if(spiflash_read_sr(&sr) != ESP_OK){
			ESP_LOGE(TAG, "Erase poll RDSR failed");
			break;
		}
		if((sr & SPIFLASH_SR_WIP) == 0){
			success = true;
			break;
		}
		if((xTaskGetTickCount() - start) > timeout){
			ESP_LOGE(TAG, "Chip erase timeout");
			break;
		}
		vTaskDelay(pdMS_TO_TICKS(SPIFLASH_ERASE_POLL_MS));
	}

	busy = false;
	SPIFLASH_done_cb_t cb = erase_done_cb;
	erase_done_cb = NULL;

	if(success){
		ESP_LOGI(TAG, "Chip erase finished");
	}

	if(cb != NULL){
		cb();
	}

	vTaskDelete(NULL);
}

#else /* !SFS_USE_SPI_FLASH */

esp_err_t SPIFLASH_init(void)
{
	return ESP_ERR_NOT_SUPPORTED;
}

uint32_t SPIFLASH_getSize(void)
{
	return 0;
}

bool SPIFLASH_isBusy(void)
{
	return false;
}

esp_err_t SPIFLASH_read(uint32_t position, void *buffer, uint32_t size)
{
	(void)position;
	(void)buffer;
	(void)size;
	return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t SPIFLASH_prog(uint32_t position, void *buffer, uint32_t size)
{
	(void)position;
	(void)buffer;
	(void)size;
	return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t SPIFLASH_eraseAll(SPIFLASH_done_cb_t done_cb)
{
	(void)done_cb;
	return ESP_ERR_NOT_SUPPORTED;
}

#endif /* SFS_USE_SPI_FLASH */
