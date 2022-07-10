#include "LED_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "driver/gpio.h"


#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM      LED_WS_PIN

static uint8_t led_strip_pixels[EXAMPLE_LED_NUMBERS * 3];

static const char *TAG = "LED_driver";

void LED_driver_init(void)
{
	//Normal LED init
	ESP_LOGI(TAG, "Create RMT TX channel");
	gpio_config_t io_conf = {};
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = (1ULL<<LED_2_PIN);
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);

	//WS LED init

	ESP_LOGI(TAG, "Create RMT TX channel");
	    rmt_channel_handle_t led_chan = NULL;
	    rmt_tx_channel_config_t tx_chan_config = {
	        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
	        .gpio_num = RMT_LED_STRIP_GPIO_NUM,
	        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
	        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
	        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
	    };
	    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));

	    ESP_LOGI(TAG, "Install led strip encoder");
	    rmt_encoder_handle_t led_encoder = NULL;
	    led_strip_encoder_config_t encoder_config = {
	        .resolution = RMT_LED_STRIP_RESOLUTION_HZ,
	    };
	    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder));

	    ESP_LOGI(TAG, "Enable RMT TX channel");
	    ESP_ERROR_CHECK(rmt_enable(led_chan));

}


void
