#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/mcpwm.h"

typedef struct{
    int SERVO_MIN_PULSEWIDTH_US;
    int SERVO_MAX_PULSEWIDTH_US;

    float SERVO_MIN_DEGREE;
    float SERVO_MAX_DEGREE;

    int SERVO_TIMEBASE_FREQUENCY;

}Servo_config_t;


typedef struct{
    float S1_pos;
    float S2_pos;
    float S3_pos;
    float S4_pos;

    uint8_t servo_en;
}Servo_t;

esp_err_t Servo_init(int min_pulsewidth, int max_pulsewidth, int frequency, float min_pos, float max_pos);
esp_err_t Servo_enable();
esp_err_t Servo_disable();
esp_err_t Servo_drive(float S1_position, float S2_position, float S3_position, float S4_position);
Servo_t * Servo_get();