/**
  ******************************************************************************
  * @file    LIS331_driver.c
  * @author  Kretu
  * @brief   LIS331 driver file
  ******************************************************************************
 */

#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "SPI_driver.h"
#include "LIS331_driver.h"
#include <string.h>
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "BOARD_cfg.h"

#define INIT_TIME       5 		//ms

static const char *TAG = "LIS331";

#if !defined SPI_SLAVE_LIS331_0_PIN
esp_err_t 	LIS331_init(LIS331_range_e range) {return ESP_OK;}

esp_err_t LIS331_readMeas() {return ESP_OK;}
esp_err_t LIS331_getMeas(uint8_t sensor, LIS331_meas_t * meas) {return ESP_OK;}
esp_err_t LIS331_getMeasurementXYZ(uint8_t sensor, float* X, float* Y, float* Z) {return ESP_OK;}

esp_err_t		 	LIS331_x_axis_set(uint8_t sensor, bool val) {return ESP_OK;}
bool 				LIS331_x_axis_get(uint8_t sensor) {return false;}
esp_err_t 			LIS331_y_axis_set(uint8_t sensor, bool val) {return ESP_OK;}
bool 				LIS331_y_axis_get(uint8_t sensor) {return false;}
esp_err_t 			LIS331_z_axis_set(uint8_t sensor, bool val) {return ESP_OK;}
bool 				LIS331_z_axis_get(uint8_t sensor) {return false;}

esp_err_t 			LIS331_power_mode_set(uint8_t sensor, LIS331_power_mode_t val) {return ESP_OK;}
LIS331_power_mode_t LIS331_power_mode_get(uint8_t sensor) {return 0;}
esp_err_t 			LIS331_data_rate_set(uint8_t sensor, LIS331_data_rate_t val) {return ESP_OK;}
LIS331_data_rate_t 	LIS331_data_rate_get(uint8_t sensor) {return 0;}
esp_err_t 			LIS331_boot(uint8_t sensor) {return ESP_OK;}
esp_err_t 			LIS331_hp_filter_set(uint8_t sensor, LIS331_hp_mode_t val) {return ESP_OK;}
LIS331_hp_mode_t 	LIS331_hp_filter_get(uint8_t sensor) {return 0;}
esp_err_t 			LIS331_hp_en_set(uint8_t sensor, uint8_t interrupt, bool val) {return ESP_OK;}
bool 				LIS331_hp_en_get(uint8_t sensor, uint8_t interrupt) {return false;}
esp_err_t 			LIS331_hp_cutoff_set(uint8_t sensor, LIS331_hp_cutoff_t val) {return ESP_OK;}
LIS331_hp_cutoff_t 	LIS331_hp_cutoff_get(uint8_t sensor) {return 0;}
esp_err_t 			LIS331_bdu_set(uint8_t sensor, bool val) {return ESP_OK;}
bool 				LIS331_bdu_get(uint8_t sensor) {return false;}
esp_err_t 			LIS331_ble_set(uint8_t sensor, bool val) {return ESP_OK;}
bool 				LIS331_ble_get(uint8_t sensor) {return false;}
esp_err_t 			LIS331_range_set(uint8_t sensor, LIS331_range_t val) {return ESP_OK;}
LIS331_range_t 		LIS331_range_get(uint8_t sensor) {return 0;}
#else

static LIS331_t LIS331_d[LIS331_COUNT];
static const int SPI_SLAVE_LIS331_PIN_ARRAY[LIS331_COUNT] = SPI_SLAVE_LIS331_PINS;
static const int LIS331_TYPE_ARRAY[LIS331_COUNT] = LIS331_TYPES;

static esp_err_t LIS331_spi_init(uint8_t sensor);
static esp_err_t LIS331_read(uint8_t sensor, uint8_t addr, uint8_t * data_in, uint16_t length);
static uint8_t   LIS331_read_single(uint8_t sensor, uint8_t addr);
static esp_err_t LIS331_write(uint8_t sensor, uint8_t addr, uint8_t data_out);
static esp_err_t LIS331_set(uint8_t sensor, uint8_t reg_addr, uint8_t cmd_mask, uint8_t cmd_value);
static uint8_t   LIS331_get(uint8_t sensor, uint8_t reg_addr, uint8_t cmd_mask);
static uint8_t   LIS331_WhoAmI(uint8_t sensor);

static esp_err_t LIS331_spi_init(uint8_t sensor)
{
	if(SPI_checkInit() != ESP_OK){
		ESP_LOGE(TAG, "SPI controller not initialized! Use SPI_init() in main.c");
		return ESP_ERR_INVALID_STATE;
	}

	/* CONFIGURE SPI DEVICE */
	/* Max SCK frequency - 10MHz */
	ESP_RETURN_ON_ERROR(SPI_registerDevice(&(LIS331_d[sensor].spi_handle), SPI_SLAVE_LIS331_PIN_ARRAY[sensor],
										SPI_SCK_10MHZ, 1, 2, 6), TAG, "SPI register failed");

	return ESP_OK;
}

static esp_err_t LIS331_read(uint8_t sensor, uint8_t addr, uint8_t * data_in, uint16_t length){
	return SPI_transfer(LIS331_d[sensor].spi_handle, LIS331_CMD_READ, addr, NULL, data_in, length);
}

static uint8_t LIS331_read_single(uint8_t sensor, uint8_t addr){
	uint8_t buf = 0;
	SPI_transfer(LIS331_d[sensor].spi_handle, LIS331_CMD_READ, addr, NULL, &buf, 1);

	return buf;
}

static esp_err_t LIS331_write(uint8_t sensor, uint8_t addr, uint8_t data_out){
	return SPI_transfer(LIS331_d[sensor].spi_handle, LIS331_CMD_WRITE, addr, &data_out, NULL, 1);
}

static esp_err_t LIS331_set(uint8_t sensor, uint8_t reg_addr, uint8_t cmd_mask, uint8_t cmd_value)
{
	uint8_t reg = 0;
	reg = LIS331_read_single(sensor, reg_addr);
	reg &= ~cmd_mask;
	reg |= cmd_value;
	LIS331_write(sensor, reg_addr, reg);

	return ESP_OK;
}

static uint8_t LIS331_get(uint8_t sensor, uint8_t reg_addr, uint8_t cmd_mask)
{
	uint8_t reg = 0;
	reg = LIS331_read_single(sensor, reg_addr);

	return (reg & cmd_mask);
}

static uint8_t LIS331_WhoAmI(uint8_t sensor)
{
	uint8_t whois = 0;
	LIS331_read(sensor, LIS331_WHO_AM_I, &whois, 1);

	return whois;
}

#define LIS331_WHO_AM_I_VALUE	0x32
#define LIS331_SANITY_SAMPLES	50
#define LIS331_AXIS_LIMIT_G		3.0f

static float LIS331_stddev(const int16_t *values, int count)
{
	double mean = 0.0;
	double accum = 0.0;

	for(int i = 0; i < count; i++){
		mean += (double)values[i];
	}
	mean /= (double)count;
	for(int i = 0; i < count; i++){
		double delta = (double)values[i] - mean;
		accum += delta * delta;
	}
	return (float)sqrt(accum / (double)count);
}

static esp_err_t LIS331_waitSample(uint8_t sensor)
{
	for(int attempt = 0; attempt < 20; attempt++){
		uint8_t status = LIS331_read_single(sensor, LIS331_STATUS_REG);
		if((status & (LIS331_XOR_MASK | LIS331_YOR_MASK | LIS331_ZOR_MASK)) != 0){
			ESP_LOGE(TAG, "LIS331 %u overflow status 0x%02x", sensor, status);
			return ESP_FAIL;
		}
		if((status & LIS331_ZYXDA_MASK) != 0){
			return ESP_OK;
		}
		vTaskDelay(pdMS_TO_TICKS(1));
	}
	ESP_LOGE(TAG, "LIS331 %u data-ready timeout", sensor);
	return ESP_ERR_TIMEOUT;
}

static esp_err_t LIS331_sanity(void)
{
	static int16_t raw_x[LIS331_COUNT][LIS331_SANITY_SAMPLES];
	static int16_t raw_y[LIS331_COUNT][LIS331_SANITY_SAMPLES];
	static int16_t raw_z[LIS331_COUNT][LIS331_SANITY_SAMPLES];

	for(uint8_t sensor = 0; sensor < LIS331_COUNT; sensor++){
		uint8_t who = LIS331_WhoAmI(sensor);
		if(who != LIS331_WHO_AM_I_VALUE){
			ESP_LOGE(TAG, "LIS331 %u WHO_AM_I 0x%02x", sensor, who);
			return ESP_FAIL;
		}
	}

	for(int sample = 0; sample < LIS331_SANITY_SAMPLES; sample++){
		for(uint8_t sensor = 0; sensor < LIS331_COUNT; sensor++){
			if(LIS331_waitSample(sensor) != ESP_OK){
				return ESP_FAIL;
			}
		}
		if(LIS331_readMeas() != ESP_OK){
			return ESP_FAIL;
		}
		for(uint8_t sensor = 0; sensor < LIS331_COUNT; sensor++){
			float acc_x = LIS331_d[sensor].meas.accX;
			float acc_y = LIS331_d[sensor].meas.accY;
			float acc_z = LIS331_d[sensor].meas.accZ;
			if((fabsf(acc_x) > LIS331_AXIS_LIMIT_G) || (fabsf(acc_y) > LIS331_AXIS_LIMIT_G) ||
					(fabsf(acc_z) > LIS331_AXIS_LIMIT_G)){
				ESP_LOGE(TAG, "LIS331 %u sample %d outside +/-3 g: %d %d %d mg",
						sensor, sample, (int)(acc_x * 1000.0f), (int)(acc_y * 1000.0f), (int)(acc_z * 1000.0f));
				return ESP_FAIL;
			}
			raw_x[sensor][sample] = LIS331_d[sensor].accX_raw;
			raw_y[sensor][sample] = LIS331_d[sensor].accY_raw;
			raw_z[sensor][sample] = LIS331_d[sensor].accZ_raw;
		}
	}

	for(uint8_t sensor = 0; sensor < LIS331_COUNT; sensor++){
		if((LIS331_stddev(raw_x[sensor], LIS331_SANITY_SAMPLES) <= 0.0f) ||
				(LIS331_stddev(raw_y[sensor], LIS331_SANITY_SAMPLES) <= 0.0f) ||
				(LIS331_stddev(raw_z[sensor], LIS331_SANITY_SAMPLES) <= 0.0f)){
			ESP_LOGE(TAG, "LIS331 %u axis samples are stuck", sensor);
			return ESP_FAIL;
		}
	}

	ESP_LOGI(TAG, "LIS331 sanity passed");
	return ESP_OK;
}

esp_err_t LIS331_init(LIS331_range_e range)
{
	for(uint8_t sensor = 0; LIS331_COUNT > sensor ; sensor++){
		LIS331_spi_init(sensor);
		ESP_LOGI(TAG, "LIS331 SPI init done");

		LIS331_write(sensor, LIS331_CTRL_REG1, 0x37);	//XYZ enabled, Normal mode, 400Hz ODR
		LIS331_write(sensor, LIS331_CTRL_REG4, (range << 4) | LIS331_BDU_MASK);

		if(range == LIS331_RANGE_LOW){
			LIS331_d[sensor].sensor_range = LIS331_TYPE_ARRAY[sensor];
		}
		else if(range == LIS331_RANGE_MID){
			LIS331_d[sensor].sensor_range = 2*LIS331_TYPE_ARRAY[sensor];
		}
		else if(range == LIS331_RANGE_MID){
			LIS331_d[sensor].sensor_range = 4*LIS331_TYPE_ARRAY[sensor];
		}
		else{
			return ESP_FAIL;
		}
	}

	return LIS331_sanity();
}


esp_err_t LIS331_x_axis_set(uint8_t sensor, bool val)
{
	if (!val){
		LIS331_set(sensor, LIS331_CTRL_REG1, LIS331_XEN_MASK , 0x0);
	}
	else
	{
		LIS331_set(sensor, LIS331_CTRL_REG1, LIS331_XEN_MASK , LIS331_XEN_MASK);
	}
	return ESP_OK;
}

bool LIS331_x_axis_get(uint8_t sensor)
{
	return LIS331_get(sensor, LIS331_CTRL_REG1, LIS331_XEN_MASK) == LIS331_XEN_MASK;
}


esp_err_t LIS331_y_axis_set(uint8_t sensor, bool val)
{
	if (!val){
		LIS331_set(sensor, LIS331_CTRL_REG1, LIS331_YEN_MASK , 0x0);
	}
	else
	{
		LIS331_set(sensor, LIS331_CTRL_REG1, LIS331_YEN_MASK , LIS331_YEN_MASK);
	}
	return ESP_OK;
}

bool LIS331_y_axis_get(uint8_t sensor)
{
	return LIS331_get(sensor, LIS331_CTRL_REG1, LIS331_YEN_MASK) == LIS331_YEN_MASK;
}

esp_err_t LIS331_z_axis_set(uint8_t sensor, bool val)
{
	if (!val){
		LIS331_set(sensor, LIS331_CTRL_REG1, LIS331_ZEN_MASK , 0x0);
	}
	else
	{
		LIS331_set(sensor, LIS331_CTRL_REG1, LIS331_ZEN_MASK , LIS331_ZEN_MASK);
	}
	return ESP_OK;
}

bool LIS331_z_axis_get(uint8_t sensor)
{
	return LIS331_get(sensor, LIS331_CTRL_REG1, LIS331_ZEN_MASK) == LIS331_ZEN_MASK;
}

esp_err_t LIS331_power_mode_set(uint8_t sensor, LIS331_power_mode_t val)
{
	LIS331_set(sensor, LIS331_CTRL_REG1, LIS331_POWER_MODE_MASK , val);

	return ESP_OK;
}

LIS331_power_mode_t LIS331_power_mode_get(uint8_t sensor)
{
	return LIS331_get(sensor, LIS331_CTRL_REG1, LIS331_POWER_MODE_MASK);
}

esp_err_t LIS331_data_rate_set(uint8_t sensor, LIS331_data_rate_t val)
{
	LIS331_set(sensor, LIS331_CTRL_REG1, LIS331_DATA_RATE_MASK , val);

	return ESP_OK;
}

LIS331_data_rate_t LIS331_data_rate_get(uint8_t sensor)
{
	return LIS331_get(sensor, LIS331_CTRL_REG1, LIS331_DATA_RATE_MASK);
}

esp_err_t LIS331_xyz_acc_raw_get(uint8_t sensor)
{
	ESP_LOGV(TAG, "Raw read start\n");

  LIS331_read(sensor, LIS331_OUT_X_L, LIS331_d[sensor].raw, 6);
  LIS331_d[sensor].accX_raw = LIS331_d[sensor].accX_raw >> 4;
  LIS331_d[sensor].accY_raw = LIS331_d[sensor].accY_raw >> 4;
  LIS331_d[sensor].accZ_raw = LIS331_d[sensor].accZ_raw >> 4;
  ESP_LOGV(TAG, "Raw read ok\n");

  return ESP_OK;
}

esp_err_t LIS331_readMeas()
{
	for(uint8_t sensor = 0; LIS331_COUNT > sensor ; sensor++){
		LIS331_xyz_acc_raw_get(sensor);
		float range = 2 * LIS331_d[sensor].sensor_range;
		if(range == 0.0f)
			return ESP_ERR_INVALID_ARG;

		LIS331_d[sensor].meas.accX = (LIS331_d[sensor].accX_raw)*(range/4096.0f) - LIS331_d[sensor].accXoffset;
		LIS331_d[sensor].meas.accY = (LIS331_d[sensor].accY_raw)*(range/4096.0f) - LIS331_d[sensor].accYoffset;
		LIS331_d[sensor].meas.accZ = (LIS331_d[sensor].accZ_raw)*(range/4096.0f) - LIS331_d[sensor].accZoffset;
	}

	return ESP_OK;
}

esp_err_t LIS331_getMeas(uint8_t sensor, LIS331_meas_t * meas){
	*meas = LIS331_d[sensor].meas;

	return ESP_OK;	//ESP_FAIL
}


esp_err_t LIS331_getMeasurementXYZ(uint8_t sensor, float* X, float* Y, float* Z)
{
	LIS331_readMeas(sensor);
	*X = LIS331_d[sensor].meas.accX;
	*Y = LIS331_d[sensor].meas.accY;
	*Z = LIS331_d[sensor].meas.accZ;

	return ESP_OK;
}

esp_err_t LIS331_boot(uint8_t sensor)
{
	LIS331_set(sensor, LIS331_CTRL_REG2, LIS331_BOOT_MASK , LIS331_BOOT_MASK);

  return ESP_OK;
}


esp_err_t LIS331_hp_filter_set(uint8_t sensor, LIS331_hp_mode_t val)
{
	LIS331_set(sensor, LIS331_CTRL_REG2, LIS331_HP_MODE_MASK , val);

  return ESP_OK;
}

LIS331_hp_mode_t LIS331_hp_filter_get(uint8_t sensor)
{
	return LIS331_get(sensor, LIS331_CTRL_REG2, LIS331_HP_MODE_MASK);
}

esp_err_t LIS331_hp_en_set(uint8_t sensor, uint8_t interrupt, bool val)
{
	switch(interrupt){
	case 1:
		if (!val){
			LIS331_set(sensor, LIS331_CTRL_REG2, LIS331_HP_EN1_MASK, 0);
		}
		else
		{
			LIS331_set(sensor, LIS331_CTRL_REG2, LIS331_HP_EN1_MASK, LIS331_HP_EN1_MASK);
		}
		break;

	case 2:
		if (!val){
			LIS331_set(sensor, LIS331_CTRL_REG2, LIS331_HP_EN2_MASK, 0);
		}
		else
		{
			LIS331_set(sensor, LIS331_CTRL_REG2, LIS331_HP_EN2_MASK,LIS331_HP_EN2_MASK);
		}
		break;
	default:
		return ESP_ERR_INVALID_ARG;
	}
  return ESP_OK;
}

bool LIS331_hp_en_get(uint8_t sensor, uint8_t interrupt)
{
	switch(interrupt){
	case 1:
		return LIS331_get(sensor, LIS331_CTRL_REG2, LIS331_HP_EN1_MASK);

	case 2:
		return LIS331_get(sensor, LIS331_CTRL_REG2, LIS331_HP_EN2_MASK);
	default:
		return ESP_ERR_INVALID_ARG;
	}
}


esp_err_t LIS331_hp_cutoff_set(uint8_t sensor, LIS331_hp_cutoff_t val)
{
	LIS331_set(sensor, LIS331_CTRL_REG2, LIS331_HP_CUTOFF_MASK , val);

  return ESP_OK;
}

LIS331_hp_cutoff_t LIS331_hp_cutoff_get(uint8_t sensor)
{
	return LIS331_get(sensor, LIS331_CTRL_REG2, LIS331_HP_CUTOFF_MASK);
}


esp_err_t LIS331_bdu_set(uint8_t sensor, bool val)
{
	if (!val){
		LIS331_set(sensor, LIS331_CTRL_REG4, LIS331_BDU_MASK, 0);
	}
	else{
		LIS331_set(sensor, LIS331_CTRL_REG4, LIS331_BDU_MASK, LIS331_BDU_MASK);
	}
	return ESP_OK;
}

bool LIS331_bdu_get(uint8_t sensor)
{
	return LIS331_get(sensor, LIS331_CTRL_REG4, LIS331_BDU_MASK) == LIS331_BDU_MASK;
}

esp_err_t LIS331_ble_set(uint8_t sensor, bool val)
{
	if (!val){
		LIS331_set(sensor, LIS331_CTRL_REG4, LIS331_BLE_MASK, 0);
	}
	else{
		LIS331_set(sensor, LIS331_CTRL_REG4, LIS331_BLE_MASK, LIS331_BLE_MASK);
	}
	return ESP_OK;
}

bool LIS331_ble_get(uint8_t sensor)
{
	return LIS331_get(sensor, LIS331_CTRL_REG4, LIS331_BLE_MASK) == LIS331_BLE_MASK;
}



esp_err_t LIS331_range_set(uint8_t sensor, LIS331_range_t val)
{
	LIS331_set(sensor, LIS331_CTRL_REG4, LIS331_FS_MASK , val);
	switch(val){
	case LIS331_RANGE_100G:
		LIS331_d[sensor].sensor_range = 100;
		break;
	case LIS331_RANGE_200G:
		LIS331_d[sensor].sensor_range = 200;
		break;
	case LIS331_RANGE_400G:
		LIS331_d[sensor].sensor_range = 400;
		break;
	default:
		return ESP_ERR_INVALID_ARG;
	}

  return ESP_OK;
}

LIS331_range_t LIS331_range_get(uint8_t sensor)
{
	return LIS331_get(sensor, LIS331_CTRL_REG4, LIS331_FS_MASK);
}

#endif /* SPI_SLAVE_LIS331_0_PIN */
