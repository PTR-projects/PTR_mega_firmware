#include <stdio.h>
#include "BOARD.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "Analog_driver.h"

#define GET_UNIT(x)        ((x>>3) & 0x1)

esp_adc_cal_characteristics_t  * adc_chars;

esp_err_t Analog_init(void)
{
	//Check if TP is burned into eFuse
	if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
		printf("eFuse Two Point: Supported\n");
	} else {
		printf("eFuse Two Point: NOT supported\n");
	}
	//Check Vref is burned into eFuse
	if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
		printf("eFuse Vref: Supported\n");
	} else {
		printf("eFuse Vref: NOT supported\n");
	}

	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_11); //IGN4
	adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_11); //IGN1
	adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_11); //IGN2
	adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11); //IGN3
	adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11); //BAT

	//Characterize ADC
	adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, adc_chars);
	//print_char_val_type(val_type);
	return ESP_OK;
}

uint32_t Analog_getIGN1(){
    uint32_t adc_reading = adc1_get_raw((adc1_channel_t)ADC1_CHANNEL_4);
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    printf("Raw 1: %d\tVoltage: %dmV\n", adc_reading, voltage);

    return voltage;
}

uint32_t Analog_getIGN2(){
    uint32_t adc_reading = adc1_get_raw((adc1_channel_t)ADC1_CHANNEL_5);
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    printf("Raw 2: %d\tVoltage: %dmV\n", adc_reading, voltage);

    return voltage;
}

uint32_t Analog_getIGN3(){
    uint32_t adc_reading = adc1_get_raw((adc1_channel_t)ADC1_CHANNEL_6);
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    printf("Raw 3: %d\tVoltage: %dmV\n", adc_reading, voltage);

    return voltage;
}

uint32_t Analog_getIGN4(){
    uint32_t adc_reading = adc1_get_raw((adc1_channel_t)ADC1_CHANNEL_3);
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    printf("Raw 4: %d\tVoltage: %dmV\n", adc_reading, voltage);

    return voltage;
}

uint32_t Analog_getVBAT(){
    uint32_t adc_reading = adc1_get_raw((adc1_channel_t)ADC1_CHANNEL_7);
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    printf("Raw bat: %d\tVoltage: %dmV\n", adc_reading, voltage);

    return voltage;
}
