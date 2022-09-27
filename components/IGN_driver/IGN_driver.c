#include <stdio.h>
#include "BOARD.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "IGN_driver.h"

static uint32_t periodic_timer 	= 0;
static uint16_t IGN1_counter 	= 0;
static uint16_t IGN2_counter 	= 0;
static uint16_t IGN3_counter 	= 0;
static uint16_t IGN4_counter 	= 0;

esp_err_t IGN_init(void)
{
	gpio_reset_pin(IGN1_EN_PIN);
	gpio_reset_pin(IGN2_EN_PIN);
	gpio_reset_pin(IGN3_EN_PIN);
	gpio_reset_pin(IGN4_EN_PIN);

	gpio_set_direction(IGN1_EN_PIN, GPIO_MODE_INPUT_OUTPUT);
	gpio_set_direction(IGN2_EN_PIN, GPIO_MODE_INPUT_OUTPUT);
	gpio_set_direction(IGN3_EN_PIN, GPIO_MODE_INPUT_OUTPUT);
	gpio_set_direction(IGN4_EN_PIN, GPIO_MODE_INPUT_OUTPUT);

	return ESP_OK;
}

esp_err_t IGN_srv(uint32_t time){
	if((periodic_timer+10) <= time){
		periodic_timer = time;

		if(IGN_getState(1)){
			if(IGN1_counter < 3)
				IGN1_counter++;
			else{
				IGN_set(1, 0);
				IGN1_counter = 3;
			}
		}

		if(IGN_getState(2)){
			if(IGN2_counter < 3)
				IGN2_counter++;
			else{
				IGN_set(2, 0);
				IGN2_counter = 3;
			}
		}

		if(IGN_getState(3)){
			if(IGN3_counter < 3)
				IGN3_counter++;
			else{
				IGN_set(3, 0);
				IGN3_counter = 3;
			}
		}

		if(IGN_getState(4)){
			if(IGN4_counter < 3)
				IGN4_counter++;
			else{
				IGN_set(4, 0);
				IGN4_counter = 3;
			}
		}
	}

	return ESP_OK;
}

uint8_t IGN_getState(uint8_t ign_no){
	if(ign_no == 1){
		if(gpio_get_level(IGN1_EN_PIN)) return 1;
		else return 0;
	}

	if(ign_no == 2){
		if(gpio_get_level(IGN2_EN_PIN)) return 1;
		else return 0;
	}

	if(ign_no == 2){
		if(gpio_get_level(IGN3_EN_PIN)) return 1;
		else return 0;
	}

	if(ign_no == 2){
		if(gpio_get_level(IGN4_EN_PIN)) return 1;
		else return 0;
	}

	return -1;
}

esp_err_t IGN_set(uint8_t ign_no, uint8_t state){
	if(ign_no == 1){
		if((IGN1_counter == 0) && state){
			gpio_set_level(IGN1_EN_PIN, 1);
		} else if(state == 0){
			gpio_set_level(IGN1_EN_PIN, 0);
			IGN2_counter = 0;
		}
	}

	if(ign_no == 2){
		if((IGN2_counter == 0) && state){
			gpio_set_level(IGN2_EN_PIN, 1);
		} else if(state == 0){
			gpio_set_level(IGN2_EN_PIN, 0);
			IGN2_counter = 0;
		}
	}

	if(ign_no == 3){
		if((IGN3_counter == 0) && state){
			gpio_set_level(IGN3_EN_PIN, 1);
		} else if(state == 0){
			gpio_set_level(IGN3_EN_PIN, 0);
			IGN3_counter = 0;
		}
	}

	if(ign_no == 4){
		if((IGN4_counter == 0) && state){
			gpio_set_level(IGN4_EN_PIN, 1);
		} else if(state == 0){
			gpio_set_level(IGN4_EN_PIN, 0);
			IGN4_counter = 0;
		}
	}

	return ESP_OK;
}

uint8_t IGN_check(uint8_t ign_no){
	if(1)		//<<< --------------------- odebrac z komponentu z ADC
		return 1;
	else
		return 0;
}
