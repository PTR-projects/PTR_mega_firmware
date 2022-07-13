#ifndef WS2812_CONTROL_H
#define WS2812_CONTROL_H
#include <stdint.h>
#include "sdkconfig.h"
#include "driver/rmt.h"

#define NUM_LEDS 1

// This structure is used for indicating what the colors of each LED should be set to.
// There is a 32bit value for each LED. Only the lower 3 bytes are used and they hold the
// Red (byte 2), Green (byte 1), and Blue (byte 0) values to be set.

typedef enum{
    LED_MODE_OFF,
    LED_MODE_ON,
    LED_MODE_BLINK,
    LED_MODE_PULSE
} led_mode_t;

typedef struct{

    led_mode_t mode;
    uint8_t bright;
    uint16_t on_time_tics;
    uint16_t off_time_tics;
    uint16_t pulses;
    uint16_t counter;
    uint8_t state;
} LED_t;


void LED_srv();
void ws_led_blink_rate(uint8_t number, uint16_t on_time_tics, uint16_t off_time_tics);
void ws_led_mode(uint8_t number, led_mode_t mode);
void ws_led_colour(uint8_t number, uint8_t R, uint8_t G, uint8_t B);

void ws2812_write_led_number(uint8_t number, uint8_t R, uint8_t G, uint8_t B);

// Setup the hardware peripheral. Only call this once.
void ws2812_control_init(void);

// Update the LEDs to the new state. Call as needed.
// This function will block the current task until the RMT peripheral is finished sending
// the entire sequence.




#endif
