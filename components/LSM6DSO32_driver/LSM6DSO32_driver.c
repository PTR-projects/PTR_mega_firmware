#include <stdio.h>
#include "esp_err.h"
#include "SPI_driver.h"
#include "LSM6DSO32_driver.h"

const uint8_t aclOdr[12] = {0, 11, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
const uint8_t gyroOdr[11] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}

const uint8_t aclScale[4] = {0, 2, 3, 1};
const uint8_t gyroScale[5] = {4, 0, 1, 2 ,3}

double * outputDataRate = NULL;
uint8_t CTRL2_Gval = 0, CTRL6_Cval = 0;
esp_err_t SPI_write(uint8_t* dataArr, uint8_t len, uint8_t LSM6_addr) {
    spi_transaction_t metaSpi;
    
    metaSpi.tx_buffer = dataArr;
	metaSpi.length = (len+1) << 3;
	metaSpi.rxlength = 8;
    metaSpi.addr = LSM6_addr;
	metaSpi.cmd = CMD_WRITE;
	
    spi_device_acquire_bus(spi_dev_handle_LSM6, portMAX_DELAY);

    if(spi_device_polling_transmit(spi_dev_handle_LSM6, &metaSpi) == ESP_FAIL) return ESP_FAIL;

    spi_device_release_bus(spi_dev_handle_LSM6);

    return ESP_OK;
}

esp_err_t SPI_read(uint8_t* dataArr, uint8_t len, uint8_t LSM6_addr) {

    spi_transaction_t metaSpi;
    
    metaSpi.rx_buffer = dataArr;
	metaSpi.length = (len+1) << 3;
	metaSpi.rxlength = len << 3;
    metaSpi.addr = LSM6_addr;
	metaSpi.cmd = CMD_READ;
	
    spi_device_acquire_bus(spi_dev_handle_LSM6, portMAX_DELAY);

    if(spi_device_polling_transmit(spi_dev_handle_LSM6, &metaSpi) == ESP_FAIL) return ESP_FAIL;

    spi_device_release_bus(spi_dev_handle_LSM6);

    return ESP_OK;
}

esp_err_t LSM6_spi_init(uint8_t EN_PIN) {

    spi_device_interface_config_t LSM6_spi_config = {
			.mode           =  0,
			.spics_io_num   = EN_PIN,
			.clock_speed_hz =  1000000,
			.queue_size     =  1,
			.command_bits = 2,
			.address_bits = 6,

		};

    if(spi_bus_add_device(SPI_BUS, &LSM6_spi_config, &spi_dev_handle_LSM6) == ESP_FAIL) return ESP_FAIL;

    SPI_write(&settings3c, FIFO_CTRL, 1);
    SPI_write(&settingsFIFO, FIFO_CTRL_5, 1);

    return ESP_OK;
}

esp_err_t LSM6_setGyroReadout(uint8_t gyroResolution_DPS, uint8_t gyroLPF) {
	esp_err_t status = ESP_OK;
	CTRL2_Gval &= 0b000001110;
	if(!gyroResolution) {
		CTRL2_Gval |= 2;
	} else {CTRL2_Gval |= gyroResolution_DPS<<2;}
	CTRL6_Cval &= 0b11110000;
	CTRL6_Cval |= gyroLPF;
	
    status = SPI_write(&CTRL2_Gval, CTRL2_G, 1);
	if(status == ESP_OK) status = SPI_write(&CTRL2_Gval, CTRL2_G, 1); 
	return status;

}

esp_err_t LSM6_setGyroODR(uint16_t gyroDataRate_Hz) {
	CTRL2_Gval &= 0b000001110;
	CTRL2_Gval |= gyroDataRate_Hz << 4; 
    return SPI_write(&CTRL2_Gval, CTRL2_G, 1);	
}

esp_err_t LSM6_aclSettings(uint8_t settings, uint8_t resolution, uint8_t ODR, uint8_t LPF, uint8_t EN_PIN) {

    defaultSettingsAcl = (ODR << 4) | (resolution << 2);

    SPI_write(&LPF, CTRL8_XL, 1);
    SPI_write(&defaultSettingsAcl, CTRL1_XL, 1, EN_PIN);

}

esp_err_t LSM6_getRawData(uint8_t* rawData) {

    const uint8_t packet = OUTX_L_G;

    SPI_read(rawData, 12);
}

esp_err_t LSM6_getData(uint8_t* sensorData, uint8_t EN_PIN) {
   


}

esp_err_t LSM6_soft_reset(uint8_t EN_PIN) {
    settings |= 0b10000000;
    SPI_write(&settings3c, CTRL_3, 1);
    settings &= 0b01111111;
}