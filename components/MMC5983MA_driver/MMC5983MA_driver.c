#include <stdio.h>
#include "esp_err.h"
#include "esp_log.h"
#include "MMC5983MA_driver.h"


#define READ_REG(x) (0x80 | x)
static const char* TAG = "MMC5983MA";


esp_err_t MMC5983MA_read(uint8_t  address, uint8_t * buf, uint8_t len) {
	uint8_t txBuff[7] = {READ_REG(address)};
    SPI_RW(SPI_SLAVE_MMC5983MA, txBuff, buf, len+1);
    return ESP_OK;
}

uint8_t MMC5983MA_readSingleByte(uint8_t  address) {
	uint8_t txBuff[2] = {READ_REG(address)};
	uint8_t buf[2] = {0};
    SPI_RW(SPI_SLAVE_MMC5983MA, txBuff, buf, 2);
    return buf[1];
}

esp_err_t MMC5983MA_command(uint8_t command) {
    uint8_t txBuff[1] = {command};
    SPI_RW(SPI_SLAVE_MMC5983MA, txBuff, NULL, 1);
    ESP_LOGD(TAG, "Command %X sent to MMC5883MA", command);
    return ESP_OK;
}

esp_err_t MMC5983MA_writeSingleByte(uint8_t adress, uint8_t value) {
    uint8_t txBuff[2] = {adress,value};
    SPI_RW(SPI_SLAVE_MMC5983MA, txBuff, NULL, 2);
    ESP_LOGD(TAG, "Byte sent to MMC5883MA Adress: %X Value: %X", txBuff[0], txBuff[1]);
    return ESP_OK;
}

esp_err_t MMC5983MA_init()
{
	uint8_t buf[2] = {0};
	MMC5983MA_read(PROD_ID_REG, buf, 1);

	if(buf[1] == PROD_ID){
		ESP_LOGI(TAG, "MMC5883MA initialization returned: %X", buf[1]);
		return ESP_OK;
	}
	ESP_LOGW(TAG, "WARNING: MMC5883MA initialization returned: %X", buf[1]);
	return ESP_FAIL;
}



esp_err_t MMC5983MA_setRegisterBit(const uint8_t registerAddress, const uint8_t bitMask)
{
    uint8_t value = MMC5983MA_readSingleByte(registerAddress);
    value |= bitMask;
    MMC5983MA_writeSingleByte(registerAddress, value);
    ESP_LOGD(TAG, "MMC5883MA %X Register set %X", registerAddress, value);
    return ESP_OK;
}

esp_err_t MMC5983MA_clearRegisterBit(const uint8_t registerAddress, const uint8_t bitMask)
{
    uint8_t value = MMC5983MA_readSingleByte(registerAddress);
    value &= ~bitMask;
    MMC5983MA_writeSingleByte(registerAddress, value);
    ESP_LOGD(TAG, "MMC5883MA %X Register set %X", registerAddress, value);
    return ESP_OK;
}

bool MMC5983MA_isRegisterSet(const uint8_t registerAddress, const uint8_t bitMask)
{
    uint8_t value = MMC5983MA_readSingleByte(registerAddress);
    return (value & bitMask);
}




