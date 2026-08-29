/*
 * Guidance.c - active fin steering by earth-frame velocity error.
 * See Guidance.h for the pipeline.
 */
#include <math.h>
#include <stdbool.h>

#include "Guidance.h"

#include "PID.h"
#include "algo_utils.h"
#include "quaternion.h"
#include "common.h"
#include "FlightStateDetector.h"
#include "Effector_driver.h"
#include "effector_ids.h"

/* ---- Tunables (adjust for your airframe / servos) ---- */
#define GUID_MAX_FIN_DEG    10.0f    /* saturation: max fin deflection [deg] */
#define GUID_Q_FLOOR_PA     50.0f    /* dynamic-pressure floor for the guarded divide [Pa] */
#define GUID_PID_OUT_LIMIT  5000.0f  /* PID clamp BEFORE /q (real limit is the deg saturation) */
#define GUID_DERIV_TAU      0.02f    /* derivative low-pass time constant [s] */

/* ---- Servo command mapping ----
 * Setpoint +-100 spans the servo's full +-SERVO_TRAVEL_DEG travel, so 1 deg =
 * SERVO_UNITS_PER_DEG setpoint units (=> 0.5 deg resolution). The linkage is only
 * safe within +-GUID_MAX_FIN_DEG of travel (enforced by the saturation). The servo
 * has a SERVO_OFFSET_DEG mechanical offset; SERVO_NEUTRAL_CMD is the setpoint that
 * yields 0 deg of fin. */
#define SERVO_TRAVEL_DEG     50.0f
#define SERVO_UNITS_PER_DEG  (100.0f / SERVO_TRAVEL_DEG)                /* 2.0 units/deg */
#define SERVO_OFFSET_DEG     5.0f
#define SERVO_OFFSET_UNITS   (SERVO_OFFSET_DEG * SERVO_UNITS_PER_DEG)   /* 10 units */
#define SERVO_NEUTRAL_CMD    ((int8_t)(-SERVO_OFFSET_UNITS))            /* -10 => 0 deg fin */

/* Two independent controllers - one per body lateral axis. */
static PID_t     pid_pitch;
static PID_t     pid_yaw;
static vectorf_t demand;             /* earth-frame (ENU) velocity setpoint [m/s] */

esp_err_t Guidance_init(float kp, float ki, float kd)
{
    PID_init(&pid_pitch, kp, ki, kd, -GUID_PID_OUT_LIMIT, GUID_PID_OUT_LIMIT);
    PID_init(&pid_yaw,   kp, ki, kd, -GUID_PID_OUT_LIMIT, GUID_PID_OUT_LIMIT);
    PID_setDerivativeFilter(&pid_pitch, GUID_DERIV_TAU);
    PID_setDerivativeFilter(&pid_yaw,   GUID_DERIV_TAU);

    demand.x = 0.0f;
    demand.y = 0.0f;
    demand.z = 0.0f;
    return ESP_OK;
}

void Guidance_setDemand(float vx, float vy, float vz)
{
    demand.x = vx;
    demand.y = vy;
    demand.z = vz;
}

void Guidance_step(AHRS_t *ahrs, Sensors_t *sensors)
{
    static bool prev_enabled = false;
    const bool  enabled = FSD_isSteeringEnabled();

    if (!enabled) {
        if (prev_enabled) {                 /* just disabled -> fins to neutral (0 deg) */
            Effector_set(EFFECTOR_PITCH, SERVO_NEUTRAL_CMD);
            Effector_set(EFFECTOR_YAW,   SERVO_NEUTRAL_CMD);
        }
        prev_enabled = false;
        return;
    }
    if (!prev_enabled) {                     /* just enabled (burnout) -> clear windup */
        PID_reset(&pid_pitch);
        PID_reset(&pid_yaw);
    }
    prev_enabled = true;

    const float dt = ahrs->dt;

    /* 1) Earth-frame velocity error = velocity - demand */
    vectorf_t err_earth;
    err_earth.x = ahrs->vel_enu.x - demand.x;
    err_earth.y = ahrs->vel_enu.y - demand.y;
    err_earth.z = ahrs->vel_enu.z - demand.z;

    /* 2) Rotate the earth-frame error into the rocket/body frame.
     *    quaternionRotateVector() is earth->body (inverse of the body->earth
     *    rotation AHRS uses for acc_enu). Body z is the rocket long axis, so the
     *    lateral x/y components are the pitch/yaw errors. */
    vectorf_t err_body;
    quaternionRotateVector(&err_body, &err_earth, &(ahrs->orientation.quaternions));

    const float err_pitch = err_body.x;   /* <-- swap axes / flip sign to match your fins */
    const float err_yaw   = err_body.y;

    /* 3) Two independent PID loops */
    const float u_pitch = PID_updateError(&pid_pitch, err_pitch, dt);
    const float u_yaw   = PID_updateError(&pid_yaw,   err_yaw,   dt);

    /* 4) Divide by dynamic pressure (guarded against /0 and low-q blow-up) */
    const float speed = sqrtf(ahrs->vel_enu.x * ahrs->vel_enu.x +
                              ahrs->vel_enu.y * ahrs->vel_enu.y +
                              ahrs->vel_enu.z * ahrs->vel_enu.z);
    const float rho = Algo_airDensity(sensors->MS5607.press, sensors->MS5607.temp);
    const float q   = Algo_dynamicPressure(rho, speed);

    float defl_pitch = Algo_safeDivide(u_pitch, q, GUID_Q_FLOOR_PA);
    float defl_yaw   = Algo_safeDivide(u_yaw,   q, GUID_Q_FLOOR_PA);

    /* 5) Saturate to the max fin angle */
    defl_pitch = Algo_saturate(defl_pitch, -GUID_MAX_FIN_DEG, GUID_MAX_FIN_DEG);
    defl_yaw   = Algo_saturate(defl_yaw,   -GUID_MAX_FIN_DEG, GUID_MAX_FIN_DEG);

    /* 6) Hardware boundary ONLY: convert the fin-angle command [deg] to a servo setpoint.
     *    The control law works entirely in fin degrees and is already limited to
     *    +-GUID_MAX_FIN_DEG above, so there is no saturation here - just the conversion:
     *    setpoint = fin_deg * SERVO_UNITS_PER_DEG - SERVO_OFFSET_UNITS. */
    const int8_t cmd_pitch = (int8_t)(defl_pitch * SERVO_UNITS_PER_DEG - SERVO_OFFSET_UNITS);
    const int8_t cmd_yaw   = (int8_t)(defl_yaw   * SERVO_UNITS_PER_DEG - SERVO_OFFSET_UNITS);

    Effector_set(EFFECTOR_PITCH, cmd_pitch);
    Effector_set(EFFECTOR_YAW,   cmd_yaw);
}
