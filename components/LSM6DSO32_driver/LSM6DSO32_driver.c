#include "LSM6DSO32_driver.h"

static const char *TAG = "LSM6DSO32";

#define INIT_LSM6DS_ACC_SENS LSM6DS_ACC_FS_32G
#define INIT_LSM6DS_GYRO_DPS LSM6DS_GYRO_FS_2000_DPS

esp_err_t LSM6DSO32_Write(uint8_t sensor, LSM6DSO32_register_t reg, uint8_t val);
esp_err_t LSM6DSO32_Read (uint8_t sensor, LSM6DSO32_register_t reg, uint8_t * rx, uint8_t length);
esp_err_t LSM6DSO32_SetRegister(uint8_t sensor, LSM6DSO32_register_t, uint8_t val);


#ifdef SPI_SLAVE_LSM6DSO32_2_PIN
#define SPI_SLAVE_LSM6DSO32_PIN_NUM(x) ((x==0) ? SPI_SLAVE_LSM6DSO32_PIN:SPI_SLAVE_LSM6DSO32_2_PIN)
#else
#define SPI_SLAVE_LSM6DSO32_PIN_NUM(x) SPI_SLAVE_LSM6DSO32_PIN
#endif



const LSM6DSO32_register_addr_t LSM6DSO32_register_addr[LSM6DS_NUMBER_OF_REGISTERS]={
	LSM6DS_WHOAMI_RESPONSE_ADDR,
	LSM6DS_FUNC_CFG_ACCESS_ADDR,
	LSM6DS_INT1_CTRL_ADDR,
	LSM6DS_INT2_CTRL_ADDR,
	LSM6DS_WHOAMI_ADDR,
	LSM6DS_CTRL1_XL_ADDR,
	LSM6DS_CTRL2_G_ADDR,
	LSM6DS_CTRL3_C_ADDR,
	LSM6DS_CTRL4_C_ADDR,
	LSM6DS_CTRL5_C_ADDR,
	LSM6DS_CTRL6_C_ADDR,
	LSM6DS_CTRL7_G_ADDR,
	LSM6DS_CTRL8_XL_ADDR,
	LSM6DS_CTRL9_XL_ADDR,
	LSM6DS_CTRL10_C_ADDR,
	LSM6DS_WAKEUP_SRC_ADDR,
	LSM6DS_STATUS_REG_ADDR,
	LSM6DS_OUT_TEMP_L_ADDR,
	LSM6DS_OUTX_L_G_ADDR,
	LSM6DS_OUTX_L_A_ADDR,
	LSM6DS_STEPCOUNTER_ADDR,
	LSM6DS_TAP_CFG_ADDR
};

const uint8_t LSM6DSAccSensBits[LSM6DS_ACC_FS_LIST_SIZE]=
{
	LSM6DS_CTRL1_XL_ACC_FS_4G,	//LSM6DS_ACC_FS_4G
	LSM6DS_CTRL1_XL_ACC_FS_8G,	//LSM6DS_ACC_FS_8G
	LSM6DS_CTRL1_XL_ACC_FS_16G,	//LSM6DS_ACC_FS_16G
	LSM6DS_CTRL1_XL_ACC_FS_32G 	//LSM6DS_ACC_FS_32G
};

const float LSM6DSAccSensGPerLsb[LSM6DS_ACC_FS_LIST_SIZE]=
{
	0.000122f,	//LSM6DS_ACC_FS_4G
	0.000244f,	//LSM6DS_ACC_FS_8G
	0.000488f,	//LSM6DS_ACC_FS_16G
	0.000976f 	//LSM6DS_ACC_FS_32G
};

const uint8_t LSM6DSGyroDpsBits[LSM6DS_ACC_FS_LIST_SIZE]=
{
	LSM6DS_CTRL2_G_GYRO_FS_125_DPS , // LSM6DS_GYRO_FS_125_DPS
	LSM6DS_CTRL2_G_GYRO_FS_250_DPS , // LSM6DS_GYRO_FS_250_DPS 
	LSM6DS_CTRL2_G_GYRO_FS_500_DPS , // LSM6DS_GYRO_FS_500_DPS
	LSM6DS_CTRL2_G_GYRO_FS_1000_DPS, // LSM6DS_GYRO_FS_1000_DPS
	LSM6DS_CTRL2_G_GYRO_FS_2000_DPS, // LSM6DS_GYRO_FS_2000_DPS
	
};

const float LSM6DSGyroDpsPerLsb[LSM6DS_ACC_FS_LIST_SIZE]=
{
	0.004375f,	// LSM6DS_GYRO_FS_125_DPS
	0.00875f, 	// LSM6DS_GYRO_FS_250_DPS 
	0.01750f, 	// LSM6DS_GYRO_FS_500_DPS
	0.035f, 	// LSM6DS_GYRO_FS_1000_DPS
	0.070f, 	// LSM6DS_GYRO_FS_2000_DPS
};

typedef struct
{
	spi_dev_handle_t spi_dev_handle_LSM6DSO32;
	uint8_t LSM6DSO32_register_value[LSM6DS_NUMBER_OF_REGISTERS];
	float LSM6DSAccSensMgPerLsbCurrent;
	float LSM6DSGyrDpsPerLsb;

} LSM6DS_config_t;
typedef struct{

	LSM6DSO32_raw_data_t rawData;
	LSM6DS_meas_t meas;
	LSM6DS_config_t config;
	float accXoffset;
	float accYoffset;
	float accZoffset;

	float gyroXoffset;
	float gyroYoffset;
	float gyroZoffset;
} LSM6DSO32_t;

static LSM6DSO32_t LSM6DSO32_d[LSM6DSO32_COUNT];

esp_err_t LSM6DSO32_SPIinit(){
	if(SPI_checkInit() != ESP_OK){
		ESP_LOGE(TAG, "SPI controller not initialized! Use SPI_init() in main.c");
		return ESP_ERR_INVALID_STATE;
	}

	/* CONFIGURE SPI DEVICE */
	/* Max SCK frequency - 10MHz */
	for(uint8_t x = 0; x < LSM6DSO32_COUNT; x++)
	{
		ESP_RETURN_ON_ERROR(SPI_registerDevice(&LSM6DSO32_d[x].config.spi_dev_handle_LSM6DSO32, SPI_SLAVE_LSM6DSO32_PIN_NUM(x),
												SPI_SCK_10MHZ, 1, 1, 7), TAG, "SPI register for LSM6DS number: %x failed", x);
	}

	return ESP_OK;
}

esp_err_t LSM6DSO32_init(){
	LSM6DSO32_SPIinit();
	for(uint8_t sensor = 0; sensor < LSM6DSO32_COUNT; sensor++)
	{
		LSM6DSO32_Write(sensor, LSM6DS_CTRL1_XL, (LSM6DS_CTRL1_XL_ACC_RATE_104_HZ | LSM6DSAccSensBits[INIT_LSM6DS_ACC_SENS] | LSM6DS_CTRL1_ACC_LPF2_EN));
		LSM6DSO32_Write(sensor, LSM6DS_CTRL2_G,  (LSM6DS_CTRL2_G_GYRO_RATE_104_HZ | LSM6DSGyroDpsBits[INIT_LSM6DS_GYRO_DPS]));
		LSM6DSO32_Write(sensor, LSM6DS_CTRL3_C,  LSM6DS_CTRL3_BDU | LSM6DS_CTRL3_INT_PP | LSM6DS_CTRL3_INT_H | LSM6DS_CTRL3_INC);
		LSM6DSO32_Write(sensor, LSM6DS_CTRL4_C,  LSM6DS_CTRL4_INT12_SEP | LSM6DS_CTRL4_I2C_DIS | LSM6DS_CTRL4_GYRO_LPF1_EN);
		LSM6DSO32_Write(sensor, LSM6DS_CTRL5_C,  LSM6DS_CTRL5_ACC_ULP_DIS | LSM6DS_CTRL5_ROUNDING_DIS | LSM6DS_CTRL5_GYRO_ST_DIS | LSM6DS_CTRL5_ACC_ST_DIS);
		LSM6DSO32_Write(sensor, LSM6DS_CTRL6_C,  LSM6DS_CTRL6_GYRO_LPF1_0);
		LSM6DSO32_Write(sensor, LSM6DS_CTRL7_G, 0);	//default
		LSM6DSO32_Write(sensor, LSM6DS_CTRL8_XL, LSM6DS_CTRL8_ACC_LPF | LSM6DS_CTRL8_FILTER_ODR_4);
		LSM6DSO32_WhoAmI(sensor);
		LSM6DSO32_d[sensor].config.LSM6DSAccSensMgPerLsbCurrent = LSM6DSAccSensGPerLsb[INIT_LSM6DS_ACC_SENS];
		LSM6DSO32_d[sensor].config.LSM6DSGyrDpsPerLsb = LSM6DSGyroDpsPerLsb[INIT_LSM6DS_GYRO_DPS]; 
	}
	return ESP_OK;
}

uint8_t LSM6DSO32_WhoAmI(uint8_t sensor){
	uint8_t rxBuff[1] = {0U};
	LSM6DSO32_Read(sensor, LSM6DS_WHOAMI, rxBuff, 1);
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



esp_err_t LSM6DSO32_readMeasByID(uint8_t sensor){
	LSM6DSO32_Read(sensor, LSM6DS_OUT_TEMP_L, LSM6DSO32_d[sensor].rawData.raw, 14);

	LSM6DSO32_d[sensor].meas.accX  = (LSM6DSO32_d[sensor].rawData.accX_raw)*(LSM6DSO32_d[sensor].config.LSM6DSAccSensMgPerLsbCurrent) - LSM6DSO32_d[sensor].accXoffset;
	LSM6DSO32_d[sensor].meas.accY  = (LSM6DSO32_d[sensor].rawData.accY_raw)*(LSM6DSO32_d[sensor].config.LSM6DSAccSensMgPerLsbCurrent) - LSM6DSO32_d[sensor].accYoffset;
	LSM6DSO32_d[sensor].meas.accZ  = (LSM6DSO32_d[sensor].rawData.accZ_raw)*(LSM6DSO32_d[sensor].config.LSM6DSAccSensMgPerLsbCurrent) - LSM6DSO32_d[sensor].accZoffset;

	LSM6DSO32_d[sensor].meas.gyroX = (LSM6DSO32_d[sensor].rawData.gyroX_raw)*(LSM6DSO32_d[sensor].config.LSM6DSGyrDpsPerLsb) - LSM6DSO32_d[sensor].gyroXoffset;
	LSM6DSO32_d[sensor].meas.gyroY = (LSM6DSO32_d[sensor].rawData.gyroY_raw)*(LSM6DSO32_d[sensor].config.LSM6DSGyrDpsPerLsb) - LSM6DSO32_d[sensor].gyroYoffset;
	LSM6DSO32_d[sensor].meas.gyroZ = (LSM6DSO32_d[sensor].rawData.gyroZ_raw)*(LSM6DSO32_d[sensor].config.LSM6DSGyrDpsPerLsb) - LSM6DSO32_d[sensor].gyroZoffset;

	LSM6DSO32_d[sensor].meas.temp  = (LSM6DSO32_d[sensor].rawData.temp_raw)*(0.00390625f) + 25.0f;

	return ESP_OK;
}

esp_err_t LSM6DSO32_readMeasAll(){
	for(uint8_t sensor = 0; sensor < LSM6DSO32_COUNT; sensor++)
		{
			LSM6DSO32_readMeasByID(sensor);
		}
	return ESP_OK;
}

esp_err_t LSM6DSO32_getMeasByID(uint8_t sensor, LSM6DS_meas_t * meas){
	*meas = LSM6DSO32_d[sensor].meas;

	return ESP_OK;
}

esp_err_t LSM6DSO32_getMeasAll(LSM6DS_meas_t *meas){
	for(uint8_t x = 0; x < LSM6DSO32_COUNT; x++)
		{
			*(meas+x) = LSM6DSO32_d[x].meas;
		}

	return ESP_OK;
}




esp_err_t LSM6DSO32_SetRegister(uint8_t sensor, LSM6DSO32_register_t eRegisterToSet, uint8_t val){
	esp_err_t retval = ESP_FAIL;
	retval = LSM6DSO32_Write(sensor, eRegisterToSet, val);
	if(ESP_OK == retval)
	{
		LSM6DSO32_d[sensor].config.LSM6DSO32_register_value[eRegisterToSet] = val;
		ESP_LOGD(TAG,"LSM6DSO32 no.%d register no.0x%x set to 0x%x", sensor, eRegisterToSet, val);
	}
	else
	{
		ESP_LOGE(TAG,"LSM6DSO32 no.%d EROOR setting register no.0x%x to 0x%x", sensor, eRegisterToSet, val);
	}
	return ESP_OK;
}

esp_err_t LSM6DSO32_Write(uint8_t sensor, LSM6DSO32_register_t reg, uint8_t val){
	return SPI_transfer(LSM6DSO32_d[sensor].config.spi_dev_handle_LSM6DSO32, 0, LSM6DSO32_register_addr[reg], &val, NULL, 1);
}

esp_err_t LSM6DSO32_Read(uint8_t sensor, LSM6DSO32_register_t reg, uint8_t * rx, uint8_t length){
	if( NULL != LSM6DSO32_d[sensor].config.spi_dev_handle_LSM6DSO32 )
	{
		return SPI_transfer(LSM6DSO32_d[sensor].config.spi_dev_handle_LSM6DSO32, 1, LSM6DSO32_register_addr[reg], NULL, rx, length);
	}
	else
	{
		ESP_LOGE(TAG,"Nullpointer while reading!");
	}
	return ESP_OK;
}

esp_err_t LSM6DSO32_SetBitInRegister(uint8_t sensor, LSM6DSO32_register_t eRegisterToSet, uint8_t bitPos, bool bit){
	uint8_t val;
	LSM6DSO32_Read(sensor, eRegisterToSet, &val, 1);
	val = ((val & ~(1 << bitPos)) | (bit << bitPos));
	return LSM6DSO32_SetRegister(sensor, eRegisterToSet, val);
}

esp_err_t LSM6DSO32_Calibration(uint8_t sensor){
	
	if( !(LSM6DSO32_COUNT > sensor) )
	{
		return LSM6DSO32_Read(sensor, LSM6DS_OUT_TEMP_L, LSM6DSO32_d[sensor].rawData.raw, 14);

	}
	else
	{
		ESP_LOGE(TAG,"Wrong sensor number!");
	}
	return ESP_OK;
}