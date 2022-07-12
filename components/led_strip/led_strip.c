#include "led_strip.h"
#include "driver/rmt.h"
#include "BOARD.h"



#define LED_RMT_TX_CHANNEL			0
#define LED_RMT_TX_GPIO			LED_WS_PIN


#define BITS_PER_LED_CMD 24
#define LED_BUFFER_ITEMS ((NUM_LEDS * BITS_PER_LED_CMD))

// These values are determined by measuring pulse timing with logic analyzer and adjusting to match datasheet.
#define T0H 3  // 0 bit high time
#define T1H 7  // 1 bit high time
#define T0L 7  // 0 bit low time
#define T1L 3  // 1 bit low time

// This is the buffer which the hw peripheral will access while pulsing the output pin
rmt_item32_t led_data_buffer[LED_BUFFER_ITEMS];



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
}


void setup_rmt_data_buffer(struct led_state_t new_state)
{
  for (uint32_t led = 0; led < NUM_LEDS; led++) {
    uint32_t bits_to_send = new_state.leds[led];
    uint32_t mask = 1 << (BITS_PER_LED_CMD - 1);
    for (uint32_t bit = 0; bit < BITS_PER_LED_CMD; bit++) {
      uint32_t bit_is_set = bits_to_send & mask;
      led_data_buffer[led * BITS_PER_LED_CMD + bit] = bit_is_set ?
                                                      (rmt_item32_t){{{T1H, 1, T1L, 0}}} :
                                                      (rmt_item32_t){{{T0H, 1, T0L, 0}}};
      mask >>= 1;
    }
  }
}

void ws2812_write_leds(struct led_state_t new_state) {
  setup_rmt_data_buffer(new_state);
  ESP_ERROR_CHECK(rmt_write_items(LED_RMT_TX_CHANNEL, led_data_buffer, LED_BUFFER_ITEMS, false));
  ESP_ERROR_CHECK(rmt_wait_tx_done(LED_RMT_TX_CHANNEL, portMAX_DELAY));
}

void ws2812_write_led_number(uint8_t number, uint32_t R, uint32_t G, uint32_t B){
	R <<= 16;
	G <<= 8;
	new_state.leds[number]=(R | G | B);
	ws2812_write_leds(new_state);
}
