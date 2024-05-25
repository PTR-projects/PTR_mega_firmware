#include <LSM6DSO32_fifo.h>

static const char *TAG = "LSM6DSO32_FIFO";

LSM6DSO32_fifo_data_t fifoBuffer = {0}; //ACC + GYRO

LSM6DSO32_fifo_data_t*  LSM6DSO32_fifoGetBuffer(){
    return &fifoBuffer;
}

esp_err_t LSM6DSO32_configure_fifo(uint8_t sensor){
	esp_err_t retVal = ESP_OK;
	retVal |= LSM6DSO32_SetRegister(sensor, LSM6DS_FIFO_CTRL1_ADDR, 1<<5); //Threshold set to 32 (16ACC + 16GYRO)
	retVal |= LSM6DSO32_SetRegister(sensor, LSM6DS_FIFO_CTRL2_ADDR, 1<<7); //Limit Fifo depth to threshold
	retVal |= LSM6DSO32_SetRegister(sensor, LSM6DS_FIFO_CTRL3_ADDR, (6<<4 | 6<<0)); // ACC and GYRO batching data rate 1667Hz
	retVal |= LSM6DSO32_SetRegister(sensor, LSM6DS_FIFO_CTRL4_ADDR, 6); //Continous mode
	return retVal;
}

esp_err_t LSM6DSO32_readFIFOStatusByID(uint8_t sensor, uint16_t *FIFOStatus){
	uint8_t FIFOStatusBuffer = 0;
	LSM6DSO32_Read(sensor, LSM6DS_FIFO_STATUS2_ADDR, &FIFOStatusBuffer, 1);
	*FIFOStatus = ((uint16_t)FIFOStatusBuffer & 0b00000011) << 8;  
	LSM6DSO32_Read(sensor, LSM6DS_FIFO_STATUS1_ADDR, &FIFOStatusBuffer, 1);
	*FIFOStatus += (uint16_t)FIFOStatusBuffer;
	return ESP_OK;
}

esp_err_t LSM6DSO32_readFIFOByID(uint8_t sensor) {
	esp_err_t readResult = ESP_OK;
	uint8_t accReadingCount = 0;
	uint8_t gyroReadingCount = 0;
	int32_t accDataRaw[3] = {0};
	int32_t gyroDataRaw[3] = {0};
	uint16_t FIFOStatus = 0;
	readResult |= LSM6DSO32_readFIFOStatusByID(sensor, &FIFOStatus);
	FIFOStatus =  (FIFOStatus > (LSM6DS_FIFO_BATCH_SIZE * 2)) ? (LSM6DS_FIFO_BATCH_SIZE * 2) : FIFOStatus; //Process no more then batch size
	if (!(FIFOStatus > 0))
	{
		ESP_LOGE(TAG, "FIFO Status read 0");
		return ESP_FAIL;
	}
	
	for(uint16_t FIFOSample = 0 ; FIFOSample < FIFOStatus; FIFOSample++)
	{
		readResult |= LSM6DSO32_Read(sensor, LSM6DS_FIFO_DATA_OUT_TAG_ADDR, &fifoBuffer.tag, 1);
		//ESP_LOGE(TAG, "tag: %x", fifoBuffer.tag);
		readResult |= LSM6DSO32_Read(sensor, LSM6DS_FIFO_DATA_OUT_X_L , fifoBuffer.dataOutRaw , 6);
		
		switch ((fifoBuffer.tag >> 3))
		{
		case 0x01: 
			{
				gyroReadingCount++;
				collect_gyro_data(gyroDataRaw, fifoBuffer.dataOutRaw);
				break;
			}

		case 0x02: 
			{
				accReadingCount++;
				collect_acc_data(accDataRaw, fifoBuffer.dataOutRaw);
				break;
			}
		default:
			{
				ESP_LOGE(TAG, "Wrong fifo tag read : %x", (fifoBuffer.tag >> 3));
			}
		}

	}
	parse_acc_data(accReadingCount, accDataRaw);
	parse_gyro_data(gyroReadingCount, gyroDataRaw);
	calc_acc(sensor, accDataRaw);
	calc_gyro(sensor, gyroDataRaw);
	return readResult;
}

esp_err_t collect_gyro_data(int32_t *gyroDataRaw, int16_t *sampleValue){
	gyroDataRaw[0] += (int32_t)sampleValue[0];
	gyroDataRaw[1] += (int32_t)sampleValue[1];
	gyroDataRaw[2] += (int32_t)sampleValue[2];
	return ESP_OK;
}
esp_err_t collect_acc_data(int32_t *accDataRaw, int16_t *sampleValue){
	accDataRaw[0] += (int32_t)sampleValue[0];
	accDataRaw[1] += (int32_t)sampleValue[1];
	accDataRaw[2] += (int32_t)sampleValue[2];
	return ESP_OK;
}

esp_err_t parse_gyro_data(const uint8_t sampleNum, int32_t *gyroDataRaw){
	if(sampleNum == 0){
		return ESP_FAIL;
	}
	else{
		gyroDataRaw[0] = gyroDataRaw[0] / sampleNum;
		gyroDataRaw[1] = gyroDataRaw[1] / sampleNum;
		gyroDataRaw[2] = gyroDataRaw[2] / sampleNum;
	}
	return ESP_OK;
}

esp_err_t parse_acc_data(const uint8_t sampleNum, int32_t *accDataRaw){
	if(sampleNum == 0){
		return ESP_FAIL;
	}
	else{
		accDataRaw[0] = accDataRaw[0] / (int32_t)sampleNum;
		accDataRaw[1] = accDataRaw[1] / (int32_t)sampleNum;
		accDataRaw[2] = accDataRaw[2] / (int32_t)sampleNum;
	}
	return ESP_OK;
}


esp_err_t calc_acc(uint8_t sensor, int32_t *rawData){
	LSM6DSO32_d[sensor].meas.accX  = rawData[0]*(LSM6DSO32_d[sensor].config.LSM6DSAccSensMgPerLsbCurrent) - LSM6DSO32_d[sensor].accXoffset;
	LSM6DSO32_d[sensor].meas.accY  = rawData[1]*(LSM6DSO32_d[sensor].config.LSM6DSAccSensMgPerLsbCurrent) - LSM6DSO32_d[sensor].accYoffset;
	LSM6DSO32_d[sensor].meas.accZ  = rawData[2]*(LSM6DSO32_d[sensor].config.LSM6DSAccSensMgPerLsbCurrent) - LSM6DSO32_d[sensor].accZoffset;
	return ESP_OK;
}
esp_err_t calc_gyro(uint8_t sensor, int32_t *rawData){
	LSM6DSO32_d[sensor].meas.gyroX = ((rawData[0] * LSM6DSO32_d[sensor].config.LSM6DSGyroDpsPerLsb) - LSM6DSO32_d[sensor].gyroXoffset);
	LSM6DSO32_d[sensor].meas.gyroY = ((rawData[1] * LSM6DSO32_d[sensor].config.LSM6DSGyroDpsPerLsb) - LSM6DSO32_d[sensor].gyroYoffset);
	LSM6DSO32_d[sensor].meas.gyroZ = ((rawData[2] * LSM6DSO32_d[sensor].config.LSM6DSGyroDpsPerLsb) - LSM6DSO32_d[sensor].gyroZoffset);
	return ESP_OK;
}

