#include <stdio.h>
#include <driver/spi_master.h>
#include "esp_err.h"
#include "SPI_driver.h"

esp_err_t SPI_init(uint32_t frequency){
	return ESP_OK;
}

esp_err_t SPI_RW(spi_slave_t slave, uint8_t * data_out, uint8_t * data_in, uint16_t length){

	return ESP_OK;
}
