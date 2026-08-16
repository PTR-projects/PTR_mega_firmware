#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "Preferences.h"
#include "SX126x_driver.h"
#include "LORA_driver.h"

static const char *TAG = "LORA driver";

static esp_err_t LORA_modeLORA(uint32_t frequency, int8_t txpower);
static uint16_t SX126X_readIrqStatus();

Lora_settings_t Lora_settings_d;

esp_err_t LORA_init()
{
	ESP_RETURN_ON_ERROR(SX126X_initIO(), TAG, "SX1262_initIO fail!");
	vTaskDelay(pdMS_TO_TICKS( 20 ));

	Preferences_data_t pref;
	if(Preferences_get(&pref) != ESP_OK){
		return ESP_FAIL;
	}

	Lora_settings_d.tx_dbm			= 0;	//(uint8_t)pref.lora_tx_dbm;
	Lora_settings_d.freq_hz 		= (uint32_t)pref.lora_freq_khz * 1000UL;	// kHz -> Hz
	Lora_settings_d.network_mode 	= (uint8_t)pref.lora_mode;
	Lora_settings_d.security_key	= (uint64_t)pref.lora_key;

	ESP_LOGI(TAG, "SX1262 init...");
	ESP_RETURN_ON_ERROR(LORA_modeLORA(Lora_settings_d.freq_hz,
			Lora_settings_d.tx_dbm), TAG, "Error setting LORA mode");

	ESP_LOGI(TAG, "SX1262 ready");

	return ESP_OK;
}

esp_err_t LORA_setupLoRaTX(uint32_t frequency, int32_t offset, uint8_t modParam1,
	uint8_t modParam2, uint8_t modParam3, uint8_t modParam4, uint8_t device) {

	sx126x_status_t status = sx126x_set_standby(0, SX126X_STANDBY_CFG_RC);


	if(status == SX126X_STATUS_OK)
		status = sx126x_set_reg_mode(0, SX126X_REG_MODE_DCDC);

	sx126x_pa_cfg_params_t sx126x_pa_cfg_params_d;
	sx126x_pa_cfg_params_d.pa_duty_cycle = 0x04;
	sx126x_pa_cfg_params_d.hp_max 	  	 = 0x07;		// SX1262 only
	sx126x_pa_cfg_params_d.device_sel 	 = 0x00;		// 1=SX1261, 0=SX1262
	sx126x_pa_cfg_params_d.pa_lut 	  	 = 0x01;		// always 0x01
	if(status == SX126X_STATUS_OK)
			status = sx126x_set_pa_cfg(0, &sx126x_pa_cfg_params_d);

	if(status == SX126X_STATUS_OK)
			status = sx126x_set_dio3_as_tcxo_ctrl(0, SX126X_TCXO_CTRL_3_3V, 100);
	vTaskDelay(pdMS_TO_TICKS( 5 ));

	if(status == SX126X_STATUS_OK)
			status = sx126x_cal(0, SX126X_CAL_ALL);	//is required after setting TCXO

	if(status == SX126X_STATUS_OK)
			status = sx126x_cal_img(0, frequency);

	if(status == SX126X_STATUS_OK)
			status = sx126x_set_dio2_as_rf_sw_ctrl(0, true);

	if(status == SX126X_STATUS_OK)
			status = sx126x_set_pkt_type(0, SX126X_PKT_TYPE_LORA);

	if(status == SX126X_STATUS_OK)
			status = sx126x_set_rf_freq(0, frequency);

	sx126x_mod_params_lora_t sx126x_mod_params_lora_d;
	sx126x_mod_params_lora_d.bw   = modParam2;
	sx126x_mod_params_lora_d.cr   = modParam3;
	sx126x_mod_params_lora_d.ldro = modParam4;
	sx126x_mod_params_lora_d.sf   = modParam1;
	if(status == SX126X_STATUS_OK)
			status = sx126x_set_lora_mod_params(0, &sx126x_mod_params_lora_d);
	if(status == SX126X_STATUS_OK)
			status = sx126x_set_buffer_base_address(0, 0, 0);

	sx126x_pkt_params_lora_t sx126x_pkt_params_lora_d;
	sx126x_pkt_params_lora_d.crc_is_on 				= true;
	sx126x_pkt_params_lora_d.header_type 			= SX126X_LORA_PKT_EXPLICIT;
	sx126x_pkt_params_lora_d.invert_iq_is_on 		= false;
	sx126x_pkt_params_lora_d.pld_len_in_bytes 		= 255;
	sx126x_pkt_params_lora_d.preamble_len_in_symb 	= 8;
	if(status == SX126X_STATUS_OK)
			status = sx126x_set_lora_pkt_params(0, &sx126x_pkt_params_lora_d);

	if(status == SX126X_STATUS_OK)
			status = sx126x_set_dio_irq_params(0, SX126X_IRQ_ALL,
								(SX126X_IRQ_TX_DONE + SX126X_IRQ_TIMEOUT+ SX126X_IRQ_RX_DONE),
								 SX126X_IRQ_NONE, SX126X_IRQ_NONE);

	if(status == SX126X_STATUS_OK)
		return ESP_OK;

	return ESP_FAIL;
}

static esp_err_t LORA_modeLORA(uint32_t frequency, int8_t txpower){
	if((frequency < 410000000UL) || (frequency > 493000000UL)
			|| (txpower < -17) || (txpower > 22)){
		return ESP_ERR_INVALID_ARG;
	}

	sx126x_status_t status = sx126x_clear_irq_status(0, SX126X_IRQ_ALL);

	if(status == SX126X_STATUS_OK){
		status = LORA_setupLoRaTX(frequency, 0, SX126X_LORA_SF8, SX126X_LORA_BW_125,
									SX126X_LORA_CR_4_5, 0x02, 0x02);
	}

	if(status == SX126X_STATUS_OK){
		status = sx126x_set_tx_params(0, txpower, SX126X_RAMP_10_US);
	}

	if(status == SX126X_STATUS_OK)
		return ESP_OK;

	return ESP_FAIL;
}

void LORA_modeFSK(){

}

static uint16_t SX126X_readIrqStatus(){
	uint16_t res = 0;
	sx126x_get_irq_status(0, (sx126x_irq_mask_t*) &res );

	return res;
}

esp_err_t LORA_waitTXDone(uint32_t timeout_ms) {
	uint32_t elapsed = 0;
	while(elapsed < timeout_ms) {
		if(SX126X_readIrqStatus() & SX126X_IRQ_TX_DONE)
			return ESP_OK;
		vTaskDelay(pdMS_TO_TICKS(5));
		elapsed += 5;
	}
	return ESP_ERR_TIMEOUT;
}

esp_err_t LORA_startRX(void) {
	sx126x_set_standby(0, SX126X_STANDBY_CFG_RC);
	sx126x_clear_irq_status(0, SX126X_IRQ_ALL);

	sx126x_pkt_params_lora_t sx126x_pkt_params_lora_d;
	sx126x_pkt_params_lora_d.crc_is_on            = true;
	sx126x_pkt_params_lora_d.header_type          = SX126X_LORA_PKT_EXPLICIT;
	sx126x_pkt_params_lora_d.invert_iq_is_on      = false;
	sx126x_pkt_params_lora_d.pld_len_in_bytes     = 255;
	sx126x_pkt_params_lora_d.preamble_len_in_symb = 8;
	sx126x_set_lora_pkt_params(0, &sx126x_pkt_params_lora_d);

	sx126x_set_rx_with_timeout_in_rtc_step(0, 0xFFFFFF);  // 0xFFFFFF = continuous RX mode
	return ESP_OK;
}

esp_err_t LORA_performCAD(void) {
	sx126x_irq_mask_t irq_status;
	sx126x_cad_params_t par;
	par.cad_detect_min  = 10;
	par.cad_detect_peak = 23;
	par.cad_exit_mode   = SX126X_CAD_LBT;
	par.cad_symb_nb     = SX126X_CAD_04_SYMB;
	par.cad_timeout     = 0;

	sx126x_set_cad_params(0, &par);
	sx126x_set_cad(0);
	vTaskDelay(pdMS_TO_TICKS(10));
	sx126x_get_irq_status(0, &irq_status);
	sx126x_clear_irq_status(0, SX126X_IRQ_CAD_DONE | SX126X_IRQ_CAD_DETECTED);

	if(irq_status & SX126X_IRQ_CAD_DETECTED)
		return ESP_FAIL;  // channel busy

	return ESP_OK;  // channel clear
}

esp_err_t LORA_receive(uint8_t *rxbuffer, uint8_t *size) {
	esp_err_t ret = LORA_receivePacketLoRa(rxbuffer, size);
	if(ret == ESP_OK)
		LORA_startRX();
	return ret;
}

esp_err_t LORA_sendWithLBT(uint8_t *txbuffer, uint8_t size) {
	bool channel_clear = false;
	for(uint8_t i = 0; i < 5; i++){
		if(LORA_performCAD() == ESP_OK){
			channel_clear = true;
			break;
		}
		vTaskDelay(pdMS_TO_TICKS(5));
	}

	if(channel_clear){
		LORA_sendPacketLoRa(txbuffer, size, LORA_TX_NO_WAIT);
		LORA_waitTXDone(500);
	} else {
		ESP_LOGW(TAG, "LBT: channel busy, TX skipped");
	}

	LORA_startRX();
	return channel_clear ? ESP_OK : ESP_FAIL;
}

esp_err_t LORA_receivePacketLoRa(uint8_t *rxbuffer, uint8_t *size) {
	uint16_t irq = SX126X_readIrqStatus();

	if(!(irq & SX126X_IRQ_RX_DONE))
		return ESP_ERR_NOT_FOUND;

	sx126x_clear_irq_status(0, SX126X_IRQ_ALL);

	if(irq & SX126X_IRQ_CRC_ERROR)
		return ESP_FAIL;

	sx126x_rx_buffer_status_t buf_status;
	sx126x_get_rx_buffer_status(0, &buf_status);

	*size = buf_status.pld_len_in_bytes;
	sx126x_read_buffer(0, buf_status.buffer_start_pointer, rxbuffer, *size);

	return ESP_OK;
}

esp_err_t LORA_sendPacketLoRa(uint8_t *txbuffer, uint16_t size, uint32_t txtimeout) {
	if ((size == 0) || (size > 256)) {
		return ESP_ERR_INVALID_SIZE;
	}

	if(txbuffer == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	sx126x_set_standby(0, SX126X_STANDBY_CFG_RC);

	sx126x_set_buffer_base_address(0, 0, 0);

	sx126x_write_buffer(0, 0, txbuffer,	size);

	sx126x_pkt_params_lora_t sx126x_pkt_params_lora_d;
	sx126x_pkt_params_lora_d.crc_is_on 				= true;
	sx126x_pkt_params_lora_d.header_type 			= SX126X_LORA_PKT_EXPLICIT;
	sx126x_pkt_params_lora_d.invert_iq_is_on 		= false;
	sx126x_pkt_params_lora_d.pld_len_in_bytes 		= size;
	sx126x_pkt_params_lora_d.preamble_len_in_symb 	= 8;

	sx126x_set_lora_pkt_params(0, &sx126x_pkt_params_lora_d);

	sx126x_set_tx(0, txtimeout);	//this starts the TX

	if(txtimeout){
		volatile uint16_t timeout = 10000;
		while ((!(SX126X_readIrqStatus() & SX126X_IRQ_TX_DONE)) && timeout){	//Wait for TX done
			vTaskDelay(1);
			timeout--;
		}

		if (SX126X_readIrqStatus() & SX126X_IRQ_TIMEOUT) {        //check for timeout
			return ESP_FAIL;
		} else {
			return ESP_OK;
		}
	}

	return ESP_OK;
}
