#ifndef WS2812_CONTROL_H
#define WS2812_CONTROL_H
#include <stdint.h>
#include "sdkconfig.h"
#include "driver/rmt.h"

/**
 * @brief Enumeration of possible LED modes.
 */
typedef enum{
    LED_MODE_OFF,	/*!< LED is off. */
    LED_MODE_ON,	/*!< LED is on. */
    LED_MODE_BLINK,	/*!< LED blinks at a set interval. */
    LED_MODE_PULSE	/*!< LED pulses at a set interval. */
} led_mode_t;

/**
 * @brief Enumeration of possible LED colours.
 */
typedef enum{
    COLOUR_RED   	= 0x00FF00,
	COLOUR_GREEN 	= 0xFF0000,
	COLOUR_BLUE  	= 0x0000FF,
	COLOUR_YELLOW 	= 0xABAB00,
	COLOUR_WHITE	= 0xFFFFFF,
	COLOUR_AQUA 	= 0xAB0055,
	COLOUR_PURPLE 	= 0x0055AB,
	COLOUR_PINK 	= 0x00AB55,
	COLOUR_ORANGE 	= 0x55AB00
} led_colour_t; //Colours in GRB NOT RGB


/**
 * @brief Struct containing LED configuration data.
 */
typedef struct{

    led_mode_t mode;		/*!< Current LED mode. */
    uint8_t bright;			/*!< LED brightness. */
    uint16_t on_time_tics;	/*!< Time (in tics) that the LED is on. */
    uint16_t off_time_tics;	/*!< Time (in tics) that the LED is off. */
    uint16_t pulses;		/*!< Number of pulses. */
    uint16_t counter;		/*!< Current pulse counter. */
    uint8_t state;			/*!< Current LED state (on/off). */
} LED_t;

//Main User Interface

//#define SRV_CLOCK 100
//STRIP LED DEFINITIONS

#define STRIP_LED_CHANNEL	0
#define BITS_PER_LED_CMD 8
#define STRIP_LED_COLOURS 3
#define STRIP_LED_GPIO LED_WS_PIN
#define LED_BUFFER_ITEMS ((LED_WS_COUNT * BITS_PER_LED_CMD * STRIP_LED_COLOURS))
#define LED_ARRAY_SIZE (LED_POS + LED_STD_COUNT + BUZZER_COUNT)
#define BUZZER_POS LED_POS + LED_STD_COUNT
#define LED_POS LED_WS_COUNT * STRIP_LED_COLOURS
// HIGH/LOW times for StripLED
#define WS_T0H 3  // 0 bit high time
#define WS_T1H 7  // 1 bit high time
#define WS_T0L 7  // 0 bit low time
#define WS_T1L 3  // 1 bit low time

/**
 * @brief Update LED status task. Should be run in SRV_CLOCK intervals.
 * @return esp_err_t indicating success or error.
 */
esp_err_t LED_srv(void); //Update LED status task should be run in SRV_CLOCK intervals

/**
 * @brief Initialize LED and Strip LED.
 * @param[in] interval_ms Interval (in milliseconds) at which the LED status task should run.
 * @return esp_err_t indicating success or error.
 */
esp_err_t LED_init(uint32_t interval_ms); 	//Initialize LED and Strip LED

/**
 * @brief Set normal LED on or off.
 * @param[in] led_no Number of the LED to set.
 * @param[in] state State (on or off) to set the LED to.
 * @return esp_err_t indicating success or error.
 */
esp_err_t LED_set(uint8_t led_no, uint8_t state); 		//Set normal LED on/off

/**
 * @brief Blink LED on or off.
 * @param[in] led_no Number of the LED to blink.
 * @param[in] t_on_ms Time (in milliseconds) that the LED should be on.
 * @param[in] t_off_ms Time (in milliseconds) that the LED should be off.
 * @param[in] blinks_number Number of times the LED should blink. A value of 0 means infinite.
 * @return esp_err_t indicating success or error.
 */
esp_err_t LED_blink(uint8_t led_no, uint16_t t_on_ms, uint16_t t_off_ms, uint16_t blinks_number); // Blink LED on/off time in MS 0 beeps number means infinite

/**
 * @brief Set a strip LED on or off with a specified brightness.
 * @param[in] led_no The number of the LED to be set.
 * @param[in] colour The colour to set the strip LED to.
 * @param[in] brightness_percent The brightness of the LED (in percent).
 * @param[in] state The state to set the LED to (1 for on, 0 for off).
 * @return esp_err_t ESP_OK if successful, ESP_FAIL otherwise.
 */
esp_err_t LED_setWS(uint8_t led_no, led_colour_t colour, uint8_t brightness_percent, uint8_t state); 	//Set strip LED ON/OFF Brightness 0-255

/**
 * @brief Blink a strip LED on and off at a specified interval and brightness.
 * @param[in] led_no The number of the LED to be set.
 * @param[in] colour The colour to set the strip LED to.
 * @param[in] brightness_percent The brightness of the LED (in percent).
 * @param[in] t_on_ms The time the LED should be on (in milliseconds).
 * @param[in] t_off_ms The time the LED should be off (in milliseconds).
 * @param[in] blinks_number The number of times to blink the LED. If set to 0, the LED will blink indefinitely.
 * @return esp_err_t ESP_OK if successful, ESP_FAIL otherwise.
 */
esp_err_t LED_blinkWS(uint8_t led_no, led_colour_t colour, uint8_t brightness_percent, uint16_t t_on_ms, uint16_t t_off_ms, uint16_t blinks_number);	// Blink strip LED on/off time in MS 0 beeps number means infinite

/**
 * @brief Initializes the buzzer
 * @return ESP_OK if successful, ESP_FAIL otherwise
 */
esp_err_t BUZZER_init(); //Initialize Buzzer

/**
 * @brief Sets the state of the buzzer
 * @param[in] state 1 for on, 0 for off
 * @return ESP_OK if successful, ESP_FAIL otherwise
 */
esp_err_t BUZZER_set(uint8_t state); 	//Set buzzer LED ON/OFF

/**
 * @brief Beeps the buzzer with specified on/off times and number of beeps
 * @param[in] t_on_ms On time in milliseconds
 * @param[in] t_off_ms Off time in milliseconds
 * @param[in] beeps_number Number of beeps to make. 0 means infinite.
 * @return ESP_OK if successful, ESP_FAIL otherwise
 */
esp_err_t BUZZER_beep(uint16_t t_on_ms, uint16_t t_off_ms, uint16_t beeps_number); // Beep Buzzer on/off in MS 0 beeps number means infinite

#endif
