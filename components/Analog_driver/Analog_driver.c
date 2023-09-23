#include <stdio.h>
//#include "BOARD.h"
#include "esp_err.h"
#include "esp_log.h"

#include "driver/temperature_sensor.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include "Analog_driver.h"

//--------- ULP -----------
//#include "driver/rtc_io.h"
//#include "esp32s3/ulp.h"
//#include "esp32s3/ulp_riscv.h"
//#include "esp32s3/ulp_riscv_adc.h"
//#include "main_ulp_adc.h"
//extern const uint8_t ulp_main_bin_start[] asm("_binary_main_ulp_adc_bin_start");
//extern const uint8_t ulp_main_bin_end[]   asm("_binary_main_ulp_adc_bin_end");
//static void init_ulp_program(void);


#define GET_UNIT(x)        ((x>>3) & 0x1)

static const char* TAG = "Analog";

static uint32_t ign_det_thr = 50;

static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t adc1_cali_chan6_handle = NULL;
static adc_cali_handle_t adc1_cali_chan5_handle = NULL;
static adc_cali_handle_t adc1_cali_chan4_handle = NULL;
static adc_cali_handle_t adc1_cali_chan3_handle = NULL;
static adc_cali_handle_t adc1_cali_chan7_handle = NULL;

static temperature_sensor_handle_t temp_sensor = NULL;


static float    filter_coeff	 = 0.6f;
static float    filter_coeff_ign = 0.5f;
static int voltage_ign1 = 0;
static int voltage_ign2 = 0;
static int voltage_ign3 = 0;
static int voltage_ign4 = 0;
static int voltage_vbat = 0;
static float	mcu_temp	 = 0.0f;
static uint32_t vbat_mV_raw = 0;


uint32_t Analog_getIGN1(uint32_t vbat);
uint32_t Analog_getIGN2(uint32_t vbat);
uint32_t Analog_getIGN3(uint32_t vbat);
uint32_t Analog_getIGN4(uint32_t vbat);
uint32_t Analog_getVBAT();

esp_err_t Analog_init(uint32_t ign_det_thr_val, float filter)
{
	ign_det_thr = ign_det_thr_val;
	filter_coeff = filter;

	//-------------ADC1 Init---------------//
	adc_oneshot_unit_init_cfg_t init_config1 = {
		.unit_id = ADC_UNIT_1,
	};
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

	//-------------ADC1 Config---------------//
	adc_oneshot_chan_cfg_t config = {
		.bitwidth 	= ADC_BITWIDTH_DEFAULT,
		.atten 		= ADC_ATTEN_DB_2_5,
	};
	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_6, &config));
	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_5, &config));
	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_4, &config));
	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_3, &config));
	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_7, &config));

	//-------------ADC1 Calibration Init---------------//

//	bool do_calibration1_chan6 = example_adc_calibration_init(ADC_UNIT_1, ADC_CHANNEL_6, ADC_ATTEN_DB_2_5, &adc1_cali_chan6_handle);//IGN1
//	bool do_calibration1_chan5 = example_adc_calibration_init(ADC_UNIT_1, ADC_CHANNEL_5, ADC_ATTEN_DB_2_5, &adc1_cali_chan5_handle);//IGN2
//	bool do_calibration1_chan4 = example_adc_calibration_init(ADC_UNIT_1, ADC_CHANNEL_4, ADC_ATTEN_DB_2_5, &adc1_cali_chan4_handle);//IGN3
//	bool do_calibration1_chan3 = example_adc_calibration_init(ADC_UNIT_1, ADC_CHANNEL_3, ADC_ATTEN_DB_2_5, &adc1_cali_chan3_handle);//IGN4
//	bool do_calibration1_chan7 = example_adc_calibration_init(ADC_UNIT_1, ADC_CHANNEL_7, ADC_ATTEN_DB_2_5, &adc1_cali_chan7_handle);//BAT

	//Install temperature sensor, expected temp ranger range: 10~50 ℃
	temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 50);
	ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor_config, &temp_sensor));
	ESP_ERROR_CHECK(temperature_sensor_enable(temp_sensor));

	// Init meas filters
	adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, &voltage_ign1);
	adc_oneshot_read(adc1_handle, ADC_CHANNEL_5, &voltage_ign2);
	adc_oneshot_read(adc1_handle, ADC_CHANNEL_4, &voltage_ign3);
	adc_oneshot_read(adc1_handle, ADC_CHANNEL_3, &voltage_ign4);
	adc_oneshot_read(adc1_handle, ADC_CHANNEL_7, &voltage_vbat);

	adc_cali_raw_to_voltage(adc1_cali_chan6_handle, voltage_ign1, &voltage_ign1);
	adc_cali_raw_to_voltage(adc1_cali_chan5_handle, voltage_ign2, &voltage_ign2);
	adc_cali_raw_to_voltage(adc1_cali_chan4_handle, voltage_ign3, &voltage_ign3);
	adc_cali_raw_to_voltage(adc1_cali_chan3_handle, voltage_ign4, &voltage_ign4);
	adc_cali_raw_to_voltage(adc1_cali_chan7_handle, voltage_vbat, &voltage_vbat);
	voltage_vbat *= 11;

	temperature_sensor_get_celsius(temp_sensor, &mcu_temp);

	return ESP_OK;
}

uint32_t Analog_getIGN1(uint32_t vbat){
    int adc_reading = 0;
    int voltage 	 = 0;

    adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, &adc_reading);
    adc_cali_raw_to_voltage(adc1_cali_chan6_handle, adc_reading, &voltage);

    ESP_LOGV(TAG, "Raw 1: %d\tVoltage: %dmV", adc_reading, voltage);

    voltage_ign1 = filter_coeff_ign * voltage + (1.0f-filter_coeff_ign) * voltage_ign1;

    return voltage_ign1;
}

uint32_t Analog_getIGN2(uint32_t vbat){
	int adc_reading = 0;
	int voltage 	 = 0;

	adc_oneshot_read(adc1_handle, ADC_CHANNEL_5, &adc_reading);
	adc_cali_raw_to_voltage(adc1_cali_chan5_handle, adc_reading, &voltage);

    ESP_LOGV(TAG, "Raw 2: %d\tVoltage: %dmV", adc_reading, voltage);

    voltage_ign2 = filter_coeff_ign * voltage + (1.0f-filter_coeff_ign) * voltage_ign2;

    return voltage_ign2;
}

uint32_t Analog_getIGN3(uint32_t vbat){
	int adc_reading = 0;
	int voltage 	 = 0;

	adc_oneshot_read(adc1_handle, ADC_CHANNEL_4, &adc_reading);
	adc_cali_raw_to_voltage(adc1_cali_chan4_handle, adc_reading, &voltage);

    ESP_LOGV(TAG, "Raw 3: %d\tVoltage: %dmV", adc_reading, voltage);

    voltage_ign3 = filter_coeff_ign * voltage + (1.0f-filter_coeff_ign) * voltage_ign3;

    return voltage_ign3;
}

uint32_t Analog_getIGN4(uint32_t vbat){
	int adc_reading = 0;
	int voltage 	 = 0;

	adc_oneshot_read(adc1_handle, ADC_CHANNEL_3, &adc_reading);
	adc_cali_raw_to_voltage(adc1_cali_chan3_handle, adc_reading, &voltage);

    ESP_LOGV(TAG, "Raw 4: %d\tVoltage: %dmV", adc_reading, voltage);

    voltage_ign4 = filter_coeff_ign * voltage + (1.0f-filter_coeff_ign) * voltage_ign4;

    return voltage_ign4;
}

uint32_t Analog_getVBAT(){
	int adc_single  = 0;
	int adc_reading = 0;
	int voltage_raw = 0;
	float voltage 	= 0.0f;

	for(uint8_t i=0; i<16; i++){
		adc_oneshot_read(adc1_handle, ADC_CHANNEL_7, &adc_reading);
	}

	for(uint16_t i=0; i<1024; i++){
		adc_oneshot_read(adc1_handle, ADC_CHANNEL_7, &adc_single);
		adc_reading += adc_single;
	}

	adc_reading >>= 10;
	adc_cali_raw_to_voltage(adc1_cali_chan7_handle, adc_reading, &voltage_raw);
	voltage = (float)voltage_raw *11.0f * 1.024f;
    ESP_LOGV(TAG, "Raw bat: %d\tVoltage: %.0fmV", adc_reading, voltage);

    voltage_vbat = filter_coeff * voltage + (1.0f-filter_coeff) * voltage_vbat;
    vbat_mV_raw = voltage;

    static float raw = 0.0f;
    raw = (float)adc_reading * 0.05f + raw * 0.95f;

    static float bat = 0.0f;
    bat = (float)voltage * 0.05f + bat * 0.95f;

    ESP_LOGV(TAG, "Vbat %.0fV,  \t%u LSB", voltage, adc_reading);
    return voltage_vbat;
}

float Analog_getTempMCU(){
	float result = 0.0f;
	temperature_sensor_get_celsius(temp_sensor, &result);
	mcu_temp = filter_coeff * result + (1-filter_coeff) * mcu_temp;
	ESP_LOGV(TAG, "MCU temp: %.2f", mcu_temp);

	return mcu_temp;
}

void Analog_update(Analog_meas_t * meas){
	meas->vbat_mV = Analog_getVBAT();

	//TODO support variable threshold and fuse check
	if(vbat_mV_raw > 3200){
		ign_det_thr = (meas->vbat_mV*12 - 12619)/1000;
		meas->IGN1_det = (Analog_getIGN1(vbat_mV_raw) < ign_det_thr);
		meas->IGN2_det = (Analog_getIGN2(vbat_mV_raw) < ign_det_thr);
		meas->IGN3_det = (Analog_getIGN3(vbat_mV_raw) < ign_det_thr);
		meas->IGN4_det = (Analog_getIGN4(vbat_mV_raw) < ign_det_thr);
	} else if(meas->vbat_mV < 3200){
		meas->IGN1_det = -1;
		meas->IGN2_det = -1;
		meas->IGN3_det = -1;
		meas->IGN4_det = -1;
	}

	meas->temp = Analog_getTempMCU();
}

//---------------------------------- ULP ---------------------------------
//static void init_ulp_program(void)
//{
//	ulp_riscv_adc_cfg_t cfg = {
//		.channel = ADC1_CHANNEL_7,
//		.width   = ADC_WIDTH_BIT_12,
//		.atten   = ADC_ATTEN_DB_2_5,
//	};
//	ESP_ERROR_CHECK(ulp_riscv_adc_init(&cfg));
//
//    esp_err_t err = ulp_riscv_load_binary(ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start));
//    ESP_ERROR_CHECK(err);
//
//    /* The first argument is the period index, which is not used by the ULP-RISC-V timer
//     * The second argument is the period in microseconds, which gives a wakeup time period of: 20ms
//     */
//    ulp_set_wakeup_period(0, 20000);
//
//    /* Start the program */
//    err = ulp_riscv_run();
//    ESP_ERROR_CHECK(err);
//}
