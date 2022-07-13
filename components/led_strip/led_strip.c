#include "led_strip.h"

#include "esp_log.h"


#define LED_RMT_TX_CHANNEL			0
#define LED_RMT_TX_GPIO			4


#define BITS_PER_LED_CMD 8
#define LED_BUFFER_ITEMS ((NUM_LEDS * BITS_PER_LED_CMD))

// These values are determined by measuring pulse timing with logic analyzer and adjusting to match datasheet.
#define T0H 3  // 0 bit high time
#define T1H 7  // 1 bit high time
#define T0L 7  // 0 bit low time
#define T1L 3  // 1 bit low time

static const char* TAG = "Led_strip";

// This is the buffer which the hw peripheral will access while pulsing the output pin
rmt_item32_t led_data_buffer[LED_BUFFER_ITEMS];
static LED_t ws_led[NUM_LEDS*3];


void ws2812_control_init(void)
{
  rmt_config_t config;
  config.rmt_mode = RMT_MODE_TX;
  config.channel = LED_RMT_TX_CHANNEL;
  config.gpio_num = LED_RMT_TX_GPIO;
  config.mem_block_num = 3;
  config.tx_config.loop_en = false;
  config.tx_config.carrier_en = false;
  config.tx_config.idle_output_en = true;
  config.tx_config.idle_level = 0;
  config.tx_config.carrier_freq_hz = 10*1000*1000;
  config.clk_div = 8;

  ESP_ERROR_CHECK(rmt_config(&config));
  ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
  ESP_LOGI(TAG, "inicjalizacja");

}

void ws2812_update() {
  ESP_ERROR_CHECK(rmt_write_items(LED_RMT_TX_CHANNEL, led_data_buffer, LED_BUFFER_ITEMS, false));
  ESP_ERROR_CHECK(rmt_wait_tx_done(LED_RMT_TX_CHANNEL, portMAX_DELAY));
}

void setup_rmt_data_buffer(void)
{
	uint8_t blank = 0;
  for (uint32_t led = 0; led < NUM_LEDS * 3; led++) {
    uint8_t bits_to_send = ws_led[led].state ? ws_led[led].bright : blank ;
    uint8_t mask = 1 << (BITS_PER_LED_CMD - 1);
    for (uint8_t bit = 0; bit < BITS_PER_LED_CMD; bit++) {
      uint8_t bit_is_set = bits_to_send & mask;
      printf("%x \n", bit_is_set);
      led_data_buffer[led * BITS_PER_LED_CMD + bit] = bit_is_set ?
                                                      (rmt_item32_t){{{T1H, 1, T1L, 0}}} :
                                                      (rmt_item32_t){{{T0H, 1, T0L, 0}}};
      mask >>= 1;
    }
  }
ws2812_update();
}



void ws2812_write_led_number(uint8_t number, uint8_t R, uint8_t G, uint8_t B){

	ws_led[number*3].bright = R;
	ws_led[number*3+1].bright = G;
	ws_led[number*3+2].bright = B;

}
void ws_led_colour(uint8_t number, uint8_t R, uint8_t G, uint8_t B){
	ws_led[number*3].bright = R;
	ws_led[number*3+1].bright = G;
	ws_led[number*3+2].bright = B;

}

void ws_led_mode(uint8_t number, led_mode_t mode){
	ws_led[number*3].mode = mode;
	ws_led[number*3+1].mode = mode;
	ws_led[number*3+2].mode = mode;
}

void ws_led_blink_rate(uint8_t number, uint16_t on_time_tics, uint16_t off_time_tics){
	ws_led[number*3].off_time_tics = off_time_tics;
	ws_led[number*3+1].off_time_tics = off_time_tics;
	ws_led[number*3+2].off_time_tics = off_time_tics;
	ws_led[number*3].on_time_tics = on_time_tics;
	ws_led[number*3+1].on_time_tics = on_time_tics;
	ws_led[number*3+2].on_time_tics = on_time_tics;
}


void LED_srv(){

	for (uint8_t i=0;i<(NUM_LEDS*3); i++) {

		switch (ws_led[i].mode){
			case LED_MODE_ON : ws_led[i].state = 1;
			break;

			case LED_MODE_OFF : ws_led[i].state = 0;
			break;

			case LED_MODE_BLINK :

				if (ws_led[i].state == 0) {
					if (++ws_led[i].counter > ws_led[i].off_time_tics ) {
						ws_led[i].state = 1;
						ws_led[i].counter = 0;
					}
				}
				if (ws_led[i].state == 1)	{
					if (++ws_led[i].counter > ws_led[i].on_time_tics ) {
						ws_led[i].state = 0;
						ws_led[i].counter = 0;
					}
				}
			break;

			case LED_MODE_PULSE :
				if (ws_led[i].state == 0) {
						ws_led[i].state = 1;
						ws_led[i].counter = 0;
				}

				if (ws_led[i].state == 1)	{
					if (++ws_led[i].counter > ws_led[i].on_time_tics ) {
						ws_led[i].state = 0;
						ws_led[i].counter = 0;
					}
				}
			break;
			default: ;
		}
	}
	setup_rmt_data_buffer();
}
