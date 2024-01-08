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

/* this variables will be exported as a public symbol, visible from main CPU: */
volatile uint32_t IGN1_RAW	= 0;
volatile uint32_t IGN2_RAW	= 0;
volatile uint32_t IGN3_RAW	= 0;
volatile uint32_t IGN4_RAW	= 0;
volatile uint32_t VBAT_RAW	= 24;
volatile uint32_t READY		= 0;
volatile uint32_t ERROR		= 0;

#define VBAT_OVERSAMPLE 128

int main (void)
{
	for(uint8_t i=0; i<8; i++){
		ERROR = 1;
		ulp_riscv_adc_read_channel(ADC_NUM_1, ADC_CHANNEL_7);
		ERROR = 0;
	}

	VBAT_RAW = 0;
	for(uint16_t i=0; i<VBAT_OVERSAMPLE; i++){
		ERROR = 1;
		VBAT_RAW += ulp_riscv_adc_read_channel(ADC_NUM_1, ADC_CHANNEL_7);
		ERROR = 0;
	}

	VBAT_RAW = VBAT_RAW / VBAT_OVERSAMPLE;


//----------------- IGN 1 --------------------------
	ERROR = 1;
	IGN1_RAW = ulp_riscv_adc_read_channel(ADC_NUM_1, ADC_CHANNEL_6);
	ERROR = 0;

//----------------- IGN 2 --------------------------
	ERROR = 1;
	IGN2_RAW = ulp_riscv_adc_read_channel(ADC_NUM_1, ADC_CHANNEL_5);
	ERROR = 0;

//----------------- IGN 3 --------------------------
	ERROR = 1;
	IGN3_RAW = ulp_riscv_adc_read_channel(ADC_NUM_1, ADC_CHANNEL_4);
	ERROR = 0;

//----------------- IGN 4 --------------------------
	ERROR = 1;
	IGN4_RAW = ulp_riscv_adc_read_channel(ADC_NUM_1, ADC_CHANNEL_3);
	ERROR = 0;


//----------------- ULP ADC ready ------------------
	READY = 1;

	CLEAR_PERI_REG_MASK(RTC_CNTL_ULP_CP_TIMER_REG, RTC_CNTL_ULP_CP_SLP_TIMER_EN);
    /* ulp_riscv_shutdown() is called automatically when main exits */
    return 0;
}
