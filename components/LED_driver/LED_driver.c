#include "LED_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "BOARD_cfg.h"
#include "driver/gpio.h"
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
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
static SemaphoreHandle_t mutex_LED;
static StaticSemaphore_t xMutexBuffer_LED;
static uint8_t LED_brightness_percentage = 100;
static int32_t LED_POS_IGN[LED_IGN_NUM] = {LED_POS_IGN_LIST};

#if (LED_WS_COUNT > 0) && (LED_WS_PIN != -1)
static rmt_channel_handle_t ws2812_tx_channel;
static rmt_encoder_handle_t ws2812_bytes_encoder;
static uint8_t led_data_buffer[LED_WS_RGB_COUNT];
#endif
static LED_t led_array[LED_ARRAY_SIZE]; 			//LED BUZZER STATUS ARRAY

esp_err_t LED_init(uint32_t interval_ms){
#if (LED_STD_COUNT > 0) && (LED_2_PIN != -1)
		gpio_config_t io_conf = {};
		io_conf.intr_type = GPIO_INTR_DISABLE;
		io_conf.mode = GPIO_MODE_OUTPUT;
		io_conf.pin_bit_mask = ((1ULL<<LED_2_PIN));
		io_conf.pull_down_en = 0;
		io_conf.pull_up_en = 0;
		gpio_config(&io_conf);
#endif

#if (LED_WS_COUNT > 0) && (LED_WS_PIN != -1)
		esp_err_t err = ws2812_control_deinit();
		if(err != ESP_OK){
			return err;
		}
		err = ws2812_control_init();
		if(err != ESP_OK){
			return err;
		}
#endif

	loop_interval_ms = interval_ms;

	// Init mutex
	mutex_LED = xSemaphoreCreateMutexStatic(&(xMutexBuffer_LED));

	ESP_LOGI(TAG, "LED init done");
	return ESP_OK;
}

esp_err_t BUZZER_init(){
#if (BUZZER_GENERATOR == 1) && (BUZZER_COUNT > 0) && (BUZZER_PIN != -1)
	gpio_config_t io_conf = {};

	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = ((1ULL<<BUZZER_PIN));
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;

	gpio_config(&io_conf);
	ESP_LOGI(TAG, "BUZZER init");
#elif (BUZZER_COUNT > 0) && (BUZZER_PIN != -1)
	// space for passive buzzer code

#else
#error "Buzzer configuration error"
#endif

	return ESP_OK;
}

esp_err_t LED_srv() {
	ESP_LOGV(TAG, "LED UPDATE");
	if(mutex_LED == NULL) return ESP_ERR_INVALID_STATE;	// LED_init() not run yet
	if(xSemaphoreTake(mutex_LED, pdMS_TO_TICKS(1000)) == pdTRUE){
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
		xSemaphoreGive(mutex_LED);
	}

	setup_rmt_data_buffer();

	return ESP_OK;
}

esp_err_t BUZZER_beep(uint16_t t_on_ms, uint16_t t_off_ms, uint16_t beeps_number){
#if (BUZZER_COUNT > 0)
	if(mutex_LED == NULL) return ESP_ERR_INVALID_STATE;	// LED_init() not run yet
	if(xSemaphoreTake(mutex_LED, pdMS_TO_TICKS(1000)) == pdTRUE){
		if (beeps_number == 0) {
			led_mode		(BUZZER_ARRAY_POS, LED_MODE_BLINK);
			led_blink_rate	(BUZZER_ARRAY_POS, t_on_ms/loop_interval_ms, t_off_ms/loop_interval_ms);
		}
		else{
			led_mode		(BUZZER_ARRAY_POS, LED_MODE_PULSE);
			led_blink_rate	(BUZZER_ARRAY_POS, t_on_ms/loop_interval_ms, t_off_ms/loop_interval_ms);
			led_array[BUZZER_ARRAY_POS].pulses = beeps_number;
		}
		xSemaphoreGive(mutex_LED);
	}
#endif

	return ESP_OK;
}

esp_err_t LED_blinkSTD(uint8_t led_no, uint16_t t_on_ms, uint16_t t_off_ms, uint16_t blinks_number){
#if (LED_STD_COUNT > 0)
	if(mutex_LED == NULL) return ESP_ERR_INVALID_STATE;	// LED_init() not run yet
	if(xSemaphoreTake(mutex_LED, pdMS_TO_TICKS(1000)) == pdTRUE){
		if (blinks_number == 0) {
			led_mode(LED_WS_RGB_COUNT + led_no, LED_MODE_BLINK);
			led_blink_rate(LED_WS_RGB_COUNT + led_no, t_on_ms/loop_interval_ms, t_off_ms/loop_interval_ms);
		}
		else{
			led_mode(LED_WS_RGB_COUNT + led_no, LED_MODE_PULSE);
			led_blink_rate(LED_WS_RGB_COUNT + led_no, t_on_ms/loop_interval_ms, t_off_ms/loop_interval_ms);
			led_array[LED_WS_RGB_COUNT + led_no].pulses = blinks_number;
		}
		xSemaphoreGive(mutex_LED);
	}
#endif

	return ESP_OK;
}

esp_err_t LED_blinkWS(int16_t led_no, led_colour_t colour, uint8_t brightness_percent, uint16_t t_on_ms, uint16_t t_off_ms, uint16_t blinks_number){
#if (LED_WS_COUNT > 0)
	if(led_no == -1)
		return ESP_FAIL;

	if(mutex_LED == NULL) return ESP_ERR_INVALID_STATE;	// LED_init() not run yet
	if(xSemaphoreTake(mutex_LED, pdMS_TO_TICKS(100)) == pdTRUE){
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
		xSemaphoreGive(mutex_LED);
	}
#endif

	return ESP_OK;
}

esp_err_t LED_setARM(led_colour_t colour, uint8_t brightness_percent, uint16_t t_on_ms, uint16_t t_off_ms, uint16_t blinks_number){
#if !defined(LED_POS_ARM)
	return ESP_FAIL;
#elif LED_CHECK_IF_WS(LED_POS_ARM)
	return LED_blinkWS(LED_POS_ARM, colour, brightness_percent, t_on_ms, t_off_ms, blinks_number);
#elif LED_CHECK_IF_STD(LED_POS_ARM)
	return LED_blinkSTD(LED_POS_ARM, t_on_ms, t_off_ms, blinks_number);
#elif (LED_POS_ARM == -1)
	return ESP_FAIL;
#else
#error "LED ARM not found in configuration"
#endif
}

esp_err_t LED_setSTAT(led_colour_t colour, uint8_t brightness_percent, uint16_t t_on_ms, uint16_t t_off_ms, uint16_t blinks_number){
#if !defined(LED_POS_STAT)
	return ESP_FAIL;
#elif LED_CHECK_IF_WS(LED_POS_STAT)
	return LED_blinkWS(LED_POS_STAT, colour, brightness_percent, t_on_ms, t_off_ms, blinks_number);
#elif LED_CHECK_IF_STD(LED_POS_STAT)
	return LED_blinkSTD(LED_POS_STAT, t_on_ms, t_off_ms, blinks_number);
#elif (LED_POS_STAT == -1)
	return ESP_FAIL;
#else
#error "LED STAT not found in configuration"
#endif
}

esp_err_t LED_setBAT(led_colour_t colour, uint8_t brightness_percent, uint16_t t_on_ms, uint16_t t_off_ms, uint16_t blinks_number){
#if !defined(LED_POS_BAT)
	return ESP_FAIL;
#elif LED_CHECK_IF_WS(LED_POS_BAT)
	return LED_blinkWS(LED_POS_BAT, colour, brightness_percent, t_on_ms, t_off_ms, blinks_number);
#elif LED_CHECK_IF_STD(LED_POS_BAT)
	return LED_blinkSTD(LED_POS_BAT, t_on_ms, t_off_ms, blinks_number);
#elif (LED_POS_BAT == -1)
	return ESP_FAIL;
#else
#error "LED BAT not found in configuration"
#endif
}

esp_err_t LED_setREADY(led_colour_t colour, uint8_t brightness_percent, uint16_t t_on_ms, uint16_t t_off_ms, uint16_t blinks_number){
#if !defined(LED_POS_READY)
	return ESP_FAIL;
#elif LED_CHECK_IF_WS(LED_POS_READY)
	return LED_blinkWS(LED_POS_READY, colour, brightness_percent, t_on_ms, t_off_ms, blinks_number);
#elif LED_CHECK_IF_STD(LED_POS_READY)
	return LED_blinkSTD(LED_POS_READY, t_on_ms, t_off_ms, blinks_number);
#elif (LED_POS_READY == -1)
	return ESP_FAIL;
#else
#error "LED READY not found in configuration"
#endif
}

esp_err_t LED_setIGN(uint8_t ign_no, uint8_t brightness_percent, int8_t state){
	if(ign_no > LED_IGN_NUM)
		return ESP_ERR_NOT_SUPPORTED;

	if(LED_CHECK_IF_WS(LED_POS_IGN[ign_no])){
		led_colour_t colour = (state==-1)?COLOUR_RED:(state?COLOUR_GREEN:COLOUR_ORANGE);
		return LED_blinkWS(LED_POS_IGN[ign_no], colour, brightness_percent, 400, 0, 1);
	}
	else if(LED_CHECK_IF_STD(LED_POS_IGN[ign_no])){
		//return LED_blinkSTD(LED_POS_IGN1, t_on_ms, t_off_ms, blinks_number);
		return 0;
	}

	return ESP_ERR_INVALID_ARG;
}

esp_err_t LED_setRF(led_colour_t colour, uint8_t brightness_percent, uint16_t t_on_ms, uint16_t t_off_ms, uint16_t blinks_number){
#if !defined(LED_POS_RF)
	return ESP_FAIL;
#elif LED_CHECK_IF_WS(LED_POS_RF)
	return LED_blinkWS(LED_POS_RF, colour, brightness_percent, t_on_ms, t_off_ms, blinks_number);
#elif LED_CHECK_IF_STD(LED_POS_RF)
	return LED_blinkSTD(LED_POS_RF, t_on_ms, t_off_ms, blinks_number);
#elif (LED_POS_RF == -1)
	return ESP_FAIL;
#else
#error "LED READY not found in configuration"
#endif
}

esp_err_t LED_setBrigthnessGlobal(uint8_t percentage){
	if(LED_brightness_percentage > 100)
		LED_brightness_percentage = 100;

	LED_brightness_percentage = percentage;
	return ESP_OK;
}

//------------------------------------------------
//   Low Level
//------------------------------------------------

static esp_err_t ws2812_control_init(void) {
	rmt_tx_channel_config_t channel_config = {
		.gpio_num = STRIP_LED_GPIO,
		.clk_src = RMT_CLK_SRC_DEFAULT,
		.resolution_hz = 10000000,
		.mem_block_symbols = 48,
		.trans_queue_depth = 1,
	};
	rmt_bytes_encoder_config_t encoder_config = {
		.bit0 = {
			.duration0 = WS_T0H,
			.level0 = 1,
			.duration1 = WS_T0L,
			.level1 = 0,
		},
		.bit1 = {
			.duration0 = WS_T1H,
			.level0 = 1,
			.duration1 = WS_T1L,
			.level1 = 0,
		},
		.flags.msb_first = 1,
	};
	rmt_channel_handle_t tx_channel = NULL;
	rmt_encoder_handle_t bytes_encoder = NULL;
	esp_err_t err;

	ESP_LOGI(TAG, "Strip LED init start");
	err = rmt_new_tx_channel(&channel_config, &tx_channel);
	if(err != ESP_OK){
		ESP_LOGE(TAG, "Failed to create RMT TX channel: %s", esp_err_to_name(err));
		return err;
	}

	err = rmt_new_bytes_encoder(&encoder_config, &bytes_encoder);
	if(err != ESP_OK){
		ESP_LOGE(TAG, "Failed to create WS2812 RMT encoder: %s", esp_err_to_name(err));
		rmt_del_channel(tx_channel);
		return err;
	}

	err = rmt_enable(tx_channel);
	if(err != ESP_OK){
		ESP_LOGE(TAG, "Failed to enable RMT TX channel: %s", esp_err_to_name(err));
		rmt_del_encoder(bytes_encoder);
		rmt_del_channel(tx_channel);
		return err;
	}

	ws2812_tx_channel = tx_channel;
	ws2812_bytes_encoder = bytes_encoder;
	ESP_LOGI(TAG, "Strip LED init");
	return ESP_OK;
}

static esp_err_t ws2812_control_deinit(void) {
	if(ws2812_tx_channel == NULL && ws2812_bytes_encoder == NULL){
		return ESP_OK;
	}

	if(ws2812_tx_channel != NULL){
		esp_err_t err = rmt_tx_wait_all_done(ws2812_tx_channel, -1);
		if(err != ESP_OK){
			ESP_LOGE(TAG, "Failed waiting for RMT TX completion: %s", esp_err_to_name(err));
			return err;
		}

		err = rmt_disable(ws2812_tx_channel);
		if(err != ESP_OK){
			ESP_LOGE(TAG, "Failed to disable RMT TX channel: %s", esp_err_to_name(err));
			return err;
		}

		err = rmt_del_channel(ws2812_tx_channel);
		if(err != ESP_OK){
			ESP_LOGE(TAG, "Failed to delete RMT TX channel: %s", esp_err_to_name(err));
			return err;
		}
		ws2812_tx_channel = NULL;
	}

	if(ws2812_bytes_encoder != NULL){
		esp_err_t err = rmt_del_encoder(ws2812_bytes_encoder);
		if(err != ESP_OK){
			ESP_LOGE(TAG, "Failed to delete WS2812 RMT encoder: %s", esp_err_to_name(err));
			return err;
		}
		ws2812_bytes_encoder = NULL;
	}

	ESP_LOGI(TAG, "Strip LED deinit");
	return ESP_OK;
}

static esp_err_t  ws2812_update(void) {
	if(ws2812_tx_channel == NULL || ws2812_bytes_encoder == NULL){
		return ESP_ERR_INVALID_STATE;
	}

	const rmt_transmit_config_t transmit_config = {
		.loop_count = 0,
		.flags.eot_level = 0,
	};
	esp_err_t err = rmt_transmit(ws2812_tx_channel, ws2812_bytes_encoder,
								 led_data_buffer, sizeof(led_data_buffer), &transmit_config);
	if(err != ESP_OK){
		ESP_LOGE(TAG, "RMT transmit failed: %s", esp_err_to_name(err));
		return err;
	}

	err = rmt_tx_wait_all_done(ws2812_tx_channel, -1);
	if(err != ESP_OK){
		ESP_LOGE(TAG, "RMT transmit completion wait failed: %s", esp_err_to_name(err));
		return err;
	}
	return ESP_OK;
}

static esp_err_t setup_rmt_data_buffer(void) {
#if (LED_WS_COUNT > 0) && (LED_WS_PIN != -1)
	uint8_t blank = 0;
	for (uint32_t i = 0; i < LED_WS_RGB_COUNT; i++) {
		led_data_buffer[i] = led_array[i].state ?
				((uint16_t)led_array[i].bright * LED_brightness_percentage) / 100 : blank;
	}
	return ws2812_update();
#else
	return ESP_OK;
#endif
}

static esp_err_t strip_led_colour(uint8_t number, led_colour_t colour, uint8_t brightness_percent) {
	uint8_t *rgb = (uint8_t*)&colour; //read colour in 8bit chunks
	for (int8_t i = STRIP_LED_COLOURS - 1; i >= 0; i--){
		led_array[number * STRIP_LED_COLOURS + i].bright = rgb[2-i]* brightness_percent / 100;
	}
	ESP_LOGV(TAG, "Strip LED %d colour set to: %X %X %X", number, rgb[2], rgb[1], rgb[0]);
	return ESP_OK;
}

static esp_err_t strip_led_mode(uint8_t number, led_mode_t mode) {
	for (int8_t i = STRIP_LED_COLOURS - 1; i >= 0; i--){
		led_array[number * STRIP_LED_COLOURS + i].mode = mode;
	}
	ESP_LOGV(TAG, "Strip LED %d mode set to: %d", number, mode);
	return ESP_OK;
}

static esp_err_t strip_led_blink_pulses(uint8_t number, uint16_t pulses) {
	for (int8_t i = STRIP_LED_COLOURS - 1; i >= 0; i--){
		led_array[number * STRIP_LED_COLOURS + i].pulses = pulses;
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
		led_array[number * STRIP_LED_COLOURS + i].off_time_tics = off_time_tics;
		led_array[number * STRIP_LED_COLOURS + i].on_time_tics = on_time_tics;
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

#if (LED_2_PIN != -1) && (LED_STD_COUNT > 0)
	if (i >= (LED_WS_RGB_COUNT) && i < BUZZER_ARRAY_POS )
	{
		gpio_set_level(LED_2_PIN, state);
		ESP_LOGV(TAG, "LED %d set %d", LED_2_PIN, state);
	}
#endif

#if (BUZZER_PIN != -1) && (BUZZER_COUNT > 0)
	if( i >= BUZZER_ARRAY_POS)
	{
	#if BUZZER_GENERATOR == 1
		gpio_set_level(BUZZER_PIN, state);
	#else
			//Passive buzzer code
	#endif
	}
#endif

	return ESP_OK;
}

esp_err_t LED_set(uint8_t led_no, uint8_t state){
	if (state == 0) {
		led_mode(LED_WS_RGB_COUNT + led_no, LED_MODE_OFF);
	}
	else{
		led_mode(LED_WS_RGB_COUNT + led_no, LED_MODE_ON);
	}
	return ESP_OK;
}

esp_err_t BUZZER_set(uint8_t state){
	if (state == 0) {
		led_mode(BUZZER_ARRAY_POS, LED_MODE_OFF);
	}
	else{
		led_mode(BUZZER_ARRAY_POS, LED_MODE_ON);
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
