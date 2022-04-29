#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "HAL_SPI.h"

static const char TAG[] = "SPI_HAL";

esp_err_t HAL_SPI_init(){
	ESP_LOGI(TAG, "Initializing bus SPI");
//	spi_bus_config_t buscfg={
//		.miso_io_num = PIN_NUM_MISO,
//		.mosi_io_num = PIN_NUM_MOSI,
//		.sclk_io_num = PIN_NUM_CLK,
//		.quadwp_io_num = -1,
//		.quadhd_io_num = -1,
//		.max_transfer_sz = 32,
//	};

	return ESP_OK;	//ESP_FAIL
}

esp_err_t SPI_HAL_RW(){
	return ESP_OK;	//ESP_FAIL
}


esp_err_t SPI_HAL_read(){
	return ESP_OK;	//ESP_FAIL
}

esp_err_t SPI_HAL_write(){
	return ESP_OK;	//ESP_FAIL
}
