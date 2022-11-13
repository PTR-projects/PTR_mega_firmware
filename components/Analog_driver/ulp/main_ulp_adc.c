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
#include "ulp_riscv/ulp_riscv_adc_ulp_core.h"

/* this variables will be exported as a public symbol, visible from main CPU: */
uint16_t ULP_IGN1_DET = 0;
uint16_t ULP_IGN2_DET = 0;
uint16_t ULP_IGN3_DET = 0;
uint16_t ULP_IGN4_DET = 0;
uint16_t ULP_VBAT 	  = 0;

int main (void)
{

    /* ulp_riscv_shutdown() is called automatically when main exits */
    return 0;
}
