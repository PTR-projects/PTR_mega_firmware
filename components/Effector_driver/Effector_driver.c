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

static Effector_t    effector_registry[EFFECTOR_MAX_COUNT][EFFECTOR_BINDINGS_PER_ID];
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

static bool Effector_hasBinding(effector_id_t id){
    for(uint8_t i = 0; i < EFFECTOR_BINDINGS_PER_ID; i++){
        if(effector_registry[id][i].registered){
            return true;
        }
    }
    return false;
}

static Effector_t * Effector_findFree(effector_id_t id){
    for(uint8_t i = 0; i < EFFECTOR_BINDINGS_PER_ID; i++){
        if(!effector_registry[id][i].registered){
            return &effector_registry[id][i];
        }
    }
    return NULL;
}

static esp_err_t Effector_dispatch(Effector_t * entry, int8_t value);

static esp_err_t Effector_requireId(effector_id_t id){
    ESP_RETURN_ON_FALSE(id < EFFECTOR_MAX_COUNT, ESP_ERR_INVALID_ARG, TAG, "id %d out of range", id);
    ESP_RETURN_ON_FALSE(Effector_hasBinding(id), ESP_ERR_INVALID_ARG, TAG, "effector %d not registered", id);
    return ESP_OK;
}

/* Commands one binding and clears any timed-activation window. */
static esp_err_t Effector_commandBinding(effector_id_t id, Effector_t * entry, int8_t value){
    esp_err_t err = Effector_checkArm(entry, id);
    if(err != ESP_OK){
        return err;
    }

    err = Effector_dispatch(entry, value);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "effector %d dispatch failed: %s", id, esp_err_to_name(err));
        return err;
    }

    entry->current_value = value;
    /* A direct command is not a timed activation. Effector_activate() sets
     * ACTIVE again after this returns. */
    entry->state       = EFFECTOR_STATE_IDLE;
    entry->deadline_ms = 0;
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

    Effector_t * entry = Effector_findFree(id);
    if(entry == NULL){
        ESP_LOGE(TAG, "Effector %d binding list full", id);
        return ESP_ERR_NO_MEM;
    }

    entry->type               = type;
    entry->hw                 = hw;
    entry->active_value       = active_value;
    entry->inactive_value     = inactive_value;
    entry->current_value      = inactive_value;
    entry->ignore_arm         = ignore_arm;
    entry->activation_time_ms = activation_time_ms;
    entry->state              = EFFECTOR_STATE_IDLE;
    entry->deadline_ms        = 0;
    entry->registered         = true;

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
    esp_err_t id_err = Effector_requireId(id);
    if(id_err != ESP_OK){
        return id_err;
    }

    esp_err_t first_err = ESP_OK;
    for(uint8_t i = 0; i < EFFECTOR_BINDINGS_PER_ID; i++){
        Effector_t * entry = &effector_registry[id][i];
        if(!entry->registered){
            continue;
        }

        esp_err_t err = Effector_commandBinding(id, entry, value);
        if(err != ESP_OK && first_err == ESP_OK){
            first_err = err;
        }
    }

    return first_err;
}

esp_err_t Effector_activate(effector_id_t id){
    esp_err_t id_err = Effector_requireId(id);
    if(id_err != ESP_OK){
        return id_err;
    }

    uint32_t time_now = (uint32_t)(esp_timer_get_time() / 1000);
    esp_err_t first_err = ESP_OK;

    for(uint8_t i = 0; i < EFFECTOR_BINDINGS_PER_ID; i++){
        Effector_t * entry = &effector_registry[id][i];
        if(!entry->registered){
            continue;
        }

        esp_err_t err = Effector_commandBinding(id, entry, entry->active_value);
        if(err != ESP_OK){
            if(first_err == ESP_OK){
                first_err = err;
            }
            continue;
        }

        entry->state = EFFECTOR_STATE_ACTIVE;
        if(entry->activation_time_ms != EFFECTOR_ACTIVATION_INFINITE){
            entry->deadline_ms = time_now + entry->activation_time_ms;
        }
    }

    return first_err;
}

esp_err_t Effector_deactivate(effector_id_t id){
    esp_err_t id_err = Effector_requireId(id);
    if(id_err != ESP_OK){
        return id_err;
    }

    esp_err_t first_err = ESP_OK;
    for(uint8_t i = 0; i < EFFECTOR_BINDINGS_PER_ID; i++){
        Effector_t * entry = &effector_registry[id][i];
        if(!entry->registered){
            continue;
        }

        esp_err_t err = Effector_commandBinding(id, entry, entry->inactive_value);
        if(err != ESP_OK && first_err == ESP_OK){
            first_err = err;
        }
    }

    return first_err;
}

void Effector_srv(void){
    uint32_t time_now = (uint32_t)(esp_timer_get_time() / 1000);

    for(effector_id_t id = 0; id < EFFECTOR_MAX_COUNT; id++){
        for(uint8_t i = 0; i < EFFECTOR_BINDINGS_PER_ID; i++){
            Effector_t * entry = &effector_registry[id][i];

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
                esp_err_t err = Effector_commandBinding(id, entry, entry->inactive_value);
                if(err != ESP_OK){
                    ESP_LOGE(TAG, "Effector %d binding %u auto-deactivate failed: %s",
                             id, i, esp_err_to_name(err));
                    /* commandBinding leaves state unchanged on failure, so this
                     * binding stays ACTIVE and Effector_srv() retries next call.
                     * Other bindings of this id are not touched. */
                    entry->state = EFFECTOR_STATE_ACTIVE;
                }
            }
        }
    }
}

Effector_t * Effector_getData(effector_id_t id){
    if(id >= EFFECTOR_MAX_COUNT){
        return NULL;
    }

    for(uint8_t i = 0; i < EFFECTOR_BINDINGS_PER_ID; i++){
        if(effector_registry[id][i].registered){
            return &effector_registry[id][i];
        }
    }
    return NULL;
}