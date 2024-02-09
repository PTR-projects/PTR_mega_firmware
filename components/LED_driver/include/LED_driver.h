#ifndef WS2812_CONTROL_H
#define WS2812_CONTROL_H
#include <stdint.h>
#include "sdkconfig.h"
#include "BOARD.h"
#include "driver/rmt.h"

//Pulse mode names
typedef enum{
    LED_MODE_OFF,
    LED_MODE_ON,
    LED_MODE_BLINK,
    LED_MODE_PULSE
} led_mode_t;

typedef enum{
    COLOUR_RED = 0x00FF00,
	COLOUR_GREEN = 0xFF0000,
	COLOUR_BLUE = 0x0000FF,
	COLOUR_YELLOW = 0xABAB00,
	COLOUR_WHITE = 0xFFFFFF,
	COLOUR_AQUA = 0xAB0055,
	COLOUR_PURPLE = 0x0055AB,
	COLOUR_PINK = 0x00AB55,
	COLOUR_ORANGE = 0x55AB00
} led_colour_t; //Colours in GRB NOT RGB

typedef enum{
	LED_READY 	= LED_POS_READY,
	LED_ARM 	= LED_POS_ARM,
	LED_STAT 	= LED_POS_STAT,
	LED_RF 		= LED_POS_RF,
	LED_IGN1 	= LED_POS_IGN1,
	LED_IGN2 	= LED_POS_IGN2,
	LED_IGN3 	= LED_POS_IGN3,
	LED_IGN4 	= LED_POS_IGN4,
	BUZZER		= 100
} led_enum_t;

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

//Led configuration change command
typedef struct{
	led_enum_t LED_no;
	LED_t config;
} led_cmd_t;

//Main User Interface

//#define SRV_CLOCK 100
//STRIP LED DEFINITIONS

#define STRIP_LED_CHANNEL	0
#define BITS_PER_LED_CMD 8
#define STRIP_LED_COLOURS 3
#define STRIP_LED_GPIO LED_WS_PIN

#define LED_WS_RGB_COUNT 		(LED_WS_COUNT * STRIP_LED_COLOURS)
#define LED_WS_BUFFER_ITEMS 	((LED_WS_COUNT * BITS_PER_LED_CMD * STRIP_LED_COLOURS))
#define LED_ARRAY_SIZE 			(LED_WS_RGB_COUNT + LED_STD_COUNT + BUZZER_COUNT)
#define BUZZER_ARRAY_POS 		(LED_WS_RGB_COUNT + LED_STD_COUNT)

#define LED_CHECK_IF_WS(x)  ((x >= LED_WS_POS0)  && (x < (LED_WS_POS0+LED_WS_COUNT))  && (x != -1))
#define LED_CHECK_IF_STD(x) ((x >= LED_STD_POS0) && (x < (LED_STD_POS0+LED_STD_COUNT)) && (x != -1))

// HIGH/LOW times for StripLED
#define WS_T0H 3  // 0 bit high time
#define WS_T1H 7  // 1 bit high time
#define WS_T0L 7  // 0 bit low time
#define WS_T1L 3  // 1 bit low time


esp_err_t LED_srv(void); //Update LED status task should be run in SRV_CLOCK intervals

// LED
esp_err_t LED_init(uint32_t interval_ms); 	//Initialize LED and Strip LED
esp_err_t LED_set(uint8_t led_no, uint8_t state); 		//Set normal LED on/off
esp_err_t LED_blinkSTD(uint8_t led_no, uint16_t t_on_ms, uint16_t t_off_ms, uint16_t blinks_number); // Blink LED on/off time in MS 0 beeps number means infinite

//Strip LED
esp_err_t LED_setWS(uint8_t led_no, led_colour_t colour, uint8_t brightness_percent, uint8_t state); 	//Set strip LED ON/OFF Brightness 0-255
esp_err_t LED_blinkWS(int16_t led_no, led_colour_t colour, uint8_t brightness_percent, uint16_t t_on_ms, uint16_t t_off_ms, uint16_t blinks_number);	// Blink strip LED on/off time in MS 0 beeps number means infinite

//Buzzer
esp_err_t BUZZER_init(); //Initialize Buzzer
esp_err_t BUZZER_set(uint8_t state); 	//Set buzzer LED ON/OFF
esp_err_t BUZZER_beep(uint16_t t_on_ms, uint16_t t_off_ms, uint16_t beeps_number); // Beep Buzzer on/off in MS 0 beeps number means infinite

//High Level LED/Buzzer wrapper
esp_err_t LED_setARM(led_colour_t colour, uint8_t brightness_percent, uint16_t t_on_ms, uint16_t t_off_ms, uint16_t blinks_number);
esp_err_t LED_setSTAT(led_colour_t colour, uint8_t brightness_percent, uint16_t t_on_ms, uint16_t t_off_ms, uint16_t blinks_number);
esp_err_t LED_setREADY(led_colour_t colour, uint8_t brightness_percent, uint16_t t_on_ms, uint16_t t_off_ms, uint16_t blinks_number);
esp_err_t LED_setIGN(uint8_t ign_no, uint8_t brightness_percent, int8_t state);
esp_err_t LED_setRF(led_colour_t colour, uint8_t brightness_percent, uint16_t t_on_ms, uint16_t t_off_ms, uint16_t blinks_number);
esp_err_t LED_setBrigthnessGlobal(uint8_t percentage);

#endif
