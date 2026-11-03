#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#define MISSION_EDGE_COUNT              18
#define MISSION_GROUPS_PER_TRANSITION   4
#define MISSION_CONDS_PER_GROUP         8
#define MISSION_EFFECTORS_PER_EDGE      4
#define MISSION_BINDINGS_PER_ID         4
#define MISSION_EFFECTOR_SLOTS          32
#define MISSION_FILE_MAX                40960

typedef enum {
    MISSION_SIG_ASCENT_RATE = 0,
    MISSION_SIG_VELOCITY_P,
    MISSION_SIG_ALTITUDE,
    MISSION_SIG_ALTITUDE_P,
    MISSION_SIG_ALT_DROP,
    MISSION_SIG_ACC_X,
    MISSION_SIG_ACC_Y,
    MISSION_SIG_ACC_Z,
    MISSION_SIG_GYRO_X,
    MISSION_SIG_GYRO_Y,
    MISSION_SIG_GYRO_Z,
    MISSION_SIG_TIME_LAUNCH_MS,
    MISSION_SIG_TIME_STATE_MS,
    MISSION_SIG_TILT,
    MISSION_SIG_VBAT,
    MISSION_SIG_PRESSURE,
    MISSION_SIG_COUNT
} mission_signal_t;

typedef enum {
    MISSION_CMP_GT = 0,
    MISSION_CMP_GE,
    MISSION_CMP_LT,
    MISSION_CMP_LE,
    MISSION_CMP_COUNT
} mission_cmp_t;

typedef struct {
    uint8_t signal;
    uint8_t cmp;
    float   threshold;
} mission_cond_t;

typedef struct {
    uint8_t       count;
    mission_cond_t conds[MISSION_CONDS_PER_GROUP];
} mission_group_t;

typedef struct {
    uint8_t         enabled;
    uint8_t         order;
    uint8_t         group_count;
    mission_group_t groups[MISSION_GROUPS_PER_TRANSITION];
    uint8_t         effector_count;
    uint8_t         effectors[MISSION_EFFECTORS_PER_EDGE];
} mission_edge_t;

typedef struct {
    uint8_t  id;
    uint8_t  type;
    uint8_t  channel;
    int8_t   active_value;
    int8_t   inactive_value;
    uint8_t  ignore_arm;
    uint32_t activation_time_ms;
} mission_effector_t;

typedef struct {
    float ascent_rate;
    float velocity_p;
    float altitude;
    float altitude_p;
    float alt_drop;
    float acc_x;
    float acc_y;
    float acc_z;
    float gyro_x;
    float gyro_y;
    float gyro_z;
    float time_launch_ms;
    float time_state_ms;
    float tilt;
    float vbat;
    float pressure;
} mission_sample_t;

typedef struct {
    uint8_t to_state;
    uint8_t effector_count;
    uint8_t effectors[MISSION_EFFECTORS_PER_EDGE];
} mission_fire_t;

/**
 * @brief Load /www/mission.cfg or the compiled default when the file is missing or invalid.
 *        Call after Preferences_init() and after the SPIFFS www partition is mounted.
 */
esp_err_t Preferences_mission_init(void);

/**
 * @brief Publish the sensor sample used by the evaluator. Time bases are filled by the poll call.
 */
void Preferences_mission_set_sample(const mission_sample_t *sample);

/**
 * @brief First enabled edge out of from_state whose OR-groups match. Groups are ORed,
 *        comparisons inside a group are ANDed. An enabled edge with no comparisons does not fire.
 * @return true when an edge fired.
 */
bool Preferences_mission_poll(uint8_t from_state, float time_launch_ms, float time_state_ms, mission_fire_t *fire);

uint8_t Preferences_mission_effector_count(void);
const mission_effector_t * Preferences_mission_effector(uint8_t index);

/**
 * @brief JSON description of the current mission. Caller frees the result.
 */
char * Preferences_mission_json(void);

/**
 * @brief Validate a JSON body, store it, and write /www/mission.cfg. Does not restart.
 */
esp_err_t Preferences_mission_store(const char *json);

/**
 * @brief Check the web "crc,json" body, then store it.
 */
esp_err_t Preferences_mission_store_web(char *buf);
