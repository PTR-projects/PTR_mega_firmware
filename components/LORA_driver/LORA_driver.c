#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"

#include "SX126x_hal.h"
#include "SX126x_driver.h"
#include "LORA_driver.h"

static const char *TAG = "LORA driver";

void LORA_modeLORA(uint32_t frequency, int8_t txpower);

void LORA_init()
{
	SX126X_initIO();
	vTaskDelay(pdMS_TO_TICKS( 20 ));

	ESP_LOGI(TAG, "SX1262 init...   ");
	LORA_modeLORA(433000000UL, 0);	//433MHz, 0dBm
	ESP_LOGI(TAG, "SX1262 ready\n");
}

void LORA_setupLoRaTX(uint32_t frequency, int32_t offset, uint8_t modParam1,
	uint8_t modParam2, uint8_t modParam3, uint8_t modParam4, uint8_t device) {
	sx126x_set_standby(0, SX126X_STANDBY_CFG_RC);
	sx126x_set_reg_mode(0, SX126X_REG_MODE_DCDC);

	sx126x_pa_cfg_params_t sx126x_pa_cfg_params_d;
	sx126x_pa_cfg_params_d.pa_duty_cycle = 0x04;
	sx126x_pa_cfg_params_d.hp_max 	  	 = 0x07;
	sx126x_pa_cfg_params_d.device_sel 	 = 0x00;
	sx126x_pa_cfg_params_d.pa_lut 	  	 = 0x01;
	sx126x_set_pa_cfg(0, &sx126x_pa_cfg_params_d);

	sx126x_set_dio3_as_tcxo_ctrl(0, SX126X_TCXO_CTRL_3_3V, 100);
	vTaskDelay(pdMS_TO_TICKS( 5 ));
	sx126x_cal(0, SX126X_CAL_ALL);	//is required after setting TCXO
	sx126x_cal_img(0, frequency);
	sx126x_set_dio2_as_rf_sw_ctrl(0, true);
	sx126x_set_pkt_type(0, SX126X_PKT_TYPE_LORA);
	sx126x_set_rf_freq(0, frequency);

	sx126x_mod_params_lora_t sx126x_mod_params_lora_d;
	sx126x_mod_params_lora_d.bw   = modParam2;
	sx126x_mod_params_lora_d.cr   = modParam3;
	sx126x_mod_params_lora_d.ldro = modParam4;
	sx126x_mod_params_lora_d.sf   = modParam1;
	sx126x_set_lora_mod_params(0, &sx126x_mod_params_lora_d);
	sx126x_set_buffer_base_address(0, 0, 0);

	sx126x_pkt_params_lora_t sx126x_pkt_params_lora_d;
	sx126x_pkt_params_lora_d.crc_is_on 				= true;
	sx126x_pkt_params_lora_d.header_type 			= SX126X_LORA_PKT_EXPLICIT;
	sx126x_pkt_params_lora_d.invert_iq_is_on 		= false;
	sx126x_pkt_params_lora_d.pld_len_in_bytes 		= 255;
	sx126x_pkt_params_lora_d.preamble_len_in_symb 	= 8;
	sx126x_set_lora_pkt_params(0, &sx126x_pkt_params_lora_d);

	sx126x_set_dio_irq_params(0, SX126X_IRQ_ALL, (SX126X_IRQ_TX_DONE + SX126X_IRQ_TIMEOUT+ SX126X_IRQ_RX_DONE),
								 SX126X_IRQ_NONE, SX126X_IRQ_NONE);
}

void LORA_modeLORA(uint32_t frequency, int8_t txpower){
	sx126x_clear_irq_status(0, SX126X_IRQ_ALL);
	LORA_setupLoRaTX(frequency, 0, SX126X_LORA_SF8, SX126X_LORA_BW_125, SX126X_LORA_CR_4_5,
				0x02, 0x02);
	sx126x_set_tx_params(0, txpower, SX126X_RAMP_10_US);
}

void LORA_modeFSK(){

}

/**
* @brief Reads the IRQ status of the SX126X.
* @return The IRQ status of the SX126X.
* This function reads and returns the IRQ status of the SX126X.
*/
uint16_t SX126X_readIrqStatus(){
	uint16_t res = 0;
	sx126x_get_irq_status(0, (sx126x_irq_mask_t*) &res );

	return res;
}

bool LORA_sendPacketLoRa(uint8_t *txbuffer, uint16_t size, uint32_t txtimeout) {
	if ((size == 0) || (size > 256)) {
		return false;
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
			return false;
		} else {
			return true;
		}
	}

	return true;
}
