#include <stdio.h>
#include "BOARD.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/temp_sensor.h"
#include "esp_adc_cal.h"
#include "Analog_driver.h"

#define GET_UNIT(x)        ((x>>3) & 0x1)

static const char* TAG = "Analog";

static esp_adc_cal_characteristics_t  * adc_chars;
static uint32_t ign_det_thr = 100;

static float    filter_coeff = 0.1f;
static uint32_t voltage_ign1 = 0;
static uint32_t voltage_ign2 = 0;
static uint32_t voltage_ign3 = 0;
static uint32_t voltage_ign4 = 0;
static uint32_t voltage_vbat = 0;

esp_err_t Analog_init(uint32_t ign_det_thr_val, float filter)
{
	ign_det_thr = ign_det_thr_val;
	filter_coeff = filter;

	//Check if TP is burned into eFuse
	if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
		ESP_LOGI(TAG, "eFuse Two Point: Supported\n");
	} else {
		ESP_LOGI(TAG, "eFuse Two Point: NOT supported\n");
	}
	//Check Vref is burned into eFuse
	if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
		ESP_LOGI(TAG, "eFuse Vref: Supported\n");
	} else {
		ESP_LOGI(TAG, "eFuse Vref: NOT supported\n");
	}

	//Check TP+Vref is burned into eFuse
	if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP_FIT) == ESP_OK) {
		ESP_LOGI(TAG, "eFuse Two Point+Vref: Supported\n");
	} else {
		ESP_LOGI(TAG, "eFuse Point+Vref: NOT supported\n");
	}

	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_6); //IGN4
	adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_6); //IGN1
	adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_6); //IGN2
	adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_6); //IGN3
	adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_6); //BAT

	//Characterize ADC
	adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_6, ADC_WIDTH_BIT_12, 1100, adc_chars);

	//-------------- Temp sensor init -------------------------------------
	temp_sensor_config_t temp_sensor = TSENS_CONFIG_DEFAULT();
	temp_sensor.dac_offset = TSENS_DAC_L2;  //TSENS_DAC_L2 is default   L4(-40℃ ~ 20℃), L2(-10℃ ~ 80℃) L1(20℃ ~ 100℃) L0(50℃ ~ 125℃)
	temp_sensor_set_config(temp_sensor);
	temp_sensor_start();

	// Init meas filters
	voltage_ign1 = esp_adc_cal_raw_to_voltage(adc1_get_raw((adc1_channel_t)ADC1_CHANNEL_6), adc_chars);
	voltage_ign2 = esp_adc_cal_raw_to_voltage(adc1_get_raw((adc1_channel_t)ADC1_CHANNEL_5), adc_chars);
	voltage_ign3 = esp_adc_cal_raw_to_voltage(adc1_get_raw((adc1_channel_t)ADC1_CHANNEL_4), adc_chars);
	voltage_ign4 = esp_adc_cal_raw_to_voltage(adc1_get_raw((adc1_channel_t)ADC1_CHANNEL_3), adc_chars);
	voltage_vbat = esp_adc_cal_raw_to_voltage(adc1_get_raw((adc1_channel_t)ADC1_CHANNEL_7), adc_chars) * 11;

	return ESP_OK;
}

uint32_t Analog_getIGN1(){
    uint32_t adc_reading = adc1_get_raw((adc1_channel_t)ADC1_CHANNEL_6);
    uint32_t voltage 	 = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    ESP_LOGV(TAG, "Raw 1: %d\tVoltage: %dmV", adc_reading, voltage);

    voltage_ign1 = filter_coeff * voltage + (1-filter_coeff) * voltage_ign1;

    return voltage_ign1;
}

uint32_t Analog_getIGN2(){
    uint32_t adc_reading = adc1_get_raw((adc1_channel_t)ADC1_CHANNEL_5);
    uint32_t voltage 	 = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    ESP_LOGV(TAG, "Raw 2: %d\tVoltage: %dmV", adc_reading, voltage);

    voltage_ign2 = filter_coeff * voltage + (1-filter_coeff) * voltage_ign2;

    return voltage_ign2;
}

uint32_t Analog_getIGN3(){
    uint32_t adc_reading = adc1_get_raw((adc1_channel_t)ADC1_CHANNEL_4);
    uint32_t voltage 	 = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    ESP_LOGV(TAG, "Raw 3: %d\tVoltage: %dmV", adc_reading, voltage);

    voltage_ign3 = filter_coeff * voltage + (1-filter_coeff) * voltage_ign3;

    return voltage_ign3;
}

uint32_t Analog_getIGN4(){
    uint32_t adc_reading = adc1_get_raw((adc1_channel_t)ADC1_CHANNEL_3);
    uint32_t voltage 	 = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    ESP_LOGV(TAG, "Raw 4: %d\tVoltage: %dmV", adc_reading, voltage);

    voltage_ign4 = filter_coeff * voltage + (1-filter_coeff) * voltage_ign4;

    return voltage_ign4;
}

uint32_t Analog_getVBAT(){
    uint32_t adc_reading = adc1_get_raw((adc1_channel_t)ADC1_CHANNEL_7);
    uint32_t voltage 	 = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars) * 11;
    ESP_LOGV(TAG, "Raw bat: %d\tVoltage: %dmV", adc_reading, voltage);

    voltage_vbat = filter_coeff * voltage + (1-filter_coeff) * voltage_vbat;

    return voltage_vbat;
}

float Analog_getTempMCU(){
	float result = 0;
	temp_sensor_read_celsius(&result);

	ESP_LOGV(TAG, "Temp: %.2f Celsius", result);
	ESP_LOGE("TAG", "Internal temp. sensor not accurate!");

	return 0.0f;
}

void Analog_update(Analog_meas_t * meas){
	meas->vbat_mV = Analog_getVBAT();

	//TODO support variable threshold and fuse check
	if(meas->vbat_mV > 2000){
		meas->IGN1_det = (Analog_getIGN1() < ign_det_thr);
		meas->IGN2_det = (Analog_getIGN2() < ign_det_thr);
		meas->IGN3_det = (Analog_getIGN3() < ign_det_thr);
		meas->IGN4_det = (Analog_getIGN4() < ign_det_thr);
	} else {
		meas->IGN1_det = 0;
		meas->IGN2_det = 0;
		meas->IGN3_det = 0;
		meas->IGN4_det = 0;
	}

	//meas->temp = Analog_getTempMCU();
}
