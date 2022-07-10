#include <stdio.h>
#include <string.h>
#include <driver/spi_master.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "BOARD.h"
#include "SPI_driver.h"

static spi_device_handle_t spi_dev_handle;


esp_err_t SPI_init(uint32_t frequency){
	// -------------- Configure CS pins ----------------------------------
	gpio_reset_pin		(SPI_SLAVE_MS5607_PIN);
	gpio_set_direction	(SPI_SLAVE_MS5607_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level		(SPI_SLAVE_MS5607_PIN, 1);

	gpio_reset_pin		(SPI_SLAVE_LIS331_PIN);
	gpio_set_direction	(SPI_SLAVE_LIS331_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level		(SPI_SLAVE_LIS331_PIN, 1);

	gpio_reset_pin		(SPI_SLAVE_LSM6DSO32_PIN);
	gpio_set_direction	(SPI_SLAVE_LSM6DSO32_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level		(SPI_SLAVE_LSM6DSO32_PIN, 1);

	gpio_reset_pin		(SPI_SLAVE_FLASH_PIN);
	gpio_set_direction	(SPI_SLAVE_FLASH_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level		(SPI_SLAVE_FLASH_PIN, 1);

	gpio_reset_pin		(SPI_SLAVE_MMC5983MA_PIN);
	gpio_set_direction	(SPI_SLAVE_MMC5983MA_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level		(SPI_SLAVE_MMC5983MA_PIN, 1);

	gpio_reset_pin		(SPI_SLAVE_SX1262_PIN);
	gpio_set_direction	(SPI_SLAVE_SX1262_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level		(SPI_SLAVE_SX1262_PIN, 1);

	// -------------- SPI init --------------------------------------------
	esp_err_t ret;
	spi_bus_config_t buscfg={
		.miso_io_num   = SPI_MISO_PIN,
		.mosi_io_num   = SPI_MOSI_PIN,
		.sclk_io_num   = SPI_SCK_PIN,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
	};

	ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_DISABLED); //Initialize the SPI bus (prev: SPI_DMA_CH_AUTO)
	ESP_ERROR_CHECK(ret);

	spi_device_interface_config_t devcfg = {
		.mode           =  0,
		.spics_io_num   = -1,
		.clock_speed_hz =  1 * 1000 * 1000,
		.queue_size     =  1,
	};

	ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi_dev_handle));

	return ESP_OK;
}

esp_err_t SPI_RW(spi_slave_t slave, uint8_t * data_out, uint8_t * data_in, uint16_t length){
	SPI_CS(slave, 0);

	esp_err_t ret = ESP_OK;
	spi_transaction_t trans;
	memset(&trans, 0x00, sizeof(trans));
	trans.length = 8 * length;
	trans.tx_buffer = data_out;
	trans.rx_buffer = data_in;

	spi_device_acquire_bus(spi_dev_handle, portMAX_DELAY);
	if (spi_device_polling_transmit(spi_dev_handle, &trans) != ESP_OK)
	{
		ESP_LOGE("SPI DRIVER", "%s(%d): spi transmit failed", __FUNCTION__, __LINE__);
		ret = ESP_FAIL;
	}
	spi_device_release_bus(spi_dev_handle);

	SPI_CS(slave, 1);

	return ret;
}

esp_err_t SPI_CS(spi_slave_t slave, uint8_t state){
	switch(slave){
	case SPI_SLAVE_MS5607:
		gpio_set_level(SPI_SLAVE_MS5607_PIN, state);
		break;

	case SPI_SLAVE_LIS331:
		gpio_set_level(SPI_SLAVE_LIS331_PIN, state);
		break;

	case SPI_SLAVE_LSM6DSO32:
		gpio_set_level(SPI_SLAVE_LSM6DSO32_PIN, state);
		break;

	case SPI_SLAVE_FLASH:
		gpio_set_level(SPI_SLAVE_FLASH_PIN, state);
		break;

	case SPI_SLAVE_MMC5983MA:
		gpio_set_level(SPI_SLAVE_MMC5983MA_PIN, state);
		break;

	case SPI_SLAVE_SX1262:
		gpio_set_level(SPI_SLAVE_SX1262_PIN, state);
		break;

	default:
		return ESP_ERR_INVALID_ARG;
	}
	return ESP_OK;
}
