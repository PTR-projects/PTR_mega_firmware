#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "esp_log.h"
#include "esp_err.h"
#include "cJSON.h"
#include <rom/crc.h>

#include "Preferences.h"
#include "Mission.h"

static const char *TAG = "Mission";
static const char *mission_path = "/www/mission.cfg";

static const uint8_t k_from[MISSION_EDGE_COUNT] = {
    0, 1, 2, 3, 3, 4, 4, 4, 5, 6, 7, 8, 8, 9, 10, 10, 11, 12
};
static const uint8_t k_to[MISSION_EDGE_COUNT] = {
    1, 2, 3, 4, 7, 5, 6, 7, 6, 7, 8, 10, 9, 10, 11, 12, 12, 15
};

static const char *k_state_names[] = {
    "STARTUP", "PREFLIGHT", "BOOST", "SECOND_STAGE_DELAY",
    "SECOND_STAGE_IGNITION", "SECOND_STAGE_BOOST", "FREEFLIGHT", "FREEFALL",
    "DRAGCHUTE_FALL", "DRAGCHUTE_FAILURE", "MAINSHUTE_FALL", "RECOVERY_FAILURE",
    "LANDING", "HARD_ERROR", "SOFT_ERROR", "SHUTDOWN"
};

static const char *k_signals[MISSION_SIG_COUNT] = {
    "ascent_rate", "velocity_p", "altitude", "altitude_p", "alt_drop",
    "acc_x", "acc_y", "acc_z",
    "gyro_x", "gyro_y", "gyro_z",
    "time_launch_ms", "time_state_ms",
    "tilt", "vbat", "pressure"
};

static const char *k_cmps[MISSION_CMP_COUNT] = { ">", ">=", "<", "<=" };

static const char *k_ids[] = {
    "DROGUE", "MAIN", "CANSAT_1", "CANSAT_2", "CANSAT_3",
    "STAGE1_IGN", "STAGE2_IGN", "MOV", "MFV"
};

static const char *k_types[] = { "pwm", "sbus", "igniter" };

#define MISSION_ID_COUNT   (sizeof(k_ids) / sizeof(k_ids[0]))
#define MISSION_TYPE_COUNT (sizeof(k_types) / sizeof(k_types[0]))
#define MISSION_STATE_COUNT (sizeof(k_state_names) / sizeof(k_state_names[0]))

typedef struct {
    uint8_t            effector_count;
    mission_effector_t effectors[MISSION_EFFECTOR_SLOTS];
    mission_edge_t     edges[MISSION_EDGE_COUNT];
} mission_t;

static mission_t        mission_d;
static mission_t        mission_scratch;
static mission_t        mission_saved;
static mission_sample_t mission_sample;
static bool             mission_ready = false;

static const char *json_string(const cJSON *item){
    if(!cJSON_IsString(item)){
        return NULL;
    }
    return item->valuestring;
}

static int name_index(const char *const *table, size_t count, const char *name){
    if(name == NULL){
        return -1;
    }
    for(size_t i = 0; i < count; i++){
        if(strcmp(table[i], name) == 0){
            return (int)i;
        }
    }
    return -1;
}

static void add_cond(uint8_t edge, uint8_t sig, uint8_t cmp, float threshold){
    mission_edge_t *e = &mission_d.edges[edge];
    if(e->group_count == 0){
        e->group_count = 1;
    }
    mission_group_t *g = &e->groups[0];
    if(g->count >= MISSION_CONDS_PER_GROUP){
        return;
    }
    g->conds[g->count].signal    = sig;
    g->conds[g->count].cmp       = cmp;
    g->conds[g->count].threshold = threshold;
    g->count++;
}

static void add_edge_effector(uint8_t edge, uint8_t id){
    mission_edge_t *e = &mission_d.edges[edge];
    if(e->effector_count >= MISSION_EFFECTORS_PER_EDGE){
        return;
    }
    e->effectors[e->effector_count++] = id;
}

static void add_binding(uint8_t id, uint8_t type, uint8_t channel,
                        int8_t active, int8_t inactive, uint8_t ignore_arm, uint32_t time_ms){
    if(mission_d.effector_count >= MISSION_EFFECTOR_SLOTS){
        return;
    }
    mission_effector_t *e = &mission_d.effectors[mission_d.effector_count++];
    e->id                 = id;
    e->type               = type;
    e->channel            = channel;
    e->active_value       = active;
    e->inactive_value     = inactive;
    e->ignore_arm         = ignore_arm;
    e->activation_time_ms = time_ms;
}

static void mission_set_default(void){
    memset(&mission_d, 0, sizeof(mission_d));

    uint8_t seen[MISSION_STATE_COUNT] = {0};
    for(uint8_t i = 0; i < MISSION_EDGE_COUNT; i++){
        mission_d.edges[i].enabled = 1;
        mission_d.edges[i].order   = seen[k_from[i]]++;
    }

    float main_alt_m = 200.0f;
    float staging_delay_ms = 0.0f;
    Preferences_data_t pref;
    if(Preferences_get(&pref) == ESP_OK){
        main_alt_m = (float)pref.main_alt_m;
        staging_delay_ms = (float)pref.staging_delay_ms;
    }

    const float boost_acc = 2.6f * 9.81f;

    add_cond(0,  MISSION_SIG_TIME_STATE_MS, MISSION_CMP_GE, 500.0f);

    add_cond(1,  MISSION_SIG_TIME_STATE_MS, MISSION_CMP_GE, 100.0f);
    add_cond(1,  MISSION_SIG_ACC_X,         MISSION_CMP_GE, boost_acc);

    add_cond(2,  MISSION_SIG_TIME_STATE_MS, MISSION_CMP_GE, 500.0f);
    add_cond(2,  MISSION_SIG_ACC_X,         MISSION_CMP_LT, 0.0f);

    add_cond(3,  MISSION_SIG_TIME_STATE_MS, MISSION_CMP_GE, staging_delay_ms);
    add_edge_effector(3, 6);

    add_cond(4,  MISSION_SIG_TIME_STATE_MS, MISSION_CMP_GE, 200.0f);
    add_cond(4,  MISSION_SIG_ALT_DROP,      MISSION_CMP_GT, 10.0f);

    add_cond(5,  MISSION_SIG_TIME_STATE_MS, MISSION_CMP_GE, 100.0f);
    add_cond(5,  MISSION_SIG_ACC_X,         MISSION_CMP_GE, boost_acc);

    add_cond(6,  MISSION_SIG_TIME_STATE_MS, MISSION_CMP_GE, 3000.0f);

    add_cond(7,  MISSION_SIG_TIME_STATE_MS, MISSION_CMP_GE, 1000.0f);
    add_cond(7,  MISSION_SIG_ALT_DROP,      MISSION_CMP_GT, 10.0f);

    add_cond(8,  MISSION_SIG_TIME_STATE_MS, MISSION_CMP_GE, 200.0f);
    add_cond(8,  MISSION_SIG_ACC_X,         MISSION_CMP_LT, 0.0f);

    add_cond(9,  MISSION_SIG_TIME_STATE_MS, MISSION_CMP_GE, 200.0f);
    add_cond(9,  MISSION_SIG_ALT_DROP,      MISSION_CMP_GT, 10.0f);

    add_cond(10, MISSION_SIG_TIME_STATE_MS, MISSION_CMP_GE, 0.0f);
    add_edge_effector(10, 0);

    add_cond(11, MISSION_SIG_TIME_STATE_MS, MISSION_CMP_GE, 100.0f);
    add_cond(11, MISSION_SIG_ALTITUDE_P,    MISSION_CMP_LE, main_alt_m);
    add_edge_effector(11, 1);

    add_cond(12, MISSION_SIG_TIME_STATE_MS, MISSION_CMP_GE, 2000.0f);
    add_cond(12, MISSION_SIG_ASCENT_RATE,   MISSION_CMP_LT, -60.0f);

    add_cond(13, MISSION_SIG_TIME_STATE_MS, MISSION_CMP_GE, 100.0f);
    add_edge_effector(13, 0);
    add_edge_effector(13, 1);

    add_cond(14, MISSION_SIG_TIME_STATE_MS, MISSION_CMP_GE, 5000.0f);
    add_cond(14, MISSION_SIG_ASCENT_RATE,   MISSION_CMP_LT, -60.0f);

    add_cond(15, MISSION_SIG_TIME_STATE_MS, MISSION_CMP_GE, 30000.0f);
    add_cond(15, MISSION_SIG_ALTITUDE_P,    MISSION_CMP_LT, 200.0f);
    add_cond(15, MISSION_SIG_VELOCITY_P,    MISSION_CMP_GT, -2.0f);

    add_cond(16, MISSION_SIG_TIME_STATE_MS, MISSION_CMP_GE, 30000.0f);
    add_cond(16, MISSION_SIG_ALTITUDE_P,    MISSION_CMP_LT, 200.0f);
    add_cond(16, MISSION_SIG_VELOCITY_P,    MISSION_CMP_GT, -2.0f);

    add_cond(17, MISSION_SIG_TIME_STATE_MS, MISSION_CMP_GE, 30000.0f);

    add_binding(0, 2, 0, 1, 0, 0, 100);
    add_binding(1, 2, 1, 1, 0, 0, 100);
    add_binding(6, 2, 2, 1, 0, 0, 100);
#ifdef CFG_CANSAT_ROCKET
    add_binding(1, 1, 0, 100, 0, 1, 0);
    add_binding(2, 1, 1, 100, 0, 1, 0);
    add_binding(3, 1, 2, 100, 0, 1, 0);
    add_binding(4, 1, 3, 100, 0, 1, 0);
#endif
}

static bool binding_slots_ok(const mission_t *m){
    uint8_t per_id[MISSION_ID_COUNT] = {0};
    for(uint8_t i = 0; i < m->effector_count; i++){
        uint8_t id = m->effectors[i].id;
        if(id >= MISSION_ID_COUNT){
            return false;
        }
        if(++per_id[id] > MISSION_BINDINGS_PER_ID){
            return false;
        }
    }
    return true;
}

static bool parse_cond(cJSON *item, mission_cond_t *out){
    int sig = name_index(k_signals, MISSION_SIG_COUNT, json_string(cJSON_GetObjectItem(item, "sig")));
    int cmp = name_index(k_cmps, MISSION_CMP_COUNT, json_string(cJSON_GetObjectItem(item, "cmp")));
    cJSON *thr = cJSON_GetObjectItem(item, "thr");
    if(sig < 0 || cmp < 0 || !cJSON_IsNumber(thr)){
        return false;
    }
    out->signal    = (uint8_t)sig;
    out->cmp       = (uint8_t)cmp;
    out->threshold = (float)thr->valuedouble;
    return true;
}

static bool parse_groups(cJSON *groups, mission_edge_t *edge){
    if(!cJSON_IsArray(groups)){
        return false;
    }
    int n = cJSON_GetArraySize(groups);
    if(n < 0 || n > MISSION_GROUPS_PER_TRANSITION){
        return false;
    }
    memset(edge->groups, 0, sizeof(edge->groups));
    edge->group_count = (uint8_t)n;
    for(int g = 0; g < n; g++){
        cJSON *group = cJSON_GetArrayItem(groups, g);
        cJSON *conds = cJSON_GetObjectItem(group, "conds");
        if(!cJSON_IsArray(conds)){
            return false;
        }
        int cn = cJSON_GetArraySize(conds);
        if(cn < 0 || cn > MISSION_CONDS_PER_GROUP){
            return false;
        }
        edge->groups[g].count = (uint8_t)cn;
        for(int c = 0; c < cn; c++){
            if(!parse_cond(cJSON_GetArrayItem(conds, c), &edge->groups[g].conds[c])){
                return false;
            }
        }
    }
    return true;
}

static bool parse_edge_effectors(cJSON *arr, mission_edge_t *edge){
    if(!cJSON_IsArray(arr)){
        return false;
    }
    int n = cJSON_GetArraySize(arr);
    if(n < 0 || n > MISSION_EFFECTORS_PER_EDGE){
        return false;
    }
    edge->effector_count = (uint8_t)n;
    for(int i = 0; i < n; i++){
        cJSON *item = cJSON_GetArrayItem(arr, i);
        if(!cJSON_IsNumber(item) || item->valueint < 0 || item->valueint >= (int)MISSION_ID_COUNT){
            return false;
        }
        edge->effectors[i] = (uint8_t)item->valueint;
    }
    return true;
}

static bool parse_effectors(cJSON *arr, mission_t *m){
    if(!cJSON_IsArray(arr)){
        return false;
    }
    int n = cJSON_GetArraySize(arr);
    if(n < 0 || n > MISSION_EFFECTOR_SLOTS){
        return false;
    }
    m->effector_count = 0;
    for(int i = 0; i < n; i++){
        cJSON *item = cJSON_GetArrayItem(arr, i);
        cJSON *id = cJSON_GetObjectItem(item, "id");
        cJSON *ch = cJSON_GetObjectItem(item, "ch");
        cJSON *active = cJSON_GetObjectItem(item, "active");
        cJSON *inactive = cJSON_GetObjectItem(item, "inactive");
        cJSON *time_ms = cJSON_GetObjectItem(item, "time_ms");
        int type = name_index(k_types, MISSION_TYPE_COUNT, json_string(cJSON_GetObjectItem(item, "type")));
        if(!cJSON_IsNumber(id) || !cJSON_IsNumber(ch) || !cJSON_IsNumber(active) ||
           !cJSON_IsNumber(inactive) || !cJSON_IsNumber(time_ms) || type < 0){
            return false;
        }
        if(id->valueint < 0 || id->valueint >= (int)MISSION_ID_COUNT){
            return false;
        }
        if(ch->valueint < 0 || ch->valueint > 255){
            return false;
        }
        if(active->valueint < -128 || active->valueint > 127 ||
           inactive->valueint < -128 || inactive->valueint > 127){
            return false;
        }
        uint32_t activation = (uint32_t)time_ms->valueint;
        if(activation != 0 && (activation < 10 || activation > 10000 || (activation % 10) != 0)){
            return false;
        }
        mission_effector_t *e = &m->effectors[m->effector_count++];
        e->id                 = (uint8_t)id->valueint;
        e->type               = (uint8_t)type;
        e->channel            = (uint8_t)ch->valueint;
        e->active_value       = (int8_t)active->valueint;
        e->inactive_value     = (int8_t)inactive->valueint;
        e->ignore_arm         = cJSON_IsTrue(cJSON_GetObjectItem(item, "ignore_arm")) ? 1 :
                                (cJSON_IsNumber(cJSON_GetObjectItem(item, "ignore_arm")) &&
                                 cJSON_GetObjectItem(item, "ignore_arm")->valueint != 0);
        e->activation_time_ms = activation;
    }
    return binding_slots_ok(m);
}

static bool mission_from_json(const char *json){
    cJSON *root = cJSON_Parse(json);
    if(root == NULL){
        return false;
    }

    memcpy(&mission_scratch, &mission_d, sizeof(mission_scratch));

    cJSON *effectors = cJSON_GetObjectItem(root, "effectors");
    if(effectors != NULL && !parse_effectors(effectors, &mission_scratch)){
        cJSON_Delete(root);
        return false;
    }

    cJSON *edges = cJSON_GetObjectItem(root, "edges");
    if(edges != NULL){
        if(!cJSON_IsArray(edges)){
            cJSON_Delete(root);
            return false;
        }
        int n = cJSON_GetArraySize(edges);
        for(int i = 0; i < n; i++){
            cJSON *item = cJSON_GetArrayItem(edges, i);
            cJSON *id = cJSON_GetObjectItem(item, "id");
            if(!cJSON_IsNumber(id) || id->valueint < 0 || id->valueint >= MISSION_EDGE_COUNT){
                cJSON_Delete(root);
                return false;
            }
            mission_edge_t *edge = &mission_scratch.edges[id->valueint];
            cJSON *enabled = cJSON_GetObjectItem(item, "enabled");
            if(cJSON_IsBool(enabled) || cJSON_IsNumber(enabled)){
                edge->enabled = cJSON_IsTrue(enabled) || (cJSON_IsNumber(enabled) && enabled->valueint != 0);
            }
            cJSON *order = cJSON_GetObjectItem(item, "order");
            if(cJSON_IsNumber(order) && order->valueint >= 0 && order->valueint <= 255){
                edge->order = (uint8_t)order->valueint;
            }
            cJSON *groups = cJSON_GetObjectItem(item, "groups");
            if(groups != NULL && !parse_groups(groups, edge)){
                cJSON_Delete(root);
                return false;
            }
            cJSON *eff = cJSON_GetObjectItem(item, "effectors");
            if(eff != NULL && !parse_edge_effectors(eff, edge)){
                cJSON_Delete(root);
                return false;
            }
        }
    }

    cJSON_Delete(root);
    memcpy(&mission_d, &mission_scratch, sizeof(mission_d));
    return true;
}

static bool mission_write_file(const char *json){
    const char *tmp_path = "/www/mission.cfg.tmp";
    size_t len = strlen(json);
    char *file = malloc(2 + len);
    if(file == NULL){
        return false;
    }
    memcpy(file + 2, json, len);
    uint16_t crc = crc16_le(0, (const uint8_t *)(file + 2), len);
    file[0] = (char)(crc & 0xFF);
    file[1] = (char)((crc >> 8) & 0xFF);

    FILE *f = fopen(tmp_path, "w");
    if(f == NULL){
        free(file);
        return false;
    }
    size_t wrote = fwrite(file, 1, 2 + len, f);
    fclose(f);
    free(file);
    if(wrote != 2 + len){
        remove(tmp_path);
        return false;
    }
    remove(mission_path);
    if(rename(tmp_path, mission_path) != 0){
        return false;
    }
    return true;
}

static bool mission_read_file(void){
    struct stat st;
    if(stat(mission_path, &st) != 0 || st.st_size < 3 || st.st_size > (MISSION_FILE_MAX + 2)){
        return false;
    }

    FILE *f = fopen(mission_path, "r");
    if(f == NULL){
        return false;
    }

    char *buf = calloc(1, (size_t)st.st_size + 1);
    if(buf == NULL){
        fclose(f);
        return false;
    }
    size_t n = fread(buf, 1, (size_t)st.st_size, f);
    fclose(f);
    if(n < 3){
        free(buf);
        return false;
    }

    uint16_t stored = (uint16_t)(uint8_t)buf[0] | ((uint16_t)(uint8_t)buf[1] << 8);
    uint16_t crc = crc16_le(0, (const uint8_t *)(buf + 2), strlen(buf + 2));
    if(stored != crc){
        ESP_LOGE(TAG, "mission.cfg CRC mismatch");
        free(buf);
        return false;
    }
    bool ok = mission_from_json(buf + 2);
    free(buf);
    return ok;
}

static cJSON *cond_json(const mission_cond_t *cond){
    cJSON *item = cJSON_CreateObject();
    cJSON_AddStringToObject(item, "sig", k_signals[cond->signal]);
    cJSON_AddStringToObject(item, "cmp", k_cmps[cond->cmp]);
    cJSON_AddNumberToObject(item, "thr", cond->threshold);
    return item;
}

char * Preferences_mission_json(void){
    cJSON *root = cJSON_CreateObject();
    cJSON *states = cJSON_CreateArray();
    cJSON *signals = cJSON_CreateArray();
    cJSON *cmps = cJSON_CreateArray();
    cJSON *ids = cJSON_CreateArray();
    cJSON *types = cJSON_CreateArray();
    cJSON *effectors = cJSON_CreateArray();
    cJSON *edges = cJSON_CreateArray();

    for(size_t i = 0; i < MISSION_STATE_COUNT; i++){
        cJSON_AddItemToArray(states, cJSON_CreateString(k_state_names[i]));
    }
    for(int i = 0; i < MISSION_SIG_COUNT; i++){
        cJSON_AddItemToArray(signals, cJSON_CreateString(k_signals[i]));
    }
    for(int i = 0; i < MISSION_CMP_COUNT; i++){
        cJSON_AddItemToArray(cmps, cJSON_CreateString(k_cmps[i]));
    }
    for(size_t i = 0; i < MISSION_ID_COUNT; i++){
        cJSON_AddItemToArray(ids, cJSON_CreateString(k_ids[i]));
    }
    for(size_t i = 0; i < MISSION_TYPE_COUNT; i++){
        cJSON_AddItemToArray(types, cJSON_CreateString(k_types[i]));
    }
    for(uint8_t i = 0; i < mission_d.effector_count; i++){
        const mission_effector_t *e = &mission_d.effectors[i];
        cJSON *item = cJSON_CreateObject();
        cJSON_AddNumberToObject(item, "id", e->id);
        cJSON_AddStringToObject(item, "type", k_types[e->type]);
        cJSON_AddNumberToObject(item, "ch", e->channel);
        cJSON_AddNumberToObject(item, "active", e->active_value);
        cJSON_AddNumberToObject(item, "inactive", e->inactive_value);
        cJSON_AddBoolToObject(item, "ignore_arm", e->ignore_arm);
        cJSON_AddNumberToObject(item, "time_ms", e->activation_time_ms);
        cJSON_AddItemToArray(effectors, item);
    }
    for(uint8_t i = 0; i < MISSION_EDGE_COUNT; i++){
        const mission_edge_t *e = &mission_d.edges[i];
        cJSON *item = cJSON_CreateObject();
        cJSON *groups = cJSON_CreateArray();
        cJSON *eff = cJSON_CreateArray();
        cJSON_AddNumberToObject(item, "id", i);
        cJSON_AddNumberToObject(item, "from", k_from[i]);
        cJSON_AddNumberToObject(item, "to", k_to[i]);
        cJSON_AddStringToObject(item, "from_name", k_state_names[k_from[i]]);
        cJSON_AddStringToObject(item, "to_name", k_state_names[k_to[i]]);
        cJSON_AddBoolToObject(item, "enabled", e->enabled);
        cJSON_AddNumberToObject(item, "order", e->order);
        for(uint8_t g = 0; g < e->group_count; g++){
            cJSON *group = cJSON_CreateObject();
            cJSON *conds = cJSON_CreateArray();
            for(uint8_t c = 0; c < e->groups[g].count; c++){
                cJSON_AddItemToArray(conds, cond_json(&e->groups[g].conds[c]));
            }
            cJSON_AddItemToObject(group, "conds", conds);
            cJSON_AddItemToArray(groups, group);
        }
        for(uint8_t n = 0; n < e->effector_count; n++){
            cJSON_AddItemToArray(eff, cJSON_CreateNumber(e->effectors[n]));
        }
        cJSON_AddItemToObject(item, "groups", groups);
        cJSON_AddItemToObject(item, "effectors", eff);
        cJSON_AddItemToArray(edges, item);
    }

    cJSON_AddNumberToObject(root, "groups_max", MISSION_GROUPS_PER_TRANSITION);
    cJSON_AddNumberToObject(root, "conds_max", MISSION_CONDS_PER_GROUP);
    cJSON_AddNumberToObject(root, "bindings_per_id", MISSION_BINDINGS_PER_ID);
    cJSON_AddNumberToObject(root, "effectors_per_edge", MISSION_EFFECTORS_PER_EDGE);
    cJSON_AddItemToObject(root, "states", states);
    cJSON_AddItemToObject(root, "signals", signals);
    cJSON_AddItemToObject(root, "cmps", cmps);
    cJSON_AddItemToObject(root, "ids", ids);
    cJSON_AddItemToObject(root, "types", types);
    cJSON_AddItemToObject(root, "effectors", effectors);
    cJSON_AddItemToObject(root, "edges", edges);

    char *text = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return text;
}

esp_err_t Preferences_mission_init(void){
    mission_set_default();
    mission_ready = true;
    if(!mission_read_file()){
        mission_set_default();
        ESP_LOGW(TAG, "Using compiled mission default");
    } else {
        ESP_LOGI(TAG, "Loaded mission.cfg");
    }
    return ESP_OK;
}

void Preferences_mission_set_sample(const mission_sample_t *sample){
    if(sample != NULL){
        mission_sample = *sample;
    }
}

static float signal_value(uint8_t signal){
    switch(signal){
    case MISSION_SIG_ASCENT_RATE:    return mission_sample.ascent_rate;
    case MISSION_SIG_VELOCITY_P:     return mission_sample.velocity_p;
    case MISSION_SIG_ALTITUDE:       return mission_sample.altitude;
    case MISSION_SIG_ALTITUDE_P:     return mission_sample.altitude_p;
    case MISSION_SIG_ALT_DROP:       return mission_sample.alt_drop;
    case MISSION_SIG_ACC_X:          return mission_sample.acc_x;
    case MISSION_SIG_ACC_Y:          return mission_sample.acc_y;
    case MISSION_SIG_ACC_Z:          return mission_sample.acc_z;
    case MISSION_SIG_GYRO_X:         return mission_sample.gyro_x;
    case MISSION_SIG_GYRO_Y:         return mission_sample.gyro_y;
    case MISSION_SIG_GYRO_Z:         return mission_sample.gyro_z;
    case MISSION_SIG_TIME_LAUNCH_MS: return mission_sample.time_launch_ms;
    case MISSION_SIG_TIME_STATE_MS:  return mission_sample.time_state_ms;
    case MISSION_SIG_TILT:           return mission_sample.tilt;
    case MISSION_SIG_VBAT:           return mission_sample.vbat;
    case MISSION_SIG_PRESSURE:       return mission_sample.pressure;
    default:                         return 0.0f;
    }
}

static bool cmp_holds(uint8_t cmp, float value, float threshold){
    switch(cmp){
    case MISSION_CMP_GT: return value > threshold;
    case MISSION_CMP_GE: return value >= threshold;
    case MISSION_CMP_LT: return value < threshold;
    case MISSION_CMP_LE: return value <= threshold;
    default:             return false;
    }
}

static bool group_holds(const mission_group_t *group){
    if(group->count == 0){
        return false;
    }
    for(uint8_t i = 0; i < group->count; i++){
        const mission_cond_t *c = &group->conds[i];
        if(!cmp_holds(c->cmp, signal_value(c->signal), c->threshold)){
            return false;
        }
    }
    return true;
}

static bool edge_holds(const mission_edge_t *edge){
    if(!edge->enabled || edge->group_count == 0){
        return false;
    }
    for(uint8_t g = 0; g < edge->group_count; g++){
        if(group_holds(&edge->groups[g])){
            return true;
        }
    }
    return false;
}

bool Preferences_mission_poll(uint8_t from_state, float time_launch_ms, float time_state_ms, mission_fire_t *fire){
    if(!mission_ready || fire == NULL || from_state >= MISSION_STATE_COUNT){
        return false;
    }

    mission_sample.time_launch_ms = time_launch_ms;
    mission_sample.time_state_ms  = time_state_ms;

    int best = -1;
    for(uint8_t i = 0; i < MISSION_EDGE_COUNT; i++){
        if(k_from[i] != from_state || !edge_holds(&mission_d.edges[i])){
            continue;
        }
        if(best < 0 ||
           mission_d.edges[i].order < mission_d.edges[best].order ||
           (mission_d.edges[i].order == mission_d.edges[best].order && i < best)){
            best = i;
        }
    }
    if(best < 0){
        return false;
    }

    fire->to_state = k_to[best];
    fire->effector_count = mission_d.edges[best].effector_count;
    memcpy(fire->effectors, mission_d.edges[best].effectors, fire->effector_count);
    return true;
}

uint8_t Preferences_mission_effector_count(void){
    return mission_d.effector_count;
}

const mission_effector_t * Preferences_mission_effector(uint8_t index){
    if(index >= mission_d.effector_count){
        return NULL;
    }
    return &mission_d.effectors[index];
}

esp_err_t Preferences_mission_store(const char *json){
    if(json == NULL || strlen(json) == 0 || strlen(json) >= MISSION_FILE_MAX){
        return ESP_ERR_INVALID_SIZE;
    }
    memcpy(&mission_saved, &mission_d, sizeof(mission_saved));
    if(!mission_from_json(json)){
        ESP_LOGE(TAG, "Mission JSON rejected");
        return ESP_ERR_INVALID_ARG;
    }
    if(!mission_write_file(json)){
        memcpy(&mission_d, &mission_saved, sizeof(mission_d));
        return ESP_FAIL;
    }
    mission_ready = true;
    return ESP_OK;
}

esp_err_t Preferences_mission_store_web(char *buf){
    if(buf == NULL){
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t crc_from_json = (uint16_t)atoi(buf);
    buf = strchr(buf, '{');
    if(buf == NULL){
        return ESP_FAIL;
    }

    uint16_t romCRC = ~crc16_be((uint16_t)~0xffff, (const uint8_t *)buf, strlen(buf));
    if(crc_from_json != romCRC){
        ESP_LOGE(TAG, "Mission web CRC mismatch");
        return ESP_ERR_INVALID_CRC;
    }
    return Preferences_mission_store(buf);
}
