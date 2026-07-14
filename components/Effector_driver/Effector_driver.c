#include <string.h>
#include <stdbool.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_timer.h"

#include "Servo_driver.h"
#include "SBUS_driver.h"
#include "IGN_driver.h"
#include "Effector_driver.h"

static const char *TAG = "Effector";

static Effector_t    effector_registry[EFFECTOR_MAX_COUNT];
static effector_arm_t arm_igniters;
static effector_arm_t arm_servos;
/*!< Timestamp from the most recent Effector_srv() call. Used by
 *   Effector_activate() as the start reference for timed activation windows,
 *   so it doesn't need its own time source. Defaults to 0 until the first
 *   Effector_srv() call. */
static uint32_t       last_srv_time_now;


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
    last_srv_time_now = 0;
    return ESP_OK;
}

esp_err_t Effector_register(effector_id_t id, effector_type_t type, effector_hw_t hw,
                             int8_t active_value, int8_t inactive_value, bool ignore_arm,
                             uint32_t activation_time_ms){
    ESP_RETURN_ON_FALSE(id < EFFECTOR_MAX_COUNT, ESP_ERR_INVALID_ARG, TAG, "id %d out of range", id);

    if(effector_registry[id].registered){
        ESP_LOGE(TAG, "Effector %d already registered", id);
        return ESP_ERR_INVALID_STATE;
    }

    if(activation_time_ms != EFFECTOR_ACTIVATION_INFINITE){
        ESP_RETURN_ON_FALSE(activation_time_ms >= EFFECTOR_ACTIVATION_MIN_MS &&
                             activation_time_ms <= EFFECTOR_ACTIVATION_MAX_MS,
                             ESP_ERR_INVALID_ARG, TAG,
                             "effector %d activation_time_ms %u out of range [%u, %u]",
                             id, (unsigned)activation_time_ms,
                             (unsigned)EFFECTOR_ACTIVATION_MIN_MS, (unsigned)EFFECTOR_ACTIVATION_MAX_MS);

        ESP_RETURN_ON_FALSE((activation_time_ms % EFFECTOR_ACTIVATION_RESOLUTION_MS) == 0,
                             ESP_ERR_INVALID_ARG, TAG,
                             "effector %d activation_time_ms %u not a multiple of %u ms",
                             id, (unsigned)activation_time_ms, (unsigned)EFFECTOR_ACTIVATION_RESOLUTION_MS);
    }

    effector_registry[id].type               = type;
    effector_registry[id].hw                 = hw;
    effector_registry[id].active_value       = active_value;
    effector_registry[id].inactive_value     = inactive_value;
    effector_registry[id].current_value      = inactive_value;
    effector_registry[id].ignore_arm         = ignore_arm;
    effector_registry[id].activation_time_ms = activation_time_ms;
    effector_registry[id].state              = EFFECTOR_STATE_IDLE;
    effector_registry[id].deadline_ms        = 0;
    effector_registry[id].registered         = true;

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
    /* A direct/manual Effector_set() is not a timed activation: make sure
     * Effector_srv() does not later act on a stale deadline for this effector. */
    entry->state = EFFECTOR_STATE_IDLE;
    return ESP_OK;
}

esp_err_t Effector_activate(effector_id_t id){
    ESP_RETURN_ON_FALSE(id < EFFECTOR_MAX_COUNT, ESP_ERR_INVALID_ARG, TAG, "id %d out of range", id);
    ESP_RETURN_ON_FALSE(effector_registry[id].registered, ESP_ERR_INVALID_ARG, TAG, "effector %d not registered", id);

    Effector_t * entry = &effector_registry[id];

    ESP_RETURN_ON_ERROR(Effector_set(id, entry->active_value), TAG, "effector %d activate failed", id);

    entry->state = EFFECTOR_STATE_ACTIVE;
    if(entry->activation_time_ms != EFFECTOR_ACTIVATION_INFINITE){
        uint32_t time_now = (uint32_t)(esp_timer_get_time() / 1000);
        entry->deadline_ms = time_now + entry->activation_time_ms;
    }

    return ESP_OK;
}

esp_err_t Effector_deactivate(effector_id_t id){
    ESP_RETURN_ON_FALSE(id < EFFECTOR_MAX_COUNT, ESP_ERR_INVALID_ARG, TAG, "id %d out of range", id);
    ESP_RETURN_ON_FALSE(effector_registry[id].registered, ESP_ERR_INVALID_ARG, TAG, "effector %d not registered", id);

    return Effector_set(id, effector_registry[id].inactive_value);
}

void Effector_srv(void){
    uint32_t time_now = (uint32_t)(esp_timer_get_time() / 1000);

    for(effector_id_t id = 0; id < EFFECTOR_MAX_COUNT; id++){
        Effector_t * entry = &effector_registry[id];

        if(!entry->registered){
            continue;
        }
        if(entry->state != EFFECTOR_STATE_ACTIVE){
            continue;
        }
        if(entry->activation_time_ms == EFFECTOR_ACTIVATION_INFINITE){
            continue;
        }

        /* Wraparound-safe "has the deadline passed" check: works correctly
         * across a 32-bit time_now rollover as long as the actual elapsed
         * time never exceeds ~24.8 days, which always holds here since
         * activation_time_ms is capped at EFFECTOR_ACTIVATION_MAX_MS (10 s). */
        if((int32_t)(time_now - entry->deadline_ms) >= 0){
            esp_err_t err = Effector_deactivate(id);
            if(err != ESP_OK){
                ESP_LOGE(TAG, "Effector %d auto-deactivate failed: %s", id, esp_err_to_name(err));
                /* Leave state as ACTIVE; Effector_srv() will retry on the next call. */
            }
        }
    }
}

Effector_t * Effector_getData(effector_id_t id){
    if(id >= EFFECTOR_MAX_COUNT || !effector_registry[id].registered){
        return NULL;
    }
    return &effector_registry[id];
}