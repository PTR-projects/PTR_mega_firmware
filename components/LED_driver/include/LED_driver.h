#ifndef WS2812_CONTROL_H
#define WS2812_CONTROL_H
#include <stdint.h>
#include "sdkconfig.h"
#include "driver/rmt.h"

//Pulse mode names
typedef enum{
    LED_MODE_OFF,
    LED_MODE_ON,
    LED_MODE_BLINK,
    LED_MODE_PULSE
} led_mode_t;

typedef enum{
    COLOUR_RED = 0xFF0000,
	COLOUR_GREEN = 0x00FF00,
	COLOUR_BLUE = 0x0000FF,
} led_colour_t;


//Led configuration struct
typedef struct{

    led_mode_t mode;
    uint8_t bright;
    uint16_t on_time_tics;
    uint16_t off_time_tics;
    uint16_t pulses;
    uint16_t counter;
    uint8_t state;
} LED_t;


void LED_srv(); //Update LED status task
void led_blink_rate(uint8_t number, uint8_t on_time_tics, uint8_t off_time_tics); //Set blink rate for LED
void led_mode(uint8_t number, led_mode_t mode); //Set blink mode for LED
void strip_led_colour(uint8_t number, led_colour_t colour); //Set colour of RGB LED

void setup_rmt_data_buffer(void); //Prepare strip leds set array for update
void ws2812_update(); //update strip leds


void ws2812_control_init(void); //Strip LED init


#endif
