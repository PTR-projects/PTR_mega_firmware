#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "effector_ids.h"

#define EFFECTOR_MAX_COUNT 16

/**
 * @brief Physical backend type for an effector.
 */
typedef enum {
    EFFECTOR_TYPE_SERVO_PWM,    /*!< PWM servo via Servo_driver. */
    EFFECTOR_TYPE_SERVO_SBUS,   /*!< SBUS servo via SBUS_driver. */
    EFFECTOR_TYPE_IGNITER,      /*!< Igniter via IGN_driver. */
} effector_type_t;

/**
 * @brief Hardware configuration. Only the union member matching the type is used.
 */
typedef union {
    struct { uint8_t servo_num; } servo_pwm;   /*!< 1-based servo number for Servo_driver. */
    struct { uint8_t channel;   } servo_sbus;  /*!< SBUS channel index (0–15). */
    struct { uint8_t channel;   } igniter;     /*!< Igniter channel (APO/MAIN/SECOND_STAGE/AUX). */
} effector_hw_t;

/**
 * @brief Arming status for a group of effectors.
 */
typedef enum {
    EFFECTOR_DISARMED,
    EFFECTOR_ARMED,
} effector_arm_t;

/**
 * @brief Single effector registry entry.
 */
typedef struct {
    effector_type_t type;
    effector_hw_t   hw;
    int8_t          active_value;    /*!< Value used by Effector_activate(). */
    int8_t          inactive_value;  /*!< Value used by Effector_deactivate(). */
    int8_t          current_value;   /*!< Last successfully commanded value. */
    bool            ignore_arm;      /*!< If true, arming checks are skipped for this effector. */
    bool            registered;      /*!< Slot is occupied. */
} Effector_t;

/**
 * @brief Initialises the effector registry and sets both arm flags to DISARMED.
 */
esp_err_t Effector_init();

/**
 * @brief Registers a logical effector.
 * @param id            Logical ID from effector_ids.h.
 * @param type          Physical backend type.
 * @param hw            Hardware configuration for the chosen backend.
 * @param active_value  Position/value commanded by Effector_activate() (-100…+100).
 * @param inactive_value Position/value commanded by Effector_deactivate() (-100…+100).
 * @param ignore_arm    If true, this effector ignores the servo/igniter arming flags.
 * @return ESP_OK, ESP_ERR_INVALID_ARG if id is out of range,
 *         ESP_ERR_INVALID_STATE if the slot is already registered.
 */
esp_err_t Effector_register(effector_id_t id, effector_type_t type, effector_hw_t hw,
                             int8_t active_value, int8_t inactive_value, bool ignore_arm);

/**
 * @brief Arms all igniter-type effectors.
 */
void Effector_armIgniters();

/**
 * @brief Disarms all igniter-type effectors.
 */
void Effector_disarmIgniters();

/**
 * @brief Arms all servo-type effectors (PWM and SBUS).
 */
void Effector_armServos();

/**
 * @brief Disarms all servo-type effectors (PWM and SBUS).
 */
void Effector_disarmServos();

/**
 * @brief Commands an effector to an arbitrary value (-100…+100).
 *        Respects the arming flags unless ignore_arm is set for this effector.
 * @return ESP_OK, ESP_ERR_INVALID_ARG, or ESP_ERR_INVALID_STATE if blocked by arming.
 */
esp_err_t Effector_set(effector_id_t id, int8_t value);

/**
 * @brief Commands an effector to its registered active_value.
 */
esp_err_t Effector_activate(effector_id_t id);

/**
 * @brief Commands an effector to its registered inactive_value.
 */
esp_err_t Effector_deactivate(effector_id_t id);

/**
 * @brief Returns a pointer to the registry entry for inspection.
 * @return Pointer to Effector_t, or NULL if id is out of range or unregistered.
 */
Effector_t * Effector_getData(effector_id_t id);
