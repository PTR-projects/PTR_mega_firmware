/*
 * PID.c - generic single-axis PID controller.
 * See PID.h for the contract.
 */
#include "PID.h"

void PID_init(PID_t *pid, float kp, float ki, float kd, float out_min, float out_max)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;

    pid->out_min = out_min;
    pid->out_max = out_max;

    /* Default anti-windup clamp = output clamp. */
    pid->integ_min = out_min;
    pid->integ_max = out_max;

    pid->tau = 0.0f;

    PID_reset(pid);
}

void PID_setIntegratorLimits(PID_t *pid, float integ_min, float integ_max)
{
    pid->integ_min = integ_min;
    pid->integ_max = integ_max;
}

void PID_setDerivativeFilter(PID_t *pid, float tau)
{
    pid->tau = (tau > 0.0f) ? tau : 0.0f;
}

void PID_reset(PID_t *pid)
{
    pid->integrator       = 0.0f;
    pid->prev_measurement = 0.0f;
    pid->prev_derivative  = 0.0f;
    pid->initialized      = false;
}

float PID_update(PID_t *pid, float setpoint, float measurement, float dt)
{
    if (dt <= 0.0f) {
        return 0.0f;   /* invalid timestep - do not integrate/differentiate */
    }

    const float error = setpoint - measurement;

    /* ---- Proportional ---- */
    const float P = pid->kp * error;

    /* ---- Integral (accumulated in output units) with clamping anti-windup ---- */
    pid->integrator += pid->ki * error * dt;
    if (pid->integrator > pid->integ_max) pid->integrator = pid->integ_max;
    if (pid->integrator < pid->integ_min) pid->integrator = pid->integ_min;

    /* ---- Derivative on measurement (no setpoint-change kick) ---- */
    float derivative = 0.0f;
    if (pid->initialized) {
        /* For a constant setpoint, d(error)/dt = -d(measurement)/dt. */
        const float raw = -(measurement - pid->prev_measurement) / dt;
        if (pid->tau > 0.0f) {
            /* First-order low-pass: alpha = dt / (tau + dt). */
            const float alpha = dt / (pid->tau + dt);
            derivative = pid->prev_derivative + alpha * (raw - pid->prev_derivative);
        } else {
            derivative = raw;
        }
    } else {
        /* First sample after a reset: no derivative yet, just seed history. */
        pid->initialized = true;
    }
    pid->prev_measurement = measurement;
    pid->prev_derivative  = derivative;
    const float D = pid->kd * derivative;

    /* ---- Sum and clamp total output ---- */
    float out = P + pid->integrator + D;
    if (out > pid->out_max) out = pid->out_max;
    if (out < pid->out_min) out = pid->out_min;

    return out;
}

float PID_updateError(PID_t *pid, float error, float dt)
{
    if (dt <= 0.0f) {
        return 0.0f;
    }

    /* ---- Proportional ---- */
    const float P = pid->kp * error;

    /* ---- Integral (output units) with clamping anti-windup ---- */
    pid->integrator += pid->ki * error * dt;
    if (pid->integrator > pid->integ_max) pid->integrator = pid->integ_max;
    if (pid->integrator < pid->integ_min) pid->integrator = pid->integ_min;

    /* ---- Derivative on the error signal (prev_measurement reused as prev_error) ---- */
    float derivative = 0.0f;
    if (pid->initialized) {
        const float raw = (error - pid->prev_measurement) / dt;
        if (pid->tau > 0.0f) {
            const float alpha = dt / (pid->tau + dt);
            derivative = pid->prev_derivative + alpha * (raw - pid->prev_derivative);
        } else {
            derivative = raw;
        }
    } else {
        pid->initialized = true;
    }
    pid->prev_measurement = error;
    pid->prev_derivative  = derivative;
    const float D = pid->kd * derivative;

    /* ---- Sum and clamp total output ---- */
    float out = P + pid->integrator + D;
    if (out > pid->out_max) out = pid->out_max;
    if (out < pid->out_min) out = pid->out_min;

    return out;
}
