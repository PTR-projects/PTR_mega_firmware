
#if defined (CONFIG_BOARD_KPPTR_VER_0_REV_1) || (CONFIG_BOARD_KPPTR_VER_1_REV_0)

#ifdef CONFIG_BOARD_KPPTR_VER_0_REV_1
#define SPI_SLAVE_MS5607_PIN	41
#define SPI_SLAVE_LIS331_PIN	36
#define SPI_SLAVE_LSM6DSO32_PIN	35
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

#define I2C_SDA_PIN				15
#define I2C_SCL_PIN				16

#define GNSS_RX_PIN				1
#define GNSS_TX_PIN				2
#define GNSS_UART 				UART_NUM_2

#define UART_EXT_OUT			13
#define UART_EXT_IN				14

#define IGN1_EN_PIN				9
#define IGN2_EN_PIN				10
#define IGN3_EN_PIN				11
#define IGN4_EN_PIN				12
#define IGN1_DET_PIN			7
#define IGN2_DET_PIN			6
#define IGN3_DET_PIN			5
#define IGN4_DET_PIN			4

#define VBAT_MEAS_PIN			8

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
#define LED_POS_RF				(LED_STD_POS0+0)

#define BUZZER_GENERATOR		1
#define BUZZER_POS				BUZZER_POS0
#endif

#ifdef CONFIG_BOARD_KPPTR_VER_1_REV_0
#define SPI_SLAVE_MS5607_PIN	41
#define SPI_SLAVE_LIS331_PIN	36
#define SPI_SLAVE_LSM6DSO32_PIN	35
#define SPI_SLAVE_LSM6DSO32_2_PIN	33
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

#define I2C_SDA_PIN				15
#define I2C_SCL_PIN				16

#define GNSS_RX_PIN				1
#define GNSS_TX_PIN				2
#define GNSS_UART 				UART_NUM_2

#define UART_EXT_OUT			13
#define UART_EXT_IN				14

#define IGN1_EN_PIN				12
#define IGN2_EN_PIN				10
#define IGN3_EN_PIN				11
#define IGN4_EN_PIN				9
#define IGN1_DET_PIN			7
#define IGN2_DET_PIN			6
#define IGN3_DET_PIN			5
#define IGN4_DET_PIN			4

#define VBAT_MEAS_PIN			8

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
#define LED_POS_RF				(LED_WS_POS0+3)

#define BUZZER_GENERATOR		1
#define BUZZER_POS				BUZZER_POS0
#endif

#else
#error "This board version don't exist or require new definition."

#endif
