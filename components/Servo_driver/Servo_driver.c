#include "Servo_driver.h"

#include <stdio.h>

#include "esp_err.h"
#include "esp_log.h"
#include "driver/mcpwm.h"
#include "driver/gpio.h"

#include "BOARD.h"

static const char *TAG = "Servo_driver";

Servo_config_t Servo_config_d;

Servo_t Servo_d;

/*!
 * @brief Calculate pulsewidth at given position
 * @return uint32_t pulsewidth 
 */
uint32_t example_angle_to_compare(float position, Servo_config_t Servo_config)
{
    return (uint32_t)((position - Servo_config.SERVO_MIN_DEGREE) * (float)(Servo_config.SERVO_MAX_PULSEWIDTH_US - Servo_config.SERVO_MIN_PULSEWIDTH_US) / (Servo_config.SERVO_MAX_DEGREE - Servo_config.SERVO_MIN_DEGREE) + (float)Servo_config.SERVO_MIN_PULSEWIDTH_US);
}


/*!
 * @brief Initialize servo component
 * @param min_pulsewidth
 * Minimum servo pulsewidth in microseconds
 * @param max_pulsewidth
 * Maximum servo pulsewidth in microseconds
 * @param frequency
 * Servo operating frequency
 * @param min_pos
 * Lower servo range 
 * @param max_pos
 * Upper servo range
 * @return `ESP_OK` if done
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t Servo_init(int min_pulsewidth, int max_pulsewidth, int frequency, float min_pos, float max_pos){

    Servo_config_d.SERVO_MIN_PULSEWIDTH_US = min_pulsewidth;
    Servo_config_d.SERVO_MAX_PULSEWIDTH_US = max_pulsewidth;
    Servo_config_d.SERVO_MIN_DEGREE = min_pos;
    Servo_config_d.SERVO_MAX_DEGREE = max_pos;
    Servo_config_d.SERVO_TIMEBASE_FREQUENCY = frequency;

   mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, SERVO1_PIN); 
   mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM1A, SERVO2_PIN);
   mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM0B, SERVO3_PIN);
   mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM1B, SERVO4_PIN);

    mcpwm_config_t pwm_config = {
        .frequency = Servo_config_d.SERVO_TIMEBASE_FREQUENCY , 
        .cmpr_a = 0,     
        .counter_mode = MCPWM_UP_COUNTER,
        .duty_mode = MCPWM_DUTY_MODE_0,
    };

    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_1, &pwm_config);
    mcpwm_init(MCPWM_UNIT_1, MCPWM_TIMER_0, &pwm_config);
    mcpwm_init(MCPWM_UNIT_1, MCPWM_TIMER_1, &pwm_config);

    gpio_reset_pin(SERVO_EN_PIN);
    gpio_set_direction(SERVO_EN_PIN, GPIO_MODE_INPUT_OUTPUT);
   

    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, example_angle_to_compare(0.0f, Servo_config_d));
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_A, example_angle_to_compare(0.0f, Servo_config_d));
    mcpwm_set_duty_in_us(MCPWM_UNIT_1, MCPWM_TIMER_0, MCPWM_OPR_B, example_angle_to_compare(0.0f, Servo_config_d));
    mcpwm_set_duty_in_us(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_B, example_angle_to_compare(0.0f, Servo_config_d));

    return ESP_OK;
}

/*!
 * @brief Enable SERVO_EN pin 
 * @return `ESP_OK` if done
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t Servo_enable(){
    gpio_set_level(SERVO_EN_PIN, 1);
      Servo_d.servo_en = 1;

    return ESP_OK;
}

/*!
 * @brief Disable SERVO_EN pin 
 * @return `ESP_OK` if done
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t Servo_disable(){
    gpio_set_level(SERVO_EN_PIN, 0);
    Servo_d.servo_en = 0;

    return ESP_OK;
}

/*!
 * @brief Drive servos to desired position
 * @param position
 * Position value for each servo
 * @return `ESP_OK` if done
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t Servo_drive(float S1_position, float S2_position, float S3_position, float S4_position){
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, example_angle_to_compare(S1_position, Servo_config_d));
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_A, example_angle_to_compare(S2_position, Servo_config_d));
    mcpwm_set_duty_in_us(MCPWM_UNIT_1, MCPWM_TIMER_0, MCPWM_OPR_B, example_angle_to_compare(S3_position, Servo_config_d));
    mcpwm_set_duty_in_us(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_B, example_angle_to_compare(S4_position, Servo_config_d));

    Servo_d.S1_pos = S1_position;
    Servo_d.S2_pos = S2_position;
    Servo_d.S3_pos = S3_position;
    Servo_d.S4_pos = S4_position;


    return ESP_OK;
}

Servo_t * Servo_get(){
    return &Servo_d;
}