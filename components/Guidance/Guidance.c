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
#define GUID_MAX_FIN_DEG    7.0f    /* saturation: max fin deflection [deg] */
#define GUID_Q_FLOOR_PA     1000.0f    /* dynamic-pressure floor for the guarded divide [Pa] */
#define GUID_PID_OUT_LIMIT  5000.0f  /* PID clamp BEFORE /q (real limit is the deg saturation) */
#define GUID_DERIV_TAU      0.02f    /* derivative low-pass time constant [s] */
#define GUID_VEL_LIMIT_MS   50.0f    /* lateral velocity gate: stop steering if |vel_enu.x| or
                                        |vel_enu.y| exceeds this (either sign) [m/s]. Set to your
                                        airframe's max plausible lateral velocity. */

/* ---- Servo command mapping ----
 * Setpoint +-100 spans the servo's full +-SERVO_TRAVEL_DEG travel, so 1 deg =
 * SERVO_UNITS_PER_DEG setpoint units (=> 0.5 deg resolution). The linkage is only
 * safe within +-GUID_MAX_FIN_DEG of travel (enforced by the saturation). Each servo
 * has its own mechanical offset (pitch/yaw differ); SERVO_NEUTRAL_*_CMD is the
 * setpoint that yields 0 deg of fin for that servo. */
#define SERVO_TRAVEL_DEG          50.0f
#define SERVO_UNITS_PER_DEG       (100.0f / SERVO_TRAVEL_DEG)                    /* 2.0 units/deg */
#define SERVO_OFFSET_PITCH_DEG    9.0f
#define SERVO_OFFSET_YAW_DEG      9.5f
#define SERVO_OFFSET_PITCH_UNITS  (SERVO_OFFSET_PITCH_DEG * SERVO_UNITS_PER_DEG) /* 18 units */
#define SERVO_OFFSET_YAW_UNITS    (SERVO_OFFSET_YAW_DEG   * SERVO_UNITS_PER_DEG) /* 19 units */
#define SERVO_NEUTRAL_PITCH_CMD   ((int8_t)(-SERVO_OFFSET_PITCH_UNITS))          /* -18 => 0 deg fin */
#define SERVO_NEUTRAL_YAW_CMD     ((int8_t)(-SERVO_OFFSET_YAW_UNITS))            /* -19 => 0 deg fin */

/* Per-servo mount direction: flips the DEFLECTION sign only (a servo can be mounted
 * inverted). It MUST NOT be applied to the offset - negating the whole offset+deflection
 * command (e.g. Effector_set(id, -cmd)) moves the neutral point and the travel window
 * outside the linkage's safe range and drives it into the mechanical endstop. */
#define SERVO_DIR_PITCH           (-1.0f)   /* pitch servo mounted inverted */
#define SERVO_DIR_YAW             (+1.0f)

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
    static bool aborted      = false;   /* velocity-gate latch: once tripped, no restart this flight */
    const bool  enabled = FSD_isSteeringEnabled();

    if (!enabled) {
        if (prev_enabled) {                 /* just disabled -> fins to neutral (0 deg) */
            Effector_set(EFFECTOR_PITCH, SERVO_NEUTRAL_PITCH_CMD);
            Effector_set(EFFECTOR_YAW,   SERVO_NEUTRAL_YAW_CMD);
        }
        prev_enabled = false;
        return;
    }
    if (!prev_enabled) {                     /* just enabled (burnout) -> fresh steering phase */
        PID_reset(&pid_pitch);
        PID_reset(&pid_yaw);
        aborted = false;                     /* clear the latch for this new flight */
    }
    prev_enabled = true;

    /* Latched velocity-gate abort: once the lateral velocity has exceeded the threshold in this
     * steering phase, guidance stays OFF for the rest of the flight - it does NOT restart even if
     * the velocity comes back in range. Fins held at neutral. */
    if (aborted) {
        Effector_set(EFFECTOR_PITCH, SERVO_NEUTRAL_PITCH_CMD);
        Effector_set(EFFECTOR_YAW,   SERVO_NEUTRAL_YAW_CMD);
        return;
    }

    /* Velocity sanity gate: if lateral earth-frame velocity is implausibly large in either
     * direction (|vel_enu.x| or |vel_enu.y| over the threshold), latch the abort, centre the
     * fins, and stop steering for good this flight. */
    if ((fabsf(ahrs->vel_enu.x) > GUID_VEL_LIMIT_MS) ||
        (fabsf(ahrs->vel_enu.y) > GUID_VEL_LIMIT_MS)) {
        aborted = true;
        Effector_set(EFFECTOR_PITCH, SERVO_NEUTRAL_PITCH_CMD);
        Effector_set(EFFECTOR_YAW,   SERVO_NEUTRAL_YAW_CMD);
        PID_reset(&pid_pitch);
        PID_reset(&pid_yaw);
        return;
    }

    const float dt = ahrs->dt;

    /* 1) Earth-frame (ENU) velocity error = velocity - demand.
     *    x=East, y=North are the horizontal drift we steer to null; z=Up (vertical
     *    velocity) is deliberately NOT controlled - its error is zeroed so it never
     *    feeds the fins, and the rocket keeps whatever vertical velocity it has. */
    vectorf_t err_earth;
    err_earth.x = ahrs->vel_enu.x - demand.x;   /* East  */
    err_earth.y = ahrs->vel_enu.y - demand.y;   /* North */
    err_earth.z = 0.0f;                         /* Up: vertical velocity left uncontrolled */

    /* 2) Rotate the earth-frame (ENU) velocity error into the rocket/body frame.
     *    quaternionRotateVector() is earth->body (inverse of the body->earth
     *    rotation AHRS uses for acc_enu). Classic aerospace convention (matches the
     *    AHRS Euler sequence): +x is the rocket long axis, roll about x, pitch about
     *    y, yaw about z. So err_body.x is the AXIAL velocity error (ignored for
     *    lateral steering) and the two lateral components map to:
     *      - pitch (rotation about +y) nulls the body-z velocity error
     *      - yaw   (rotation about +z) nulls the body-y velocity error */
    vectorf_t err_body;
    quaternionRotateVector(&err_body, &err_earth, &(ahrs->orientation.quaternions));

    const float err_pitch = -err_body.z;   /* pitch about +y  <-- flip sign to match your fins */
    const float err_yaw   = err_body.y;   /* yaw   about +z  <-- flip sign to match your fins */

    //const float err_pitch = ahrs->acc_rf.z;   
    //const float err_yaw   = ahrs->acc_rf.y;   

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
     *    +-GUID_MAX_FIN_DEG above, so there is no saturation here - just the conversion.
     *    The mount-direction sign is applied to the DEFLECTION only, then the fixed
     *    per-servo offset is subtracted (see SERVO_DIR_* note above):
     *    setpoint = (dir * fin_deg) * SERVO_UNITS_PER_DEG - SERVO_OFFSET_*_UNITS. */
    const int8_t cmd_pitch = (int8_t)(SERVO_DIR_PITCH * defl_pitch * SERVO_UNITS_PER_DEG
                                      - SERVO_OFFSET_PITCH_UNITS);
    const int8_t cmd_yaw   = (int8_t)(SERVO_DIR_YAW   * defl_yaw   * SERVO_UNITS_PER_DEG
                                      - SERVO_OFFSET_YAW_UNITS);

    Effector_set(EFFECTOR_PITCH, cmd_pitch);
    Effector_set(EFFECTOR_YAW,   cmd_yaw);
}
