#include "Servo_driver.h"

#include <stdio.h>

#include "esp_err.h"
#include "esp_log.h"
#include "driver/mcpwm.h"
#include "driver/gpio.h"
#include "BOARD.h"

static const char *TAG = "Servo_driver";
servo_t Servo_d;

int CH_TO_UNIT[6]   = {MCPWM_UNIT_0, MCPWM_UNIT_0, MCPWM_UNIT_0, MCPWM_UNIT_1, MCPWM_UNIT_1, MCPWM_UNIT_1};
int CH_TO_SIGNAL[6] = {MCPWM0A, MCPWM1A, MCPWM2A, MCPWM0A, MCPWM1A, MCPWM2A};
int CH_TO_TIMER[6]  = {MCPWM_TIMER_0, MCPWM_TIMER_1, MCPWM_TIMER_2,MCPWM_TIMER_0, MCPWM_TIMER_1, MCPWM_TIMER_2};

#if BOARD_SERVO_PWM_NUM > 6
#error "To many PWM signals required. Max MCPWM PWM channels = 6"
#endif

#if !((BOARD_SERVO_PWM_NUM > 0) && !(BOARD_SERVO_SBUS_NUM > 0))
esp_err_t Servo_init(int min_pulsewidth, int max_pulsewidth, int frequency) { return ESP_FAIL; }
esp_err_t Servo_enable() { return ESP_FAIL; }
esp_err_t Servo_disable() { return ESP_FAIL; }
esp_err_t Servo_drive(int8_t S1_position, int8_t S2_position, int8_t S3_position, int8_t S4_position) { return ESP_FAIL; }
servo_t * Servo_get() { return &Servo_d; }

#else
int SERVO_PINS[BOARD_SERVO_PWM_NUM] = BOARD_SERVO_PWM_PINS;
Servo_config_t Servo_config_d[BOARD_SERVO_PWM_NUM];
static uint32_t angle_to_PWM(float position, Servo_config_t Servo_config);

/*!
 * @brief Initialize servo component
 * @param min_pulsewidth
 * Minimum servo pulsewidth in microseconds
 * @param max_pulsewidth
 * Maximum servo pulsewidth in microseconds
 * @param frequency
 * Servo operating frequency
 * @return `ESP_OK` if done
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t Servo_init(int min_pulsewidth, int max_pulsewidth, int frequency){
#if (BOARD_SERVO_PWM_NUM > 0)

	for(int i=0;i<BOARD_SERVO_PWM_NUM;i++){
		Servo_config_d[i].min_pulsewidth_us  = min_pulsewidth;
		Servo_config_d[i].max_pulsewidth_us  = max_pulsewidth;
		Servo_config_d[i].timebase_frequency
 = frequency;
	}

#if defined SERVO_EN_PIN
    gpio_reset_pin		(SERVO_EN_PIN);
    gpio_set_direction	(SERVO_EN_PIN, GPIO_MODE_INPUT_OUTPUT);
#endif

	for(int i=0;i<BOARD_SERVO_PWM_NUM;i++){
		mcpwm_config_t pwm_config = {
			.frequency 		= Servo_config_d[i].timebase_frequency,
			.cmpr_a 		= 0,
			.counter_mode 	= MCPWM_UP_COUNTER,
			.duty_mode 		= MCPWM_DUTY_MODE_0,
		};

		mcpwm_gpio_init		(CH_TO_UNIT[i], CH_TO_SIGNAL[i], SERVO_PINS[i]);
		mcpwm_init			(CH_TO_UNIT[i], CH_TO_TIMER[i],  &pwm_config);
		mcpwm_set_duty_in_us(CH_TO_UNIT[i], CH_TO_TIMER[i],  MCPWM_GEN_A, angle_to_PWM(0, Servo_config_d[i]));
	}
#endif

#if (BOARD_SERVO_SBUS_NUM > 0)

#endif

    return ESP_OK;
}

/*!
 * @brief Enable SERVO_EN pin 
 * @return `ESP_OK` if done
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t Servo_enable(){
#if defined SERVO_EN_PIN
    gpio_set_level(SERVO_EN_PIN, 1);
	Servo_d.servo_en = 1;

    return ESP_OK;
#else
    return ESP_FAIL;
#endif
}

/*!
 * @brief Disable SERVO_EN pin 
 * @return `ESP_OK` if done
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t Servo_disable(){
#if defined SERVO_EN_PIN
    gpio_set_level(SERVO_EN_PIN, 0);
    Servo_d.servo_en = 0;

    return ESP_OK;
#else
    return ESP_FAIL;
#endif
}

esp_err_t Servo_driveSinglePWM(uint8_t servo_num, int8_t position){
#if (BOARD_SERVO_PWM_NUM > 0)
	if(servo_num > BOARD_SERVO_PWM_NUM){
		return ESP_FAIL;
	}

	return mcpwm_set_duty_in_us(CH_TO_UNIT[servo_num-1], CH_TO_TIMER[servo_num-1], MCPWM_GEN_A,
			                                angle_to_PWM(position, Servo_config_d[servo_num-1]));
#endif
	return ESP_FAIL;
}

/*!
 * @brief Drive servos to desired position
 * @param position
 * Position value for each servo
 * @return `ESP_OK` if done
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t Servo_drive(int8_t S1_position, int8_t S2_position, int8_t S3_position, int8_t S4_position){
	Servo_driveSinglePWM(1, S1_position);
	Servo_driveSinglePWM(2, S2_position);
	Servo_driveSinglePWM(3, S3_position);
	Servo_driveSinglePWM(4, S4_position);

    Servo_d.S1_pos = S1_position;
    Servo_d.S2_pos = S2_position;
    Servo_d.S3_pos = S3_position;
    Servo_d.S4_pos = S4_position;

    return ESP_OK;
}

esp_err_t Servo_configSingle(uint8_t servo_num, int min_pulsewidth, int max_pulsewidth, int frequency){
#if (BOARD_SERVO_PWM_NUM > 0)

	if(servo_num > BOARD_SERVO_PWM_NUM){
		return ESP_FAIL;
	}

	Servo_config_d[servo_num].min_pulsewidth_us  = min_pulsewidth;
	Servo_config_d[servo_num].max_pulsewidth_us  = max_pulsewidth;
	Servo_config_d[servo_num].timebase_frequency = frequency;
	return ESP_OK;

#endif
	return ESP_FAIL;
}

servo_t * Servo_get(){
    return &Servo_d;
}

/*!
 * @brief Calculate pulsewidth at given position
 * @return uint32_t pulsewidth
 */
static uint32_t angle_to_PWM(float position, Servo_config_t Servo_config) {
    return (uint32_t)((position + 100) * (Servo_config.max_pulsewidth_us - Servo_config.min_pulsewidth_us) / 200 + Servo_config.min_pulsewidth_us);
}

#endif
