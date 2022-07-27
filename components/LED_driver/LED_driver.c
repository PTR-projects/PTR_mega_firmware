#include "LED_driver.h"
#include "esp_log.h"
#include "BOARD.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_err.h"
//LED/BUZZER DEFINITIONS
#define LED_COUNT 1
#define BUZZER_COUNT 1
#define BUZZER_GENERATOR 1
#define SRV_CLOCK 100
//STRIP LED DEFINITIONS

#define STRIP_LED_CHANNEL	0
#define BITS_PER_LED_CMD 8
#define STRIP_LED_COLOURS 3
#define STRIP_LED_COUNT 3
#define STRIP_LED_GPIO LED_WS_PIN
#define LED_BUFFER_ITEMS ((STRIP_LED_COUNT * BITS_PER_LED_CMD * STRIP_LED_COLOURS))
#define LED_ARRAY_SIZE (LED_POS + LED_COUNT + BUZZER_COUNT)
#define BUZZER_POS LED_POS + LED_COUNT
#define LED_POS STRIP_LED_COUNT * STRIP_LED_COLOURS
// HIGH/LOW times for StripLED 
#define T0H 3  // 0 bit high time
#define T1H 7  // 1 bit high time
#define T0L 7  // 0 bit low time
#define T1L 3  // 1 bit low time

// ESPLOG Tag definition
static const char *TAG = "LED";


rmt_item32_t led_data_buffer[LED_BUFFER_ITEMS * STRIP_LED_COLOURS]; //Strip LED set buffer TODO Tu chyba nie powinno byæ tego * STRIP_LED_COUNT
static LED_t led_array[LED_ARRAY_SIZE]; //LED BUZZER STATUS ARRAY

void ws2812_control_init(void) {
	rmt_config_t config;
	config.rmt_mode = RMT_MODE_TX;
	config.channel = STRIP_LED_CHANNEL;
	config.gpio_num = STRIP_LED_GPIO;
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

esp_err_t LED_init(){
	gpio_config_t io_conf = {};
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = ((1ULL<<LED_2_PIN));
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);

	return ESP_OK;
}

esp_err_t Buzzer_init(){
#if BUZZER_GENERATOR == 1
	gpio_config_t io_conf = {};

	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = ((1ULL<<BUZZER_PIN));
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;

	gpio_config(&io_conf);

#else
	// space for passive buzzer code
#endif

	return ESP_OK;
}




void ws2812_update() {
	ESP_ERROR_CHECK(
			rmt_write_items(STRIP_LED_CHANNEL, led_data_buffer,
					LED_BUFFER_ITEMS, false));
	ESP_ERROR_CHECK (rmt_wait_tx_done( STRIP_LED_CHANNEL, portMAX_DELAY));
}

void setup_rmt_data_buffer(void) {
	uint8_t blank = 0;
	for (uint32_t x = 0; x < STRIP_LED_COUNT * 3; x++) {
		uint8_t bits_to_send =
				led_array[x].state ? led_array[x].bright : blank;
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
	for (int8_t i = STRIP_LED_COLOURS - 1; i >= 0; i--){
	led_array[number * 3 + i].bright = rgb[2-i];
	}
	ESP_LOGI(TAG, "Strip LED %d colour set to: %X %X %X", number, rgb[2], rgb[1], rgb[0]);
}


void strip_led_mode(uint8_t number, led_mode_t mode) {
	for (int8_t i = STRIP_LED_COLOURS - 1; i >= 0; i--){
	led_array[number * 3 + i].mode = mode;
	}
	ESP_LOGI(TAG, "Strip LED %d mode set to: %d", number, mode);
}

void strip_led_blink_counter(uint8_t number, uint16_t counter) {
	for (int8_t i = STRIP_LED_COLOURS - 1; i >= 0; i--){
	led_array[number * 3 + i].counter = counter;
	}
	ESP_LOGI(TAG, "Strip LED %d blink counter set to: %d", number, counter);
}
void led_mode(uint8_t number, led_mode_t mode) {
	led_array[(LED_POS + number)].mode = mode;
	ESP_LOGI(TAG, "LED %d mode set to: %d", number, mode);
}

void strip_led_blink_rate(uint8_t number, uint16_t on_time_tics, uint16_t off_time_tics) {
	for (int8_t i = STRIP_LED_COLOURS - 1; i >= 0; i--){
		led_array[number * 3 + i].off_time_tics = off_time_tics;
		led_array[number * 3 + i].on_time_tics = on_time_tics;
	}
	ESP_LOGI(TAG, "Strip LED %d blink rate set to: %d/%d ON/OFF", number, on_time_tics, off_time_tics);
}

void led_blink_rate(uint8_t number, uint16_t on_time_tics, uint16_t off_time_tics) {
	led_array[(LED_POS + number)].off_time_tics = off_time_tics;
	led_array[(LED_POS + number)].on_time_tics = on_time_tics;
	ESP_LOGI(TAG, "LED %d blink rate set to: %d/%d ON/OFF", number, on_time_tics, off_time_tics);
}


void any_led_state(uint8_t i, uint8_t state){
	if (i < STRIP_LED_COUNT * STRIP_LED_COLOURS)
				{
					led_array[i].state = state;

				}

				if ( i < (LED_POS + 1))
				{
					gpio_set_level(LED_2_PIN, state);

				}

				else
	#if BUZZER_GENERATOR == 1
				{
					gpio_set_level(BUZZER_PIN, state);

				}
	#elif
				{
					//Passive buzzer code

				}
	#endif
}

esp_err_t LED_srv() {

	for (uint8_t i = 0; i < (LED_ARRAY_SIZE); i++) {

		switch (led_array[i].mode) {
		case LED_MODE_ON:
			any_led_state(i, 1);
			break;


		case LED_MODE_OFF:
			any_led_state(i, 0);
						break;

		case LED_MODE_BLINK:

			if (led_array[i].state == 0) {
				if (++led_array[i].counter > led_array[i].off_time_tics) {
					any_led_state(i, 1);
					led_array[i].counter = 0;
				}
			}
			if (led_array[i].state == 1) {
				if (++led_array[i].counter > led_array[i].on_time_tics) {
					any_led_state(i, 0);
					led_array[i].counter = 0;
				}
			}
			break;

		case LED_MODE_PULSE:
			if (led_array[i].state == 0) {
				any_led_state(i, 1);
				led_array[i].counter = 0;
			}

			if (led_array[i].state == 1) {
				if (++led_array[i].counter > led_array[i].on_time_tics) {
					any_led_state(i, 0);
					led_array[i].counter = 0;
				}
			}
			break;
		default:
			;
		}
	}
	setup_rmt_data_buffer();
	return ESP_OK;
}






esp_err_t LED_blink(uint8_t led_no, uint16_t t_on_ms, uint16_t t_off_ms, uint16_t blinks_number){
	if (blinks_number == 0) {
		led_mode(LED_POS + led_no, LED_MODE_BLINK);
		led_blink_rate(led_no, t_on_ms/100, t_off_ms/100);
	}
	else{
		led_mode(LED_POS + led_no, LED_MODE_PULSE);
		led_blink_rate(led_no, t_on_ms/100, t_off_ms/100);
		led_array[led_no].counter = blinks_number;
	}
	return ESP_OK;
}
esp_err_t LED_blinkWS(uint8_t led_no, led_colour_t colour, uint8_t brightness, uint16_t t_on_ms, uint16_t t_off_ms, uint16_t blinks_number){

	if (blinks_number == 0) {
		strip_led_colour(led_no, colour);
		strip_led_mode(led_no, LED_MODE_BLINK);
		strip_led_blink_rate(led_no, t_on_ms/100, t_off_ms/100);
	}
	else{
		strip_led_colour(led_no, colour);
		strip_led_mode(led_no, LED_MODE_PULSE);
		strip_led_blink_rate(led_no, t_on_ms/100, t_off_ms/100);
		strip_led_blink_counter(led_no, blinks_number);
	}
	return ESP_OK;
}

esp_err_t Buzzer_beep(uint16_t t_on_ms, uint16_t t_off_ms, uint16_t beeps_number){
	if (beeps_number == 0) {
		led_mode(BUZZER_POS, LED_MODE_BLINK);
		led_blink_rate(BUZZER_POS, t_on_ms/100, t_off_ms/100);
	}
	else{
		led_mode(BUZZER_POS, LED_MODE_PULSE);
		led_blink_rate(BUZZER_POS, t_on_ms/100, t_off_ms/100);
		led_array[BUZZER_POS].counter = beeps_number;
	}
	return ESP_OK;
}
