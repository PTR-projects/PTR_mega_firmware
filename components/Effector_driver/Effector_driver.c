#include <string.h>
#include <stdbool.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"

#include "Servo_driver.h"
#include "SBUS_driver.h"
#include "IGN_driver.h"
#include "Effector_driver.h"

static const char *TAG = "Effector";

static Effector_t    effector_registry[EFFECTOR_MAX_COUNT];
static effector_arm_t arm_igniters;
static effector_arm_t arm_servos;


//----- Private helpers ----------

static bool Effector_isServo(effector_type_t type){
    return (type == EFFECTOR_TYPE_SERVO_PWM) || (type == EFFECTOR_TYPE_SERVO_SBUS);
}

static esp_err_t Effector_checkArm(Effector_t * entry, effector_id_t id){
    if(entry->ignore_arm){
        return ESP_OK;
    }

    if(entry->type == EFFECTOR_TYPE_IGNITER && arm_igniters == EFFECTOR_DISARMED){
        ESP_LOGE(TAG, "Effector %d blocked - igniters disarmed", id);
        return ESP_ERR_INVALID_STATE;
    }

    if(Effector_isServo(entry->type) && arm_servos == EFFECTOR_DISARMED){
        ESP_LOGE(TAG, "Effector %d blocked - servos disarmed", id);
        return ESP_ERR_INVALID_STATE;
    }

    return ESP_OK;
}

static esp_err_t Effector_dispatch(Effector_t * entry, int8_t value){
    switch(entry->type){
    case EFFECTOR_TYPE_SERVO_PWM:
        return Servo_driveSinglePWM(entry->hw.servo_pwm.servo_num, value);

    case EFFECTOR_TYPE_SERVO_SBUS:
        return SBUS_setChannel(entry->hw.servo_sbus.channel, value);

    case EFFECTOR_TYPE_IGNITER:
        return IGN_set(entry->hw.igniter.channel, value > 0 ? 1 : 0);

    default:
        ESP_LOGE(TAG, "Unknown effector type %d", entry->type);
        return ESP_ERR_INVALID_ARG;
    }
}


//----- Public API ----------

esp_err_t Effector_init(){
    memset(effector_registry, 0, sizeof(effector_registry));
    arm_igniters = EFFECTOR_DISARMED;
    arm_servos   = EFFECTOR_DISARMED;
    return ESP_OK;
}

esp_err_t Effector_register(effector_id_t id, effector_type_t type, effector_hw_t hw,
                             int8_t active_value, int8_t inactive_value, bool ignore_arm){
    ESP_RETURN_ON_FALSE(id < EFFECTOR_MAX_COUNT, ESP_ERR_INVALID_ARG, TAG, "id %d out of range", id);

    if(effector_registry[id].registered){
        ESP_LOGE(TAG, "Effector %d already registered", id);
        return ESP_ERR_INVALID_STATE;
    }

    effector_registry[id].type           = type;
    effector_registry[id].hw             = hw;
    effector_registry[id].active_value   = active_value;
    effector_registry[id].inactive_value = inactive_value;
    effector_registry[id].current_value  = inactive_value;
    effector_registry[id].ignore_arm     = ignore_arm;
    effector_registry[id].registered     = true;

    return ESP_OK;
}

void Effector_armIgniters(){
    arm_igniters = EFFECTOR_ARMED;
    ESP_LOGI(TAG, "Igniters armed");
}

void Effector_disarmIgniters(){
    arm_igniters = EFFECTOR_DISARMED;
    ESP_LOGI(TAG, "Igniters disarmed");
}

void Effector_armServos(){
    arm_servos = EFFECTOR_ARMED;
    ESP_LOGI(TAG, "Servos armed");
}

void Effector_disarmServos(){
    arm_servos = EFFECTOR_DISARMED;
    ESP_LOGI(TAG, "Servos disarmed");
}

esp_err_t Effector_set(effector_id_t id, int8_t value){
    ESP_RETURN_ON_FALSE(id < EFFECTOR_MAX_COUNT, ESP_ERR_INVALID_ARG, TAG, "id %d out of range", id);
    ESP_RETURN_ON_FALSE(effector_registry[id].registered, ESP_ERR_INVALID_ARG, TAG, "effector %d not registered", id);

    Effector_t * entry = &effector_registry[id];

    ESP_RETURN_ON_ERROR(Effector_checkArm(entry, id), TAG, "effector %d arm check failed", id);
    ESP_RETURN_ON_ERROR(Effector_dispatch(entry, value), TAG, "effector %d dispatch failed", id);

    entry->current_value = value;
    return ESP_OK;
}

esp_err_t Effector_activate(effector_id_t id){
    ESP_RETURN_ON_FALSE(id < EFFECTOR_MAX_COUNT, ESP_ERR_INVALID_ARG, TAG, "id %d out of range", id);
    ESP_RETURN_ON_FALSE(effector_registry[id].registered, ESP_ERR_INVALID_ARG, TAG, "effector %d not registered", id);

    return Effector_set(id, effector_registry[id].active_value);
}

esp_err_t Effector_deactivate(effector_id_t id){
    ESP_RETURN_ON_FALSE(id < EFFECTOR_MAX_COUNT, ESP_ERR_INVALID_ARG, TAG, "id %d out of range", id);
    ESP_RETURN_ON_FALSE(effector_registry[id].registered, ESP_ERR_INVALID_ARG, TAG, "effector %d not registered", id);

    return Effector_set(id, effector_registry[id].inactive_value);
}

Effector_t * Effector_getData(effector_id_t id){
    if(id >= EFFECTOR_MAX_COUNT || !effector_registry[id].registered){
        return NULL;
    }
    return &effector_registry[id];
}
