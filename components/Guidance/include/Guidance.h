#pragma once

#include "esp_err.h"
#include "AHRS_driver.h"
#include "Sensors.h"

/**
 * @file Guidance.h
 * @brief Active fin-steering guidance.
 *
 * Pipeline (run once per control cycle via Guidance_step):
 *   1. error_earth = velocity_ENU - demand          (earth-frame velocity error)
 *   2. error_body  = R_earth->body * error_earth     (rotate into rocket frame)
 *   3. pitch/yaw errors -> two independent PID loops
 *   4. PID output / dynamic pressure                 (gain scheduling, guarded /0)
 *   5. saturate to +-max fin angle
 *   6. command pitch/yaw servos (Effector_set)
 *
 * Only actuates while FSD_isSteeringEnabled() is true (burnout..apogee). Servos
 * must be armed (or registered with ignore_arm).
 */

/**
 * @brief Initialise the pitch and yaw PID controllers and clear the demand.
 * @param kp,ki,kd  Gains applied to BOTH axes (call the PID setters directly if
 *                  you want per-axis gains).
 */
esp_err_t Guidance_init(float kp, float ki, float kd);

/**
 * @brief Set the earth-frame (ENU) velocity demand/setpoint [m/s].
 *        Default is {0,0,0} = null all velocity error (fly along the velocity
 *        vector). Typically leave horizontal 0 to null lateral drift.
 */
void Guidance_setDemand(float vx, float vy, float vz);

/**
 * @brief Run one guidance/control step. Call from the main loop right after
 *        AHRS_compute() and FSD_detect().
 * @param ahrs     Current AHRS data (orientation + vel_enu).
 * @param sensors  Current sensor data (pressure/temperature for air density).
 */
void Guidance_step(AHRS_t *ahrs, Sensors_t *sensors);
