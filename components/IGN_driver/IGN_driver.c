#include <stdio.h>
#include "BOARD_cfg.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "IGN_driver.h"

static esp_err_t IGN_init_en_pin(int8_t pin);

static const char *TAG = "IGN_driver";

static uint16_t IGN_counter[IGN_NUM] 	= {0};
static uint16_t IGN_PIN[IGN_NUM] 		= {IGN_EN_PINS_LIST};


esp_err_t IGN_init(void)
{
	for(uint8_t i=0; i<IGN_NUM; i++){
		IGN_init_en_pin(IGN_PIN[i]);
	}

	ESP_LOGI(TAG, "Init ready!");
	return ESP_OK;
}

IGN_t IGN_getState(){
	IGN_t ign = {0};

	for(int i=0; i<IGN_NUM; i++){
		ign.igniter_state[i] = gpio_get_level(IGN_PIN[i]);
	}

	return ign;
}

esp_err_t IGN_set(uint8_t ign_no, uint8_t state){
	if(ign_no < IGN_NUM){
		if((IGN_counter[ign_no] == 0) && state){
			gpio_set_level(IGN_PIN[ign_no], 1);
			ESP_LOGI(TAG, "IGN %d set %d", ign_no, state);
		} else if(state == 0){
			gpio_set_level(IGN_PIN[ign_no], 0);
			ESP_LOGI(TAG, "IGN %d set 0", ign_no);
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