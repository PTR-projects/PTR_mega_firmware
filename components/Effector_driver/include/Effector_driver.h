#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "effector_ids.h"

#define EFFECTOR_MAX_COUNT 16

/*!< Timed-activation resolution, in ms. Non-zero activation_time_ms values
 *   passed to Effector_register() must be a multiple of this. */
#define EFFECTOR_ACTIVATION_RESOLUTION_MS  10U
/*!< Minimum non-zero activation time, in ms. */
#define EFFECTOR_ACTIVATION_MIN_MS         10U
/*!< Maximum activation time, in ms. */
#define EFFECTOR_ACTIVATION_MAX_MS         10000U
/*!< activation_time_ms value meaning "stay active until explicitly deactivated". */
#define EFFECTOR_ACTIVATION_INFINITE       0U

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
 * @brief Timed-activation state of a single effector.
 */
typedef enum {
    EFFECTOR_STATE_IDLE,    /*!< Not running a timed activation window. */
    EFFECTOR_STATE_ACTIVE,  /*!< Activated; if activation_time_ms != 0, will
                              *  auto-deactivate once Effector_srv() observes
                              *  the deadline has passed. */
} effector_state_t;

/**
 * @brief Single effector registry entry.
 */
typedef struct {
    effector_type_t   type;
    effector_hw_t      hw;
    int8_t             active_value;       /*!< Value used by Effector_activate(). */
    int8_t             inactive_value;     /*!< Value used by Effector_deactivate(). */
    int8_t             current_value;      /*!< Last successfully commanded value. */
    bool               ignore_arm;         /*!< If true, arming checks are skipped for this effector. */
    bool               registered;         /*!< Slot is occupied. */
    uint32_t           activation_time_ms; /*!< 0 = stay active indefinitely. Otherwise
                                              *  10..10000, resolution 10 ms: minimum time
                                              *  Effector_activate() keeps this effector
                                              *  active before Effector_srv() is allowed to
                                              *  auto-deactivate it. This is a floor, not an
                                              *  exact duration — actual on-time is extended
                                              *  by however late the next Effector_srv() call
                                              *  lands after the deadline (see Effector_srv()). */
    effector_state_t   state;              /*!< Current timed-activation state. */
    uint32_t           deadline_ms;        /*!< Timestamp, in the same ms-since-boot
                                              *  units as esp_timer_get_time()/1000, at
                                              *  which this effector should auto-deactivate.
                                              *  Set by Effector_activate() and checked by
                                              *  Effector_srv(). Only meaningful while
                                              *  state == EFFECTOR_STATE_ACTIVE and
                                              *  activation_time_ms != 0. */
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
 * @param activation_time_ms  How long Effector_activate() should hold this effector
 *                             active before Effector_srv() auto-deactivates it.
 *                             EFFECTOR_ACTIVATION_INFINITE (0) disables auto-deactivation.
 *                             Otherwise must be in [EFFECTOR_ACTIVATION_MIN_MS,
 *                             EFFECTOR_ACTIVATION_MAX_MS] and a multiple of
 *                             EFFECTOR_ACTIVATION_RESOLUTION_MS (10 ms…10 s, 10 ms steps).
 * @return ESP_OK, ESP_ERR_INVALID_ARG if id is out of range or activation_time_ms is
 *         outside the allowed range/resolution, ESP_ERR_INVALID_STATE if the slot is
 *         already registered.
 */
esp_err_t Effector_register(effector_id_t id, effector_type_t type, effector_hw_t hw,
                             int8_t active_value, int8_t inactive_value, bool ignore_arm,
                             uint32_t activation_time_ms);

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
 *        This is a direct/manual override: it does not start or extend a timed
 *        activation window, and always leaves the effector in EFFECTOR_STATE_IDLE
 *        (i.e. Effector_srv() will not act on it afterwards). Use Effector_activate()
 *        if you want the registered activation_time_ms timer to apply.
 * @return ESP_OK, ESP_ERR_INVALID_ARG, or ESP_ERR_INVALID_STATE if blocked by arming.
 */
esp_err_t Effector_set(effector_id_t id, int8_t value);

/**
 * @brief Commands an effector to its registered active_value and, if this effector
 *        was registered with a non-zero activation_time_ms, starts its timed
 *        activation window: the deadline is computed from the current time,
 *        read internally via esp_timer_get_time()/1000. Effector_srv() will
 *        automatically deactivate this effector once that deadline has passed.
 *        If activation_time_ms is 0, the effector stays active until
 *        Effector_deactivate()/Effector_set() is called explicitly.
 * @param id  Effector to activate.
 */
esp_err_t Effector_activate(effector_id_t id);

/**
 * @brief Commands an effector to its registered inactive_value and clears any
 *        pending timed activation window (state becomes EFFECTOR_STATE_IDLE).
 */
esp_err_t Effector_deactivate(effector_id_t id);

/**
 * @brief Periodic service. Call regularly — expected call period is 10..50 ms
 *        in this project. Reads the current time internally via
 *        esp_timer_get_time()/1000. For every effector in EFFECTOR_STATE_ACTIVE
 *        with a non-zero activation_time_ms whose deadline has passed,
 *        automatically calls Effector_deactivate() on it.
 *
 *        activation_time_ms is a floor, not an exact duration: because the
 *        deadline is only checked once per call, an effector may stay active
 *        up to (call period - 1) ms longer than requested if its deadline
 *        falls between two calls. With a 10..50 ms call period, worst-case
 *        overrun is ~50 ms — keep that in mind for activation_time_ms values
 *        close to the 10 ms minimum. The effector is never deactivated early.
 */
void Effector_srv(void);

/**
 * @brief Returns a pointer to the registry entry for inspection.
 * @return Pointer to Effector_t, or NULL if id is out of range or unregistered.
 */
Effector_t * Effector_getData(effector_id_t id);