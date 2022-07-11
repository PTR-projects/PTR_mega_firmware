#include "LED_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "driver/gpio.h"
#include "BOARD.h"


static const char *TAG = "LED_driver";

void LED_driver_init(void)
{
	//Normal LED init
	ESP_LOGI(TAG, "Create RMT TX channel");
	gpio_config_t io_conf = {};
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = ((1ULL<<LED_2_PIN) | (1ULL<<BUZZER_PIN));
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);

	//WS LED init



}


