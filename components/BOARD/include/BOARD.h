#if defined (CONFIG_BOARD_PTR_MEGA_VER_0_REV_1) || defined (CONFIG_BOARD_PTR_MEGA_VER_1_REV_0) || defined (CONFIG_BOARD_ARECORDER_VER_3_REV_0)

#ifdef CONFIG_BOARD_PTR_MEGA_VER_0_REV_1
#define SPI_SLAVE_MS5607_0_PIN	41
#define SPI_SLAVE_MS5607_PINS {SPI_SLAVE_MS5607_0_PIN}
#define MS5607_COUNT 1

#define SPI_SLAVE_LIS331_0_PIN	36
#define SPI_SLAVE_LIS331_PINS {SPI_SLAVE_LIS331_0_PIN}
#define LIS331_TYPES {100}
#define LIS331_COUNT 1

#define SPI_SLAVE_LSM6DSO32_0_PIN	35
#define SPI_SLAVE_LSM6DSO32_PINS {SPI_SLAVE_LSM6DSO32_0_PIN}
#define LSM6DSO32_COUNT 1

#define SPI_SLAVE_FLASH_PIN		33
#define SPI_SLAVE_MMC5983MA_PIN	34
#define SPI_SLAVE_SX1262_PIN	42

#define SPI_MISO_PIN			38
#define SPI_MOSI_PIN			39
#define SPI_SCK_PIN				40

#define LED_WS_PIN				47
#define LED_2_PIN				46
#define BUZZER_PIN				45

#define RF_BUSY_PIN				37
#define RF_RST_PIN				3

#define SERVO_EN_PIN			48
#define SERVO1_PIN				17
#define SERVO2_PIN				18
#define SERVO3_PIN				21
#define SERVO4_PIN				26
#define BOARD_SERVO_PWM_NUM		4
#define BOARD_SERVO_PWM_PINS 	{SERVO1_PIN, SERVO2_PIN, SERVO3_PIN, SERVO4_PIN}


#define I2C_SDA_PIN				15
#define I2C_SCL_PIN				16

#define GNSS_RX_PIN				1
#define GNSS_TX_PIN				2
#define GNSS_UART 				UART_NUM_2

#define UART_EXT_OUT			13
#define UART_EXT_IN				14

#define IGN_NUM					4
#define IGN1_EN_PIN				9
#define IGN2_EN_PIN				10
#define IGN3_EN_PIN				11
#define IGN4_EN_PIN				12
#define IGN1_DET_PIN			7
#define IGN2_DET_PIN			6
#define IGN3_DET_PIN			5
#define IGN4_DET_PIN			4
#define IGN_EN_PINS_LIST		IGN1_EN_PIN, IGN2_EN_PIN, IGN3_EN_PIN, IGN4_EN_PIN
#define IGN_DET_PINS_LIST		ADC_CHANNEL_6, ADC_CHANNEL_5, ADC_CHANNEL_4, ADC_CHANNEL_3

#define VBAT_MEAS_PIN			ADC_CHANNEL_7
#define ADC_CHANNELS_LIST		VBAT_MEAS_PIN, IGN_DET_PINS_LIST
#define ADC_CHANNELS_NUM		5

#define LED_WS_COUNT			3
#define LED_STD_COUNT			1
#define BUZZER_COUNT			1

#define LED_WS_POS0				0
#define LED_STD_POS0			(LED_WS_POS0 + LED_WS_COUNT)
#define BUZZER_POS0				(LED_WS_COUNT + LED_STD_COUNT)

#define LED_POS_READY			(LED_WS_POS0+0)
#define LED_POS_ARM				(LED_WS_POS0+1)
#define LED_POS_STAT			(LED_WS_POS0+2)
#define LED_POS_IGN1			-1
#define LED_POS_IGN2			-1
#define LED_POS_IGN3			-1
#define LED_POS_IGN4			-1
#define LED_IGN_NUM				4
#define LED_POS_IGN_LIST		LED_POS_IGN1, LED_POS_IGN2, LED_POS_IGN3, LED_POS_IGN4
#define LED_POS_RF				(LED_STD_POS0+0)

#define BUZZER_GENERATOR		1
#define BUZZER_POS				BUZZER_POS0
#endif

#ifdef CONFIG_BOARD_PTR_MEGA_VER_1_REV_0
#define SPI_SLAVE_MS5607_0_PIN	41
#define SPI_SLAVE_MS5607_PINS {SPI_SLAVE_MS5607_0_PIN}
#define MS5607_COUNT 1

#define SPI_SLAVE_LIS331_0_PIN	36
#define SPI_SLAVE_LIS331_PINS {SPI_SLAVE_LIS331_0_PIN}
#define LIS331_TYPES {100}
#define LIS331_COUNT 1

#define SPI_SLAVE_LSM6DSO32_0_PIN	35
#define SPI_SLAVE_LSM6DSO32_1_PIN	33
#define SPI_SLAVE_LSM6DSO32_PINS {SPI_SLAVE_LSM6DSO32_0_PIN, SPI_SLAVE_LSM6DSO32_1_PIN}
#define LSM6DSO32_COUNT 2

#define SPI_SLAVE_MMC5983MA_PIN	34
#define SPI_SLAVE_SX1262_PIN	42

#define SPI_MISO_PIN			38
#define SPI_MOSI_PIN			39
#define SPI_SCK_PIN				40

#define LED_WS_PIN				48
//#define LED_2_PIN				46
#define BUZZER_PIN				45

#define RF_BUSY_PIN				37
#define RF_RST_PIN				3

#define SERVO_EN_PIN			46
#define SERVO1_PIN				17
#define SERVO2_PIN				18
#define SERVO3_PIN				21
#define SERVO4_PIN				26
#define BOARD_SERVO_PWM_NUM		4
#define BOARD_SERVO_PWM_PINS 	{SERVO1_PIN, SERVO2_PIN, SERVO3_PIN, SERVO4_PIN}

#define I2C_SDA_PIN				15
#define I2C_SCL_PIN				16

#define GNSS_RX_PIN				1
#define GNSS_TX_PIN				2
#define GNSS_UART 				UART_NUM_2

#define UART_EXT_OUT			13
#define UART_EXT_IN				14

#define IGN_NUM					4
#define IGN1_EN_PIN				12
#define IGN2_EN_PIN				10
#define IGN3_EN_PIN				11
#define IGN4_EN_PIN				9
#define IGN1_DET_PIN			7
#define IGN2_DET_PIN			6
#define IGN3_DET_PIN			5
#define IGN4_DET_PIN			4
#define IGN_EN_PINS_LIST		IGN1_EN_PIN, IGN2_EN_PIN, IGN3_EN_PIN, IGN4_EN_PIN
#define IGN_DET_PINS_LIST		ADC_CHANNEL_6, ADC_CHANNEL_5, ADC_CHANNEL_4, ADC_CHANNEL_3

#define VBAT_MEAS_PIN			ADC_CHANNEL_7
#define ADC_CHANNELS_LIST		VBAT_MEAS_PIN, IGN_DET_PINS_LIST
#define ADC_CHANNELS_NUM		5

#define LED_WS_COUNT			8
#define LED_STD_COUNT			0
#define BUZZER_COUNT			1

#define LED_WS_POS0				0
#define LED_STD_POS0			(LED_WS_POS0  + LED_WS_COUNT)
#define BUZZER_POS0				(LED_WS_COUNT + LED_STD_COUNT)

#define LED_POS_READY			(LED_WS_POS0+0)
#define LED_POS_ARM				(LED_WS_POS0+1)
#define LED_POS_STAT			(LED_WS_POS0+2)
#define LED_POS_IGN1			(LED_WS_POS0+7)
#define LED_POS_IGN2			(LED_WS_POS0+6)
#define LED_POS_IGN3			(LED_WS_POS0+5)
#define LED_POS_IGN4			(LED_WS_POS0+4)
#define LED_IGN_NUM				4
#define LED_POS_IGN_LIST		LED_POS_IGN1, LED_POS_IGN2, LED_POS_IGN3, LED_POS_IGN4
#define LED_POS_RF				(LED_WS_POS0+3)

#define BUZZER_GENERATOR		1
#define BUZZER_POS				BUZZER_POS0

#elif defined CONFIG_BOARD_ARECORDER_VER_3_REV_0
#define SPI_SLAVE_MS5607_0_PIN	41
#define SPI_SLAVE_MS5607_PINS {SPI_SLAVE_MS5607_0_PIN}
#define MS5607_COUNT 1

#define SPI_SLAVE_LIS331_0_PIN	37
#define SPI_SLAVE_LIS331_PINS {SPI_SLAVE_LIS331_0_PIN}
#define LIS331_TYPES {100}
#define LIS331_COUNT 1

#define SPI_SLAVE_LSM6DSO32_0_PIN	36
#define SPI_SLAVE_LSM6DSO32_PINS {SPI_SLAVE_LSM6DSO32_0_PIN}
#define LSM6DSO32_COUNT 1

#define SPI_MISO_PIN			38
#define SPI_MOSI_PIN			39
#define SPI_SCK_PIN				40

#define LED_WS_PIN				35
#define BUZZER_PIN				12

#define I2C_SDA_PIN				45
#define I2C_SCL_PIN				46

#define UART_EXT_OUT			2
#define UART_EXT_IN				3

#define IGN_NUM					3
#define IGN1_EN_PIN				9
#define IGN2_EN_PIN				8
#define IGN3_EN_PIN				7
#define IGN1_DET_PIN			6
#define IGN2_DET_PIN			5
#define IGN3_DET_PIN			4
#define IGN_EN_PINS_LIST		IGN1_EN_PIN, IGN2_EN_PIN, IGN3_EN_PIN
#define IGN_DET_PINS_LIST		ADC_CHANNEL_5, ADC_CHANNEL_4, ADC_CHANNEL_3


#define VBAT_MEAS_PIN			ADC_CHANNEL_9
#define ADC_CHANNELS_LIST		VBAT_MEAS_PIN, IGN_DET_PINS_LIST
#define ADC_CHANNELS_NUM		4

#define LED_WS_COUNT			4
#define LED_STD_COUNT			0
#define BUZZER_COUNT			1

#define LED_WS_POS0				0
#define LED_STD_POS0			(LED_WS_POS0  + LED_WS_COUNT)
#define BUZZER_POS0				(LED_WS_COUNT + LED_STD_COUNT)

#define LED_POS_READY			-1
#define LED_POS_ARM				-1
#define LED_POS_STAT			(LED_WS_POS0+0)
#define LED_POS_IGN1			(LED_WS_POS0+3)
#define LED_POS_IGN2			(LED_WS_POS0+2)
#define LED_POS_IGN3			(LED_WS_POS0+1)
#define LED_POS_IGN4			-1
#define LED_POS_RF				-1
#define LED_IGN_NUM				3
#define LED_POS_IGN_LIST		LED_POS_IGN1, LED_POS_IGN2, LED_POS_IGN3

#define BUZZER_GENERATOR		1
#define BUZZER_POS				BUZZER_POS0

#endif

#else
#error "This board version don't exist or require new definition."

#endif
