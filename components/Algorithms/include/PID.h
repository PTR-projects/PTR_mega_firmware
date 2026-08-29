#pragma once

#include <stdbool.h>

/**
 * @file PID.h
 * @brief Generic single-axis PID controller.
 *
 * Hardware-agnostic: no dependency on sensors, AHRS, servos or FreeRTOS.
 * It is pure math over floats, so it can be unit-tested on a host and reused
 * for any control loop. Create one PID_t instance per controlled axis
 * (e.g. one for pitch, one for yaw).
 *
 * Features:
 *  - Derivative-on-measurement (no derivative "kick" when the setpoint changes).
 *  - Optional first-order low-pass on the derivative term (noise rejection).
 *  - Clamped-integrator anti-windup + hard output clamp.
 *  - Explicit dt per call (the caller owns the timebase).
 *
 * The controller works in whatever units you clamp the output to. For servo
 * steering, clamp to the position range you pass to Effector_set()/Servo
 * (i.e. -100..+100), so PID_update() returns a ready-to-command servo value.
 */
typedef struct {
    /* ---- Tunables (set via PID_init / helpers) ---- */
    float kp;           /*!< Proportional gain. */
    float ki;           /*!< Integral gain. */
    float kd;           /*!< Derivative gain. */

    float out_min;      /*!< Lower output clamp (output units, e.g. -100). */
    float out_max;      /*!< Upper output clamp (output units, e.g. +100). */

    float integ_min;    /*!< Integrator anti-windup lower clamp (output units). */
    float integ_max;    /*!< Integrator anti-windup upper clamp (output units). */

    float tau;          /*!< Derivative low-pass time constant [s]. 0 = disabled. */

    /* ---- Runtime state (managed internally; use PID_reset) ---- */
    float integrator;       /*!< Accumulated integral term, already in output units. */
    float prev_measurement; /*!< Previous measurement, for derivative-on-measurement. */
    float prev_derivative;  /*!< Previous (filtered) derivative, for the LPF. */
    bool  initialized;      /*!< False until the first PID_update() after a reset. */
} PID_t;

/**
 * @brief Initialise gains and output limits and clear all runtime state.
 *
 * The integrator anti-windup clamp defaults to the output clamp, which is a
 * sane starting point. Override it afterwards with PID_setIntegratorLimits()
 * if you want the integral term bounded more tightly than the total output.
 *
 * @param pid      Controller instance.
 * @param kp       Proportional gain.
 * @param ki       Integral gain.
 * @param kd       Derivative gain.
 * @param out_min  Lower output clamp (output units).
 * @param out_max  Upper output clamp (output units).
 */
void PID_init(PID_t *pid, float kp, float ki, float kd, float out_min, float out_max);

/**
 * @brief Set the integrator anti-windup clamp (output units). Optional.
 */
void PID_setIntegratorLimits(PID_t *pid, float integ_min, float integ_max);

/**
 * @brief Set the derivative low-pass time constant [s] (0 disables). Optional.
 */
void PID_setDerivativeFilter(PID_t *pid, float tau);

/**
 * @brief Clear the integrator and derivative history.
 *
 * Call this when (re)entering the control regime — e.g. exactly when the rocket
 * transitions into the steering flight state at launch — so the integrator does
 * not carry windup accumulated while the vehicle sat on the pad.
 */
void PID_reset(PID_t *pid);

/**
 * @brief Run one PID iteration.
 *
 * @param pid          Controller instance.
 * @param setpoint     Desired value (e.g. target angle = 0 for "fly straight").
 * @param measurement  Measured value (e.g. current angle).
 * @param dt           Time since the previous call [s]. Must be > 0; a value
 *                     <= 0 is treated as an invalid step and returns 0 without
 *                     mutating state.
 * @return Control output, clamped to [out_min, out_max].
 */
float PID_update(PID_t *pid, float setpoint, float measurement, float dt);

/**
 * @brief Run one PID iteration on a PRE-COMPUTED error.
 *
 * Use this when you already have the error signal (e.g. a body-frame velocity
 * error) instead of a setpoint/measurement pair. The derivative is taken on the
 * error. This shares the same runtime state as PID_update(), so use one form or
 * the other consistently for a given instance.
 *
 * @param pid    Controller instance.
 * @param error  Pre-computed error term.
 * @param dt     Time since the previous call [s]. Must be > 0 (else returns 0).
 * @return Control output, clamped to [out_min, out_max].
 */
float PID_updateError(PID_t *pid, float error, float dt);
