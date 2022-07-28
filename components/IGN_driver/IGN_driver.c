#include <stdio.h>
#include "BOARD.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "IGN_driver.h"

esp_err_t IGN_init(void)
{
	gpio_reset_pin(IGN1_EN_PIN);
	gpio_reset_pin(IGN2_EN_PIN);
	gpio_reset_pin(IGN3_EN_PIN);
	gpio_reset_pin(IGN4_EN_PIN);

	gpio_set_direction(IGN1_EN_PIN,  GPIO_MODE_OUTPUT);
	gpio_set_direction(IGN2_EN_PIN, GPIO_MODE_OUTPUT);
	gpio_set_direction(IGN3_EN_PIN, GPIO_MODE_OUTPUT);
	gpio_set_direction(IGN4_EN_PIN, GPIO_MODE_OUTPUT);

	return ESP_OK;
}
