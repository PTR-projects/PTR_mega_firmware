#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"
#include "BOARD_cfg.h"
#include "SPI_driver.h"
#include "BMP581_driver.h"

#if defined(SPI_SLAVE_BMP581_PIN)
static const char *TAG = "BMP581";
#endif

#define BMP581_REG_CHIP_ID      0x01
#define BMP581_REG_TEMP_DATA    0x1D
#define BMP581_REG_OSR_CONFIG   0x36
#define BMP581_REG_ODR_CONFIG   0x37
#define BMP581_REG_OSR_EFF      0x38
#define BMP581_REG_CMD          0x7E

#define BMP581_CHIP_ID          0x50
#define BMP581_CMD_SOFT_RESET   0xB6
#define BMP581_SPI_READ         0x80

/* OSR 1x/1x with pressure enabled. Valid together with a 100 Hz ODR. */
#define BMP581_OSR_PRESS_EN     0x40
/* odr = 0x0A (100.299 Hz) at bit 2, pwr_mode = normal (0b01). */
#define BMP581_ODR_100HZ_NORMAL 0x29
#define BMP581_ODR_IS_VALID     0x80

#if !defined(SPI_SLAVE_BMP581_PIN)

esp_err_t BMP581_init(void) { return ESP_OK; }
esp_err_t BMP581_getReloadSmart(void) { return ESP_OK; }
esp_err_t BMP581_getMeas(uint8_t sensor, BMP581_meas_t *meas) {
	(void)sensor;
	if (meas != NULL) {
		memset(meas, 0, sizeof(*meas));
	}
	return ESP_OK;
}

#else

static spi_dev_handle_t bmp581_spi;
static BMP581_meas_t bmp581_meas;

static esp_err_t bmp581_write(uint8_t reg, uint8_t value)
{
	return SPI_transfer(bmp581_spi, 0, reg, &value, NULL, 1);
}

static esp_err_t bmp581_read(uint8_t reg, uint8_t *data, uint16_t length)
{
	return SPI_transfer(bmp581_spi, 0, (uint8_t)(reg | BMP581_SPI_READ), NULL, data, length);
}

static int32_t bmp581_sign_extend_24(uint32_t raw)
{
	if (raw & 0x800000u) {
		raw |= 0xFF000000u;
	}
	return (int32_t)raw;
}

esp_err_t BMP581_init(void)
{
	uint8_t chip_id = 0;
	uint8_t osr_eff = 0;

	if (SPI_checkInit() != ESP_OK) {
		ESP_LOGE(TAG, "SPI controller not initialized");
		return ESP_ERR_INVALID_STATE;
	}

	ESP_RETURN_ON_ERROR(SPI_registerDevice(&bmp581_spi, SPI_SLAVE_BMP581_PIN,
			SPI_SCK_10MHZ, 1, 0, 8), TAG, "SPI register failed");

	/* CSB high at power-up leaves the chip in I2C. One SPI transaction selects SPI. */
	(void)bmp581_read(BMP581_REG_CHIP_ID, &chip_id, 1);
	vTaskDelay(pdMS_TO_TICKS(2));

	ESP_RETURN_ON_ERROR(bmp581_write(BMP581_REG_CMD, BMP581_CMD_SOFT_RESET), TAG, "soft reset failed");
	vTaskDelay(pdMS_TO_TICKS(5));

	ESP_RETURN_ON_ERROR(bmp581_read(BMP581_REG_CHIP_ID, &chip_id, 1), TAG, "chip id read failed");
	if (chip_id != BMP581_CHIP_ID) {
		ESP_LOGE(TAG, "unexpected chip id 0x%02x", chip_id);
		return ESP_ERR_NOT_FOUND;
	}

	ESP_RETURN_ON_ERROR(bmp581_write(BMP581_REG_OSR_CONFIG, BMP581_OSR_PRESS_EN), TAG, "OSR config failed");
	ESP_RETURN_ON_ERROR(bmp581_write(BMP581_REG_ODR_CONFIG, BMP581_ODR_100HZ_NORMAL), TAG, "ODR config failed");

	ESP_RETURN_ON_ERROR(bmp581_read(BMP581_REG_OSR_EFF, &osr_eff, 1), TAG, "OSR_EFF read failed");
	if ((osr_eff & BMP581_ODR_IS_VALID) == 0) {
		ESP_LOGW(TAG, "ODR/OSR combination was rejected (OSR_EFF=0x%02x)", osr_eff);
	}

	ESP_LOGI(TAG, "BMP581 init OK");
	return ESP_OK;
}

esp_err_t BMP581_getReloadSmart(void)
{
	uint8_t raw[6];
	int32_t temp_raw;
	int32_t press_raw;

	ESP_RETURN_ON_ERROR(bmp581_read(BMP581_REG_TEMP_DATA, raw, sizeof(raw)), TAG, "data read failed");

	temp_raw = bmp581_sign_extend_24(((uint32_t)raw[2] << 16) | ((uint32_t)raw[1] << 8) | raw[0]);
	press_raw = bmp581_sign_extend_24(((uint32_t)raw[5] << 16) | ((uint32_t)raw[4] << 8) | raw[3]);

	bmp581_meas.temp = (float)temp_raw / 65536.0f;
	bmp581_meas.press = (float)press_raw / 64.0f;
	return ESP_OK;
}

esp_err_t BMP581_getMeas(uint8_t sensor, BMP581_meas_t *meas)
{
	if ((sensor != 0) || (meas == NULL)) {
		return ESP_ERR_INVALID_ARG;
	}
	*meas = bmp581_meas;
	return ESP_OK;
}

#endif
