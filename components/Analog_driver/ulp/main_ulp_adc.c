/* ULP-RISC-V example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.

   This code runs on ULP-RISC-V  coprocessor
*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "ulp_riscv/ulp_riscv.h"
#include "ulp_riscv/ulp_riscv_utils.h"
#include "soc/rtc.h"
#include "soc/rtc_cntl_reg.h"
#include "ulp_riscv/ulp_riscv_adc_ulp_core.h"
#include "hal/adc_types.h"
#include "hal/adc_ll.h"
#include "BOARD.h"

/* this variables will be exported as a public symbol, visible from main CPU: */
volatile uint32_t VBAT_RAW 	= 0;
volatile uint32_t IGN_RAW[IGN_NUM] 	= {0};
volatile uint32_t READY		= 0;
volatile uint32_t ERROR		= 0;

#define VBAT_OVERSAMPLE 128

int main (void)
{
	uint32_t ADC_RAW[ADC_CHANNELS_NUM]	= {0};
	uint32_t ADC_CHANNELS[ADC_CHANNELS_NUM] = {ADC_CHANNELS_LIST};

	for(uint8_t i=0; i<8; i++){
		ERROR = 1;
		ulp_riscv_adc_read_channel(ADC_NUM_1, ADC_CHANNELS[0]);
		ERROR = 0;
	}


	//----------------- ADC MEAS--------------------------
	ERROR = 1;
	for(uint8_t i=0; i<ADC_CHANNELS_NUM; i++){
		ADC_RAW[i] = 0;
		for(uint16_t j=0; j<VBAT_OVERSAMPLE; j++){
			ADC_RAW[i] += ulp_riscv_adc_read_channel(ADC_NUM_1, ADC_CHANNELS[i]);
		}
		ADC_RAW[i] = ADC_RAW[i] / VBAT_OVERSAMPLE;
	}
	ERROR = 0;

	//------ Rewrite Measurements to dedicated external variables ----
	VBAT_RAW = ADC_RAW[0];

	for(uint8_t i=0; i<IGN_NUM; i++){
		IGN_RAW[i] = ADC_RAW[i+1];
	}

	//----------------- ULP ADC ready ------------------
	READY = 1;

	CLEAR_PERI_REG_MASK(RTC_CNTL_ULP_CP_TIMER_REG, RTC_CNTL_ULP_CP_SLP_TIMER_EN);
    /* ulp_riscv_shutdown() is called automatically when main exits */
    return 0;
}
