#include "LED_driver.h"
#include "esp_log.h"
#include "BOARD.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_err.h"

static esp_err_t ws2812_control_init(void);
static esp_err_t ws2812_control_deinit(void);
static esp_err_t led_blink_rate(uint8_t number, uint16_t on_time_tics, uint16_t off_time_tics); //Set blink rate for LED
static esp_err_t led_mode(uint8_t number, led_mode_t mode); //Set blink mode for LED
static esp_err_t strip_led_colour(uint8_t number, led_colour_t colour, uint8_t brightness); //Set colour of RGB LED Brightness 0-255
static esp_err_t setup_rmt_data_buffer(void); //Prepare strip leds set array for update
static esp_err_t ws2812_update(void); //update strip leds
static esp_err_t strip_led_mode(uint8_t number, led_mode_t mode);
static esp_err_t strip_led_blink_pulses(uint8_t number, uint16_t pulses);
static esp_err_t strip_led_blink_rate(uint8_t number, uint16_t on_time_tics, uint16_t off_time_tics);
static esp_err_t any_led_state(uint8_t i, uint8_t state);

// ESPLOG Tag definition
static const char *TAG = "LED";

static uint32_t loop_interval_ms = 100;


rmt_item32_t led_data_buffer[LED_BUFFER_ITEMS * STRIP_LED_COLOURS]; //Strip LED set buffer TODO Tu chyba nie powinno by� tego * LED_WS_COUNT
static LED_t led_array[LED_ARRAY_SIZE]; //LED BUZZER STATUS ARRAY

esp_err_t LED_init(uint32_t interval_ms){
	ws2812_control_deinit();

	gpio_config_t io_conf = {};
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = ((1ULL<<LED_2_PIN));
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);
	ESP_LOGI(TAG, "LED init");

	ws2812_control_init();

	loop_interval_ms = interval_ms;

	return ESP_OK;
}

esp_err_t BUZZER_init(){
#if BUZZER_GENERATOR == 1
	gpio_config_t io_conf = {};

	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = ((1ULL<<BUZZER_PIN));
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;

	gpio_config(&io_conf);
	ESP_LOGI(TAG, "BUZZER init");
#else
	// space for passive buzzer code
#endif

	return ESP_OK;
}

esp_err_t LED_srv() {
	ESP_LOGV(TAG, "LED UPDATE");
	for (uint8_t i = 0; i < (LED_ARRAY_SIZE); i++) {

		switch (led_array[i].mode) {
		case LED_MODE_ON:
			any_led_state(i, 1);
			break;

		case LED_MODE_OFF:
			any_led_state(i, 0);
						break;

		case LED_MODE_BLINK:
			ESP_LOGV(TAG, "LED blink");
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
			ESP_LOGV(TAG, "LED pulse");
			if (led_array[i].state == 0 && led_array[i].pulses > 0) {
				if (++led_array[i].counter > led_array[i].off_time_tics) {
					any_led_state(i, 1);
					led_array[i].counter = 0;
					led_array[i].pulses--;
				}
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

esp_err_t BUZZER_beep(uint16_t t_on_ms, uint16_t t_off_ms, uint16_t beeps_number){
	if (beeps_number == 0) {
		led_mode(BUZZER_POS, LED_MODE_BLINK);
		led_blink_rate(BUZZER_POS, t_on_ms/loop_interval_ms, t_off_ms/loop_interval_ms);
	}
	else{
		led_mode(BUZZER_POS, LED_MODE_PULSE);
		led_blink_rate(BUZZER_POS, t_on_ms/loop_interval_ms, t_off_ms/loop_interval_ms);
		led_array[BUZZER_POS].counter = beeps_number;
	}
	return ESP_OK;
}

esp_err_t LED_blink(uint8_t led_no, uint16_t t_on_ms, uint16_t t_off_ms, uint16_t blinks_number){
	if (blinks_number == 0) {
		led_mode(LED_POS + led_no, LED_MODE_BLINK);
		led_blink_rate(LED_POS + led_no, t_on_ms/loop_interval_ms, t_off_ms/loop_interval_ms);
	}
	else{
		led_mode(LED_POS + led_no, LED_MODE_PULSE);
		led_blink_rate(LED_POS + led_no, t_on_ms/loop_interval_ms, t_off_ms/loop_interval_ms);
		led_array[LED_POS + led_no].pulses = blinks_number;
	}
	return ESP_OK;
}

esp_err_t LED_blinkWS(uint8_t led_no, led_colour_t colour, uint8_t brightness_percent, uint16_t t_on_ms, uint16_t t_off_ms, uint16_t blinks_number){

	if (blinks_number == 0) {
		strip_led_colour(led_no, colour, brightness_percent);
		strip_led_mode(led_no, LED_MODE_BLINK);
		strip_led_blink_rate(led_no, t_on_ms/loop_interval_ms, t_off_ms/loop_interval_ms);

		if(t_off_ms == 0){
			strip_led_colour(led_no, colour, brightness_percent);
			strip_led_mode(led_no, LED_MODE_ON);
			strip_led_blink_rate(led_no, t_on_ms/loop_interval_ms, t_off_ms/loop_interval_ms);
		}
	}
	else{
		strip_led_colour(led_no, colour, brightness_percent);
		strip_led_mode(led_no, LED_MODE_PULSE);
		strip_led_blink_rate(led_no, t_on_ms/loop_interval_ms, t_off_ms/loop_interval_ms);
		strip_led_blink_pulses(led_no, blinks_number);
	}
	return ESP_OK;
}


//------------------------------------------------
//   Low Level
//------------------------------------------------

static esp_err_t ws2812_control_init(void) {
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
	ESP_LOGI(TAG, "Strip LED init start");
	ESP_ERROR_CHECK(rmt_config(&config));
	ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
	ESP_LOGI(TAG, "Strip LED init");
	return ESP_OK;
}

static esp_err_t ws2812_control_deinit(void) {
	rmt_driver_uninstall(STRIP_LED_CHANNEL); // uninstall RMT driver, ignore errors

	ESP_LOGI(TAG, "Strip LED deinit");
	return ESP_OK;
}

static esp_err_t  ws2812_update(void) {
	ESP_ERROR_CHECK(
			rmt_write_items(STRIP_LED_CHANNEL, led_data_buffer,
					LED_BUFFER_ITEMS, false));
	ESP_ERROR_CHECK (rmt_wait_tx_done( STRIP_LED_CHANNEL, portMAX_DELAY));
	return ESP_OK;
}

static esp_err_t setup_rmt_data_buffer(void) {
	uint8_t blank = 0;
	for (uint32_t x = 0; x < LED_WS_COUNT * 3; x++) {
		uint8_t bits_to_send =
				led_array[x].state ? led_array[x].bright : blank;
		uint8_t mask = 1 << (BITS_PER_LED_CMD - 1);
		for (uint8_t bit = 0; bit < BITS_PER_LED_CMD; bit++) {
			uint8_t bit_is_set = bits_to_send & mask;
			led_data_buffer[x * BITS_PER_LED_CMD + bit] =
					bit_is_set ?
							(rmt_item32_t ) { { { WS_T1H, 1, WS_T1L, 0 } } } :
							(rmt_item32_t ) { { { WS_T0H, 1, WS_T0L, 0 } } };
							mask >>= 1;
		}
	}
	ws2812_update();
	return ESP_OK;
}

static esp_err_t strip_led_colour(uint8_t number, led_colour_t colour, uint8_t brightness_percent) {
	uint8_t *rgb = (uint8_t*)&colour; //read colour in 8bit chunks
	for (int8_t i = STRIP_LED_COLOURS - 1; i >= 0; i--){
		led_array[number * 3 + i].bright = rgb[2-i]* brightness_percent / 100;
	}
	ESP_LOGV(TAG, "Strip LED %d colour set to: %X %X %X", number, rgb[2], rgb[1], rgb[0]);
	return ESP_OK;
}


static esp_err_t strip_led_mode(uint8_t number, led_mode_t mode) {
	for (int8_t i = STRIP_LED_COLOURS - 1; i >= 0; i--){
		led_array[number * 3 + i].mode = mode;
	}
	ESP_LOGV(TAG, "Strip LED %d mode set to: %d", number, mode);
	return ESP_OK;
}

static esp_err_t strip_led_blink_pulses(uint8_t number, uint16_t pulses) {
	for (int8_t i = STRIP_LED_COLOURS - 1; i >= 0; i--){
		led_array[number * 3 + i].pulses = pulses;
	}
	ESP_LOGV(TAG, "Strip LED %d blink counter set to: %d", number, pulses);
	return ESP_OK;
}
static esp_err_t led_mode(uint8_t number, led_mode_t mode) {
	led_array[number].mode = mode;
	ESP_LOGV(TAG, "LED %d mode set to: %d", number, mode);
	return ESP_OK;
}

static esp_err_t strip_led_blink_rate(uint8_t number, uint16_t on_time_tics, uint16_t off_time_tics) {
	for (int8_t i = STRIP_LED_COLOURS - 1; i >= 0; i--){
		led_array[number * 3 + i].off_time_tics = off_time_tics;
		led_array[number * 3 + i].on_time_tics = on_time_tics;
	}
	ESP_LOGV(TAG, "Strip LED %d blink rate set to: %d/%d ON/OFF", number, on_time_tics, off_time_tics);
	return ESP_OK;
}

static esp_err_t led_blink_rate(uint8_t number, uint16_t on_time_tics, uint16_t off_time_tics) {
	led_array[number].off_time_tics = off_time_tics;
	led_array[number].on_time_tics = on_time_tics;
	ESP_LOGV(TAG, "LED %d blink rate set to: %d/%d ON/OFF", number, on_time_tics, off_time_tics);
	return ESP_OK;
}


static esp_err_t any_led_state(uint8_t i, uint8_t state){
	led_array[i].state = state;

	if (i >= (LED_POS) && i < BUZZER_POS )
	{
		gpio_set_level(LED_2_PIN, state);
		ESP_LOGV(TAG, "LED %d set %d", LED_2_PIN, state);
	}

	if( i >= BUZZER_POS)
	{
	#if BUZZER_GENERATOR == 1
		gpio_set_level(BUZZER_PIN, state);
	#else
			//Passive buzzer code
	#endif
	}

	return ESP_OK;
}




esp_err_t LED_set(uint8_t led_no, uint8_t state){
	if (state == 0) {
		led_mode(LED_POS + led_no, LED_MODE_OFF);
	}
	else{
		led_mode(LED_POS + led_no, LED_MODE_ON);
	}
	return ESP_OK;
}


esp_err_t BUZZER_set(uint8_t state){
	if (state == 0) {
		led_mode(BUZZER_POS, LED_MODE_OFF);
	}
	else{
		led_mode(BUZZER_POS, LED_MODE_ON);
	}
	return ESP_OK;
}

esp_err_t LED_setWS(uint8_t led_no, led_colour_t colour, uint8_t brightness_percent, uint8_t state){

	if (state == 0) {
		strip_led_colour(led_no, colour, brightness_percent);
		strip_led_mode(led_no, LED_MODE_OFF);
	}
	else{
		strip_led_colour(led_no, colour, brightness_percent);
		strip_led_mode(led_no, LED_MODE_ON);
	}

	return ESP_OK;
}

