#include "LED_driver.h"
#include "esp_log.h"
#include "BOARD.h"


//LED/BUZZER DEFINITIONS
#define LED_COUNT 1
#define BUZZER_COUNT 1


//STRIP LED DEFINITIONS

#define led_CHANNEL	0
#define BITS_PER_LED_CMD 8
#define STRIP_LED_COLOURS 3
#define STRIP_LED_COUNT 3
#define led_GPIO 33
#define LED_BUFFER_ITEMS ((STRIP_LED_COUNT * BITS_PER_LED_CMD * STRIP_LED_COLOURS))
#define LED_ARRAY_SIZE ((STRIP_LED_COUNT * STRIP_LED_COLOURS) + LED_COUNT + BUZZER_COUNT)

// HIGH/LOW times for StripLED 
#define T0H 3  // 0 bit high time
#define T1H 7  // 1 bit high time
#define T0L 7  // 0 bit low time
#define T1L 3  // 1 bit low time

// ESPLOG Tag definition
static const char *TAG = "LED";


rmt_item32_t led_data_buffer[LED_BUFFER_ITEMS * STRIP_LED_COUNT]; //Strip LED set buffer
static LED_t led[LED_ARRAY_SIZE]; //LED BUZZER STATUS ARRAY

void ws2812_control_init(void) {
	rmt_config_t config;
	config.rmt_mode = RMT_MODE_TX;
	config.channel = led_CHANNEL;
	config.gpio_num = led_GPIO;
	config.mem_block_num = 3;
	config.tx_config.loop_en = false;
	config.tx_config.carrier_en = false;
	config.tx_config.idle_output_en = true;
	config.tx_config.idle_level = 0;
	config.tx_config.carrier_freq_hz = 10 * 1000 * 1000;
	config.clk_div = 8;

	ESP_ERROR_CHECK(rmt_config(&config));
	ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
	ESP_LOGI(TAG, "Strip LED init");
}

void ws2812_update() {
	ESP_ERROR_CHECK(
			rmt_write_items(led_CHANNEL, led_data_buffer,
					LED_BUFFER_ITEMS, false));
	ESP_ERROR_CHECK (rmt_wait_tx_done( led_CHANNEL, portMAX_DELAY));
}

void setup_rmt_data_buffer(void) {
	uint8_t blank = 0;
	for (uint32_t x = 0; x < STRIP_LED_COUNT * 3; x++) {
		uint8_t bits_to_send =
				led[x].state ? led[x].bright : blank;
		uint8_t mask = 1 << (BITS_PER_LED_CMD - 1);
		for (uint8_t bit = 0; bit < BITS_PER_LED_CMD; bit++) {
			uint8_t bit_is_set = bits_to_send & mask;
			led_data_buffer[x * BITS_PER_LED_CMD + bit] =
					bit_is_set ?
							(rmt_item32_t ) { { { T1H, 1, T1L, 0 } } } :
							(rmt_item32_t ) { { { T0H, 1, T0L, 0 } } };
							mask >>= 1;
		}
	}
	ws2812_update();
}

void strip_led_colour(uint8_t number, led_colour_t colour) {
	uint8_t *rgb = (uint8_t*)&colour; //read colour in 8bit chunks

	led[number * 3].bright = rgb[2];
	led[number * 3 + 1].bright = rgb[1];
	led[number * 3 + 2].bright = rgb[0];
	ESP_LOGI(TAG, "Strip LED %d colour set to: %X %X %X", number, rgb[2], rgb[1], rgb[0]);
}


void strip_led_mode(uint8_t number, led_mode_t mode) {
	led[number * 3].mode = mode;
	led[number * 3 + 1].mode = mode;
	led[number * 3 + 2].mode = mode;
	ESP_LOGI(TAG, "Strip LED %d mode set to: %d", number, mode);
}

void led_mode(uint8_t number, led_mode_t mode) {
	led[((STRIP_LED_COUNT * STRIP_LED_COLOURS) + number)].mode = mode;
	ESP_LOGI(TAG, "LED %d mode set to: %d", number, mode);
}

void strip_led_blink_rate(uint8_t number, uint8_t on_time_tics,
		uint8_t off_time_tics) {
	led[number * 3].off_time_tics = off_time_tics;
	led[number * 3 + 1].off_time_tics = off_time_tics;
	led[number * 3 + 2].off_time_tics = off_time_tics;
	led[number * 3].on_time_tics = on_time_tics;
	led[number * 3 + 1].on_time_tics = on_time_tics;
	led[number * 3 + 2].on_time_tics = on_time_tics;
	ESP_LOGI(TAG, "Strip LED %d blink rate set to: %d/%d ON/OFF", number, on_time_tics, off_time_tics);
}

void led_blink_rate(uint8_t number, uint8_t on_time_tics,
		uint8_t off_time_tics) {
	led[((STRIP_LED_COUNT * STRIP_LED_COLOURS) + number)].off_time_tics = off_time_tics;
	led[((STRIP_LED_COUNT * STRIP_LED_COLOURS) + number)].on_time_tics = on_time_tics;
	ESP_LOGI(TAG, "LED %d blink rate set to: %d/%d ON/OFF", number, on_time_tics, off_time_tics);
}

void LED_srv() {

	for (uint8_t i = 0; i < (LED_ARRAY_SIZE); i++) {

		switch (led[i].mode) {
		case LED_MODE_ON:
			led[i].state = 1;
			break;

		case LED_MODE_OFF:
			led[i].state = 0;
			break;

		case LED_MODE_BLINK:

			if (led[i].state == 0) {
				if (++led[i].counter > led[i].off_time_tics) {
					led[i].state = 1;
					led[i].counter = 0;
				}
			}
			if (led[i].state == 1) {
				if (++led[i].counter > led[i].on_time_tics) {
					led[i].state = 0;
					led[i].counter = 0;
				}
			}
			break;

		case LED_MODE_PULSE:
			if (led[i].state == 0) {
				led[i].state = 1;
				led[i].counter = 0;
			}

			if (led[i].state == 1) {
				if (++led[i].counter > led[i].on_time_tics) {
					led[i].state = 0;
					led[i].counter = 0;
				}
			}
			break;
		default:
			;
		}
	}
	setup_rmt_data_buffer();
}
