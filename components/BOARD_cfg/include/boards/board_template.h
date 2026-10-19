#pragma once

/*
 * Template for a new board. This file is not included by BOARD_cfg.h.
 *
 * To add a board:
 *   1. Copy this file to boards/board_<name>.h and fill in the macros.
 *   2. Add a PlatformIO environment in platformio.ini with -DCONFIG_BOARD_<NAME>.
 *   3. Add an #elif include for that macro in BOARD_cfg.h.
 *   4. Add an axis-remap branch in components/Sensors/Sensors.c
 *      (Sensors_axes_translation). That step is not part of the pin map.
 *
 * Omit an optional macro to disable that feature. Drivers that already stub
 * themselves when the chip-select is missing: MS5607, LIS331, BMP581,
 * MMC5983MA, SX1262, GNSS, and PWM servos.
 * Use -1 for an LED position that is not populated.
 *
 * Required on every current flight board: SPI bus, LSM6DSO32 count and pin
 * list, I2C, external UART, ignition lists, ADC channel list, LED/buzzer map,
 * and CONFIG_ESP_WIFI_SSID. MS5607 and LIS331 are on those boards too, but a
 * new board may omit them (PTR-micro uses BMP581 and has no high-G sensor).
 */

/* Identity
#define CONFIG_ESP_WIFI_SSID "PTR-xxxx"
*/

/* SPI bus — required when any SPI device is fitted
#define SPI_MISO_PIN            -1
#define SPI_MOSI_PIN            -1
#define SPI_SCK_PIN             -1
*/

/* MS5607 barometer — omit the whole group to stub the driver
#define SPI_SLAVE_MS5607_0_PIN  -1
#define SPI_SLAVE_MS5607_PINS   {SPI_SLAVE_MS5607_0_PIN}
#define MS5607_COUNT            1
*/

/* BMP581 barometer — omit to stub the driver. Do not define both barometers.
#define SPI_SLAVE_BMP581_PIN    -1
*/

/* LIS331 high-G accelerometer — omit the whole group to stub the driver
#define SPI_SLAVE_LIS331_0_PIN  -1
#define SPI_SLAVE_LIS331_PINS   {SPI_SLAVE_LIS331_0_PIN}
#define LIS331_TYPES            {100}
#define LIS331_COUNT            1
*/

/* LSM6DSO32 — required on current boards. Add _1_PIN when COUNT is 2.
#define SPI_SLAVE_LSM6DSO32_0_PIN   -1
#define SPI_SLAVE_LSM6DSO32_PINS    {SPI_SLAVE_LSM6DSO32_0_PIN}
#define LSM6DSO32_COUNT             1
*/

/* Optional SPI devices
#define SPI_SLAVE_FLASH_PIN     -1
#define SPI_SLAVE_MMC5983MA_PIN -1
#define SPI_SLAVE_SX1262_PIN    -1
#define RF_BUSY_PIN             -1
#define RF_RST_PIN              -1
*/

/* LEDs and buzzer. LED_2_PIN is optional (standard LED, not the WS2812 strip).
#define LED_WS_PIN              -1
#define LED_2_PIN               -1
#define BUZZER_PIN              -1

#define LED_WS_COUNT            0
#define LED_STD_COUNT           0
#define BUZZER_COUNT            0

#define LED_WS_POS0             0
#define LED_STD_POS0            (LED_WS_POS0 + LED_WS_COUNT)
#define BUZZER_POS0             (LED_WS_COUNT + LED_STD_COUNT)

#define LED_POS_READY           -1
#define LED_POS_ARM             -1
#define LED_POS_STAT            -1
#define LED_POS_IGN1            -1
#define LED_POS_IGN2            -1
#define LED_POS_IGN3            -1
#define LED_POS_IGN4            -1
#define LED_POS_RF              -1
#define LED_IGN_NUM             0
#define LED_POS_IGN_LIST        LED_POS_IGN1

#define BUZZER_GENERATOR        1
#define BUZZER_POS              BUZZER_POS0
*/

/* PWM servos — omit BOARD_SERVO_PWM_NUM to stub the driver
#define SERVO_EN_PIN            -1
#define SERVO1_PIN              -1
#define SERVO2_PIN              -1
#define SERVO3_PIN              -1
#define SERVO4_PIN              -1
#define BOARD_SERVO_PWM_NUM     4
#define BOARD_SERVO_PWM_PINS    {SERVO1_PIN, SERVO2_PIN, SERVO3_PIN, SERVO4_PIN}
*/

/* I2C — pins are reserved on current boards
#define I2C_SDA_PIN             -1
#define I2C_SCL_PIN             -1
*/

/* GNSS — omit GNSS_UART to stub the driver
#define GNSS_RX_PIN             -1
#define GNSS_TX_PIN             -1
#define GNSS_UART               UART_NUM_2
*/

/* External UART (SBUS). SBUS_driver uses these macros directly.
#define UART_EXT_OUT            -1
#define UART_EXT_IN             -1
#define UART_EXT_UART           UART_NUM_1
*/

/* Ignition and battery ADC. IGN_NUM must match the enable and detect lists.
#define IGN_NUM                 0
#define IGN1_EN_PIN             -1
#define IGN1_DET_PIN            -1
#define IGN_EN_PINS_LIST        IGN1_EN_PIN
#define IGN_DET_PINS_LIST       ADC_CHANNEL_0

#define VBAT_MEAS_PIN           ADC_CHANNEL_0
#define ADC_CHANNELS_LIST       VBAT_MEAS_PIN, IGN_DET_PINS_LIST
#define ADC_CHANNELS_NUM        1
*/
