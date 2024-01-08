#include "LSM6DSO32_driver.h"
#include "LSM6DSO32_privat.h"

/**
 * @brief Tag for identifying log messages related to LSM6DSO32.
 */
static const char *TAG = "LSM6DSO32";


/**
 * @brief LSM6DSO32 accelerometer initial sensitivity settings.
 */
#define INIT_LSM6DS_ACC_SENS LSM6DS_ACC_FS_32G
/**
 * @brief LSM6DSO32 gyroscope initial sensitivity settings.
 */
#define INIT_LSM6DS_GYRO_DPS LSM6DS_GYRO_FS_2000_DPS

static esp_err_t LSM6DSO32_Write(uint8_t sensor, LSM6DSO32_register_addr_t reg, uint8_t val);
static esp_err_t LSM6DSO32_Read (uint8_t sensor, LSM6DSO32_register_addr_t reg, uint8_t * rx, uint8_t length);
static esp_err_t LSM6DSO32_SetRegister(uint8_t sensor, LSM6DSO32_register_addr_t, uint8_t val);


#ifdef SPI_SLAVE_LSM6DSO32_2_PIN
#define SPI_SLAVE_LSM6DSO32_PIN_NUM(x) ((x==0) ? SPI_SLAVE_LSM6DSO32_PIN:SPI_SLAVE_LSM6DSO32_2_PIN)
#else
#define SPI_SLAVE_LSM6DSO32_PIN_NUM(x) SPI_SLAVE_LSM6DSO32_PIN
#endif

static LSM6DSO32_t LSM6DSO32_d[LSM6DSO32_COUNT];

/**
 * @brief Initializes the SPI communication for LSM6DSO32 sensors.
 *
 * @return esp_err_t ESP_OK if successful, otherwise an error code.
 */
esp_err_t LSM6DSO32_SPIinit(){
	if(SPI_checkInit() != ESP_OK){
		ESP_LOGE(TAG, "SPI controller not initialized! Use SPI_init() in main.c");
		return ESP_ERR_INVALID_STATE;
	}

	/* CONFIGURE SPI DEVICE */
	/* Max SCK frequency - 10MHz */
	for(uint8_t sensor = 0; LSM6DSO32_COUNT > sensor ; sensor++)
	{
		ESP_RETURN_ON_ERROR(SPI_registerDevice(&LSM6DSO32_d[sensor].config.spi_dev_handle_LSM6DSO32, SPI_SLAVE_LSM6DSO32_PIN_NUM(sensor),
												SPI_SCK_10MHZ, 1, 1, 7), TAG, "SPI register for LSM6DS number: %d sensor failed", sensor);
	}

	return ESP_OK;
}

/**
 * @brief Initializes LSM6DSO32 sensors.
 *
 * This function initializes the SPI communication for LSM6DSO32 sensors and configures
 * each sensor with default settings. It sets up the accelerometer and gyroscope parameters,
 * performs a WHO_AM_I check, and initializes internal data structures.
 *
 * @return esp_err_t ESP_OK if successful, otherwise an error code.
 */
esp_err_t LSM6DSO32_init(){
	LSM6DSO32_SPIinit();
	for(uint8_t sensor = 0; LSM6DSO32_COUNT > sensor ; sensor++)
	{
		LSM6DSO32_SetRegister(sensor, LSM6DS_CTRL1_XL_ADDR, (LSM6DS_CTRL1_XL_ACC_RATE_104_HZ | LSM6DSAccSensBits[INIT_LSM6DS_ACC_SENS] | LSM6DS_CTRL1_ACC_LPF2_EN));
		LSM6DSO32_SetRegister(sensor, LSM6DS_CTRL2_G_ADDR,  (LSM6DS_CTRL2_G_GYRO_RATE_104_HZ | LSM6DSGyroDpsBits[INIT_LSM6DS_GYRO_DPS]));
		LSM6DSO32_SetRegister(sensor, LSM6DS_CTRL3_C_ADDR,  LSM6DS_CTRL3_BDU | LSM6DS_CTRL3_INT_PP | LSM6DS_CTRL3_INT_H | LSM6DS_CTRL3_INC);
		LSM6DSO32_SetRegister(sensor, LSM6DS_CTRL4_C_ADDR,  LSM6DS_CTRL4_INT12_SEP | LSM6DS_CTRL4_I2C_DIS | LSM6DS_CTRL4_GYRO_LPF1_EN);
		LSM6DSO32_SetRegister(sensor, LSM6DS_CTRL5_C_ADDR,  LSM6DS_CTRL5_ACC_ULP_DIS | LSM6DS_CTRL5_ROUNDING_DIS | LSM6DS_CTRL5_GYRO_ST_DIS | LSM6DS_CTRL5_ACC_ST_DIS);
		LSM6DSO32_SetRegister(sensor, LSM6DS_CTRL6_C_ADDR,  LSM6DS_CTRL6_GYRO_LPF1_0);
		LSM6DSO32_SetRegister(sensor, LSM6DS_CTRL7_G_ADDR, 0);	//default
		LSM6DSO32_SetRegister(sensor, LSM6DS_CTRL8_XL_ADDR, LSM6DS_CTRL8_ACC_LPF | LSM6DS_CTRL8_FILTER_ODR_4);
		LSM6DSO32_WhoAmI(sensor);
		LSM6DSO32_d[sensor].config.LSM6DSAccSensMgPerLsbCurrent = LSM6DSAccSensGPerLsb[INIT_LSM6DS_ACC_SENS];
		LSM6DSO32_d[sensor].config.LSM6DSGyroDpsPerLsb = LSM6DSGyroDpsPerLsb[INIT_LSM6DS_GYRO_DPS]; 
	}
	return ESP_OK;
}

/**
 * @brief Retrieves the WHO_AM_I response from the LSM6DSO32 sensor.
 *
 * @param sensor Sensor number.
 * @return uint8_t WHO_AM_I response.
 */
uint8_t LSM6DSO32_WhoAmI(uint8_t sensor){
	uint8_t rxBuff[1] = {0U};
	LSM6DSO32_Read(sensor, LSM6DS_WHOAMI_ADDR, rxBuff, 1);
	if(LSM6DS_WHOAMI_RESPONSE == rxBuff[0])
		{
			ESP_LOGI(TAG, "LSM6DSO32 %x correct response", sensor);
		}
		else
		{
			ESP_LOGI(TAG, "LSM6DSO32 %x wrong response: %x\n", sensor, rxBuff[0]);
		}
	return rxBuff[0];
}


/**
 * @brief Reads measurement data from a specified LSM6DSO32 sensor.
 *
 * @param sensor Sensor number.
 * @return esp_err_t ESP_OK if successful, otherwise an error code.
 *
 * This function reads measurement data from the LSM6DSO32 sensor specified by the sensor number.
 * It performs the read operation and, if successful, calculates the accelerometer, gyroscope, and temperature measurements.
 */
esp_err_t LSM6DSO32_readMeasByID(uint8_t sensor){
	esp_err_t readResult = LSM6DSO32_Read(sensor, LSM6DS_OUT_TEMP_L_ADDR, LSM6DSO32_d[sensor].rawData.raw, 14);
	
	if (readResult == ESP_OK) 
	{
		LSM6DSO32_d[sensor].meas.accX  = (LSM6DSO32_d[sensor].rawData.accX_raw)*(LSM6DSO32_d[sensor].config.LSM6DSAccSensMgPerLsbCurrent) - LSM6DSO32_d[sensor].accXoffset;
		LSM6DSO32_d[sensor].meas.accY  = (LSM6DSO32_d[sensor].rawData.accY_raw)*(LSM6DSO32_d[sensor].config.LSM6DSAccSensMgPerLsbCurrent) - LSM6DSO32_d[sensor].accYoffset;
		LSM6DSO32_d[sensor].meas.accZ  = (LSM6DSO32_d[sensor].rawData.accZ_raw)*(LSM6DSO32_d[sensor].config.LSM6DSAccSensMgPerLsbCurrent) - LSM6DSO32_d[sensor].accZoffset;

		LSM6DSO32_d[sensor].meas.gyroX = (LSM6DSO32_d[sensor].rawData.gyroX_raw - LSM6DSO32_d[sensor].gyroXoffset) * LSM6DSO32_d[sensor].config.LSM6DSGyroDpsPerLsb;
		LSM6DSO32_d[sensor].meas.gyroY = (LSM6DSO32_d[sensor].rawData.gyroY_raw - LSM6DSO32_d[sensor].gyroYoffset) * LSM6DSO32_d[sensor].config.LSM6DSGyroDpsPerLsb;
		LSM6DSO32_d[sensor].meas.gyroZ = (LSM6DSO32_d[sensor].rawData.gyroZ_raw - LSM6DSO32_d[sensor].gyroZoffset) * LSM6DSO32_d[sensor].config.LSM6DSGyroDpsPerLsb;

		LSM6DSO32_d[sensor].meas.temp  = (LSM6DSO32_d[sensor].rawData.temp_raw)*(0.00390625f) + 25.0f;
	}
	else
	{
		ESP_LOGE(TAG, "LSM6DSO32_readMeasByID for %d failed with response: %d", sensor, readResult);
	}
	return readResult;
}

/**
 * @brief Reads all measurement data from all LSM6DSO32 sensors.
 *
 * @return esp_err_t ESP_OK if successful, otherwise an error code.
 */
esp_err_t LSM6DSO32_readMeasAll(){
	for(uint8_t sensor = 0; sensor < LSM6DSO32_COUNT; sensor++)
		{
			LSM6DSO32_readMeasByID(sensor);
		}
	return ESP_OK;
}

/**
 * @brief Retrieves measurement data from a specified LSM6DSO32 sensor.
 *
 * @param sensor Sensor number.
 * @param meas Pointer to LSM6DS_meas_t structure to store measurement data.
 * @return esp_err_t ESP_OK if successful, otherwise an error code.
 */
esp_err_t LSM6DSO32_getMeasByID(uint8_t sensor, LSM6DS_meas_t * meas){
	*meas = LSM6DSO32_d[sensor].meas;
	return ESP_OK;
}

/**
 * @brief Retrieves measurement data from all LSM6DSO32 sensors.
 *
 * @param meas Pointer to an array of LSM6DS_meas_t structures to store measurement data.
 * @return esp_err_t ESP_OK if successful, otherwise an error code.
 */
esp_err_t LSM6DSO32_getMeasAll(LSM6DS_meas_t *meas){
	for(uint8_t x = 0; x < LSM6DSO32_COUNT; x++)
		{
			*(meas+x) = LSM6DSO32_d[x].meas;
		}
	return ESP_OK;
}

/**
 * @brief Sets the value of a register in the LSM6DSO32 sensor.
 *
 * @param sensor Sensor number.
 * @param eRegisterToSet Register to set.
 * @param val Value to set in the register.
 * @return esp_err_t ESP_OK if successful, otherwise an error code.
 */
static esp_err_t LSM6DSO32_SetRegister(uint8_t sensor, LSM6DSO32_register_addr_t eRegisterToSet, uint8_t val){
	esp_err_t retval = ESP_FAIL;
	retval = LSM6DSO32_Write(sensor, eRegisterToSet, val);
	if(ESP_OK == retval)
	{
		ESP_LOGD(TAG,"LSM6DSO32 no.%d register no.0x%x set to 0x%x", sensor, eRegisterToSet, val);
	}
	else
	{
		ESP_LOGE(TAG,"LSM6DSO32 no.%d EROOR setting register no.0x%x to 0x%x", sensor, eRegisterToSet, val);
	}
	return ESP_OK;
}

/**
 * @brief Writes a single byte of data to a specified register of the LSM6DSO32 sensor.
 *
 * @param sensor Sensor number.
 * @param reg Register to write data to.
 * @param val Data byte to write.
 * @return esp_err_t ESP_OK if successful, otherwise an error code.
 */
static esp_err_t LSM6DSO32_Write(uint8_t sensor, LSM6DSO32_register_addr_t reg, uint8_t val) {
    if (LSM6DSO32_d[sensor].config.spi_dev_handle_LSM6DSO32 == NULL) {
        ESP_LOGE(TAG, "Sensor %d: Null pointer while writing!", sensor);
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t result = SPI_transfer(LSM6DSO32_d[sensor].config.spi_dev_handle_LSM6DSO32, 0, reg, &val, NULL, 1);

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Sensor %d: Failed to write data to LSM6DSO32: %d", sensor, result);
    }
  
    return result;
}

/**
 * @brief Reads a specified number of bytes from a register of the LSM6DSO32 sensor.
 *
 * @param sensor Sensor number.
 * @param reg Register to read data from.
 * @param rx Pointer to the buffer to store the read data.
 * @param length Number of bytes to read.
 * @return esp_err_t ESP_OK if successful, otherwise an error code.
 */
static esp_err_t LSM6DSO32_Read(uint8_t sensor, LSM6DSO32_register_addr_t reg, uint8_t *rx, uint8_t length) {
    if (LSM6DSO32_d[sensor].config.spi_dev_handle_LSM6DSO32 == NULL) {
        ESP_LOGE(TAG, "Sensor %d: Null pointer while reading!", sensor);
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t result = SPI_transfer(LSM6DSO32_d[sensor].config.spi_dev_handle_LSM6DSO32, 1, reg, NULL, rx, length);

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Sensor %d: Failed to read data from LSM6DSO32: %d", sensor, result);
    }

    return result;
}

/**
 * @brief Sets a specific bit in a register of the LSM6DSO32 sensor.
 *
 * @param sensor Sensor number.
 * @param eRegisterToSet Register to modify.
 * @param bitPos Bit position to set.
 * @param bit New bit value.
 * @return esp_err_t ESP_OK if successful, otherwise an error code.
 */
esp_err_t LSM6DSO32_SetBitInRegister(uint8_t sensor, LSM6DSO32_register_addr_t eRegisterToSet, uint8_t bitPos, bool bit){
	uint8_t val;
	LSM6DSO32_Read(sensor, eRegisterToSet, &val, 1);
	val = ((val & ~(1 << bitPos)) | (bit << bitPos));
	
	return LSM6DSO32_SetRegister(sensor, eRegisterToSet, val);
}

/**
 * @brief Performs calibration for the LSM6DSO32 sensor.
 *
 * @param sensor Sensor number.
 * @return esp_err_t ESP_OK if successful, otherwise an error code.
 */
esp_err_t LSM6DSO32_Calibration(uint8_t sensor){
	
	if( !(LSM6DSO32_COUNT > sensor) )
	{
		ESP_LOGE(TAG,"Not implemented!");

	}
	else
	{
		ESP_LOGE(TAG,"Wrong sensor number!");
	}
	
	return ESP_OK;
}

/**
 * @brief Sets the accelerometer sensitivity for the LSM6DSO32 sensor.
 *
 * @param sensor Sensor number.
 * @param setting New sensitivity setting.
 * @return esp_err_t ESP_OK if successful, otherwise an error code.
 */
esp_err_t LSM6DSO32_SetAccSens(uint8_t sensor, LSM6DS_acc_sens_setting_t setting)
{
	if( (LSM6DSO32_COUNT > sensor) )
	{
		uint8_t buffer;
		LSM6DSO32_Read(sensor, LSM6DS_CTRL1_XL_ADDR, &buffer, 8);
		buffer &= ~0b00001100;
		buffer |= LSM6DSAccSensBits[setting];
		LSM6DSO32_SetRegister(sensor, LSM6DS_CTRL1_XL_ADDR, buffer);
		LSM6DSO32_d[sensor].config.LSM6DSAccSensMgPerLsbCurrent = LSM6DSAccSensGPerLsb[setting];
	}
	else
	{
		ESP_LOGE(TAG,"Wrong sensor number!");
	}
	
	return ESP_OK;
}

esp_err_t LSM6DSO32_calibrateGyro(uint8_t sensor, float gain){
	static bool first_run = false;

	if(first_run == false){
		first_run = true;
		LSM6DSO32_d[sensor].gyroXoffset = (float)LSM6DSO32_d[sensor].rawData.gyroX_raw;
		LSM6DSO32_d[sensor].gyroYoffset = (float)LSM6DSO32_d[sensor].rawData.gyroY_raw;
		LSM6DSO32_d[sensor].gyroZoffset = (float)LSM6DSO32_d[sensor].rawData.gyroZ_raw;

		return ESP_OK;
	}

	LSM6DSO32_d[sensor].gyroXoffset = gain * (float)LSM6DSO32_d[sensor].rawData.gyroX_raw + (1.0f - gain) * LSM6DSO32_d[sensor].gyroXoffset;
	LSM6DSO32_d[sensor].gyroYoffset = gain * (float)LSM6DSO32_d[sensor].rawData.gyroY_raw + (1.0f - gain) * LSM6DSO32_d[sensor].gyroYoffset;
	LSM6DSO32_d[sensor].gyroZoffset = gain * (float)LSM6DSO32_d[sensor].rawData.gyroZ_raw + (1.0f - gain) * LSM6DSO32_d[sensor].gyroZoffset;

	return ESP_OK;
}



/**
 * @brief Sets the gyroscope sensitivity for the LSM6DSO32 sensor.
 *
 * @param sensor Sensor number.
 * @param setting New sensitivity setting.
 * @return esp_err_t ESP_OK if successful, otherwise an error code.
 */
esp_err_t LSM6DSO32_SetGyroDps(uint8_t sensor, LSM6DS_gyro_dps_setting_t setting)
{
	if( (LSM6DSO32_COUNT > sensor) )
	{
		uint8_t buffer;
		LSM6DSO32_Read(sensor, LSM6DS_CTRL2_G_ADDR, &buffer, 8);
		buffer &= ~0b00001110;
		buffer |= LSM6DSGyroDpsBits[setting];
		LSM6DSO32_SetRegister(sensor, LSM6DS_CTRL2_G_ADDR, buffer);
		LSM6DSO32_d[sensor].config.LSM6DSGyroDpsPerLsb = LSM6DSGyroDpsPerLsb[setting];
	}
	else
	{
		ESP_LOGE(TAG,"Wrong sensor number!");
	}
	
	return ESP_OK;
}
