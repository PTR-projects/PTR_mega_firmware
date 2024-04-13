#include <stdio.h>
#include "BOARD.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "IGN_driver.h"

static esp_err_t IGN_init_en_pin(int8_t pin);
static esp_err_t IGN_srv_handler(int8_t num);

#define CONFIG_IGN_DURATION_TICKS 1
static const char *TAG = "IGN_driver";

static uint32_t periodic_timer 			= 0;
static uint16_t IGN_counter[IGN_NUM] 	= {0};
static uint16_t IGN_PIN[IGN_NUM] 		= {IGN_EN_PINS_LIST};
static uint16_t ign_duration 			= 1;

esp_err_t IGN_init(void)
{
	for(uint8_t i=0; i<IGN_NUM; i++){
		IGN_init_en_pin(IGN_PIN[i]);
	}

	ign_duration = CONFIG_IGN_DURATION_TICKS;
	if(ign_duration == 0)
		ign_duration = 1;

	ESP_LOGI(TAG, "Init ready!");
	return ESP_OK;
}

esp_err_t IGN_srv(uint32_t time){
	if((periodic_timer+20) <= time){
		periodic_timer = time;

		for(uint8_t i=0; i<IGN_NUM; i++){
			IGN_srv_handler(i);
		}
	}

	return ESP_OK;
}

int8_t IGN_getState(uint8_t ign_no){
	if(ign_no < IGN_NUM){
		if(gpio_get_level(IGN_PIN[ign_no])) return 1;
		else return 0;
	}

	return -1;
}

esp_err_t IGN_set(uint8_t ign_no, uint8_t state){
	if(ign_no < IGN_NUM){
		if((IGN_counter[ign_no] == 0) && state){
			gpio_set_level(IGN_PIN[ign_no], 1);
			IGN_counter[ign_no] = ign_duration;
		} else if(state == 0){
			gpio_set_level(IGN_PIN[ign_no], 0);
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

static esp_err_t IGN_init_en_pin(int8_t pin){
	esp_err_t err = ESP_OK;
	err |= gpio_reset_pin(pin);
	err |= gpio_set_direction(pin, GPIO_MODE_INPUT_OUTPUT);
	err |= gpio_set_level(pin, 0);

	return err;
}

static esp_err_t IGN_srv_handler(int8_t num){
	if(num < IGN_NUM){
		if(IGN_getState(num)){
			if(IGN_counter[num] > 0)
				IGN_counter[num]--;
			else
				IGN_set(num, 0);
		}
		return ESP_OK;
	}

	return ESP_FAIL;
}
