#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/mcpwm.h"

typedef struct{
    int min_pulsewidth_us;
    int max_pulsewidth_us;
    int timebase_frequency;
} Servo_config_t;

typedef struct{
    int8_t S1_pos;
    int8_t S2_pos;
    int8_t S3_pos;
    int8_t S4_pos;

    uint8_t servo_en;
} servo_t;

esp_err_t Servo_init(int min_pulsewidth, int max_pulsewidth, int frequency);
esp_err_t Servo_enable();
esp_err_t Servo_disable();
esp_err_t Servo_driveSinglePWM(uint8_t servo_num, int8_t position);
esp_err_t Servo_drive(int8_t S1_position, int8_t S2_position, int8_t S3_position, int8_t S4_position);
esp_err_t Servo_configSingle(uint8_t servo_num, int min_pulsewidth, int max_pulsewidth, int frequency);
servo_t * Servo_get();
