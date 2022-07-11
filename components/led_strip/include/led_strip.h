#ifndef WS2812_CONTROL_H
#define WS2812_CONTROL_H
#include <stdint.h>
#include "sdkconfig.h"
#include "BOARD.h"
#define NUM_LEDS WS_LED_NUMBER

// This structure is used for indicating what the colors of each LED should be set to.
// There is a 32bit value for each LED. Only the lower 3 bytes are used and they hold the
// Red (byte 2), Green (byte 1), and Blue (byte 0) values to be set.
static struct led_state_t {
    uint32_t leds[NUM_LEDS];
} new_state;

// Setup the hardware peripheral. Only call this once.
void ws2812_control_init(void);

// Update the LEDs to the new state. Call as needed.
// This function will block the current task until the RMT peripheral is finished sending
// the entire sequence.


void ws2812_write_led_number(uint8_t number, uint32_t R, uint32_t G, uint32_t B);

#endif
