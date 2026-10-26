#pragma once

/*
 * PTR-micro ver 0 rev 1.
 *
 * Not fitted: MS5607, LIS331, GNSS, LoRa, servo, magnetometer, SPI flash,
 * extension UART, I2C. LSM6 mounting signs are not defined yet.
 */

#define CONFIG_ESP_WIFI_SSID "PTR-micro"

#define SPI_MISO_PIN			10
#define SPI_MOSI_PIN			8
#define SPI_SCK_PIN				3

#define SPI_SLAVE_LSM6DSO32_0_PIN	9
#define SPI_SLAVE_LSM6DSO32_PINS	{SPI_SLAVE_LSM6DSO32_0_PIN}
#define LSM6DSO32_COUNT			1

#define SPI_SLAVE_BMP581_PIN	2

#define BUZZER_PIN				5

#define IGN_NUM					2
#define IGN1_EN_PIN				7
#define IGN2_EN_PIN				6
#define IGN1_DET_PIN			4
#define IGN2_DET_PIN			1
#define IGN_EN_PINS_LIST		IGN1_EN_PIN, IGN2_EN_PIN
#define IGN_DET_PINS_LIST		ADC_CHANNEL_4, ADC_CHANNEL_1

#define VBAT_MEAS_PIN			ADC_CHANNEL_0
#define ADC_CHANNELS_LIST		VBAT_MEAS_PIN, IGN_DET_PINS_LIST
#define ADC_CHANNELS_NUM		3

#define LED_WS_PIN				21
//#define LED_2_PIN				-1
#define LED_WS_COUNT			4
#define LED_STD_COUNT			0
#define BUZZER_COUNT			1

#define LED_WS_POS0				0
#define LED_STD_POS0			(LED_WS_POS0 + LED_WS_COUNT)
#define BUZZER_POS0				(LED_WS_COUNT + LED_STD_COUNT)

#define LED_POS_READY			-1
#define LED_POS_ARM				-1
#define LED_POS_STAT			(LED_WS_POS0+0)
#define LED_POS_BAT	    		(LED_WS_POS0+1)
#define LED_POS_IGN1			(LED_WS_POS0+2)
#define LED_POS_IGN2			(LED_WS_POS0+3)
#define LED_POS_IGN3			-1
#define LED_POS_IGN4			-1
#define LED_POS_RF				-1
#define LED_IGN_NUM				2
#define LED_POS_IGN_LIST		LED_POS_IGN1, LED_POS_IGN2

#define BUZZER_GENERATOR		1
#define BUZZER_POS				BUZZER_POS0
