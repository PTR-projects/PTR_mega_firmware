#include <stdio.h>
#include <stdbool.h>
#include "esp_log.h"
#include "esp_err.h"

#include "AHRS_driver.h"
#include "Servo_driver.h"
#include "Cansat_driver.h"

#define TIME_ELAPSED(start_ms, now_ms, wait_ms)  (start_ms <= (now_ms - wait_ms))

#define CANSAT_BATCH1_TIME_S          3.0f  /*!< Deploy batch 1 when T-3 s to apogee. */
#define CANSAT_BATCH2_TIME_S          2.0f  /*!< Deploy batch 2 when T-2 s to apogee. */
#define CANSAT_BATCH3_TIME_S          1.0f  /*!< Deploy batch 3 when T-1 s to apogee. */
#define CANSAT_POST_BATCH3_TIMEOUT_MS 3000  /*!< Fallback: transition to DEPLOYED this many ms after batch 3. */

//----- Private function declarations ----------
static void CansatState_IDLE         (uint64_t time_ms, Cansat_t * cs, AHRS_t * ahrs);
static void CansatState_ARMED        (uint64_t time_ms, Cansat_t * cs, AHRS_t * ahrs);
static void CansatState_DEPLOY_BATCH1(uint64_t time_ms, Cansat_t * cs, AHRS_t * ahrs);
static void CansatState_DEPLOY_BATCH2(uint64_t time_ms, Cansat_t * cs, AHRS_t * ahrs);
static void CansatState_DEPLOY_BATCH3(uint64_t time_ms, Cansat_t * cs, AHRS_t * ahrs);
static void CansatState_DEPLOYED     (uint64_t time_ms, Cansat_t * cs, AHRS_t * ahrs);
static void CansatState_MANUAL       (uint64_t time_ms, Cansat_t * cs, AHRS_t * ahrs);

static void Cansat_deployBatch(uint8_t batch);
static void Cansat_closeBatch (uint8_t batch);

//----- Private variables ----------
static const char *TAG = "Cansat";

static Cansat_t  cansat_d;
static AHRS_t  * AHRS_ptr;
static uint64_t  stateChangeTime = 0;


//----- Public API ----------

esp_err_t Cansat_init(AHRS_t * ahrs){
    if(ahrs == NULL){
        return ESP_FAIL;
    }

    AHRS_ptr = ahrs;

    cansat_d.state            = CANSAT_IDLE;
    cansat_d.state_ready      = false;
    cansat_d.batches_deployed = 0;
    cansat_d.batch_open[0]    = false;
    cansat_d.batch_open[1]    = false;
    cansat_d.batch_open[2]    = false;

    return ESP_OK;
}

Cansat_t * Cansat_getData(){
    return &cansat_d;
}

void Cansat_arm(){
    cansat_d.state       = CANSAT_ARMED;
    cansat_d.state_ready = false;
    ESP_LOGI(TAG, "Armed");
}

void Cansat_disarm(){
    cansat_d.state            = CANSAT_IDLE;
    cansat_d.state_ready      = false;
    cansat_d.batches_deployed = 0;
    cansat_d.batch_open[0]    = false;
    cansat_d.batch_open[1]    = false;
    cansat_d.batch_open[2]    = false;
    ESP_LOGI(TAG, "Disarmed");
}

void Cansat_enterManual(){
    cansat_d.state       = CANSAT_MANUAL;
    cansat_d.state_ready = false;
    ESP_LOGW(TAG, "Entering manual mode");
}

void Cansat_exitManual(){
    cansat_d.state            = CANSAT_IDLE;
    cansat_d.state_ready      = false;
    cansat_d.batches_deployed = 0;
    cansat_d.batch_open[0]    = false;
    cansat_d.batch_open[1]    = false;
    cansat_d.batch_open[2]    = false;
    ESP_LOGW(TAG, "Exiting manual mode, returning to IDLE");
}

void Cansat_forceOpen(uint8_t batch){
    if(cansat_d.state != CANSAT_MANUAL){
        ESP_LOGE(TAG, "forceOpen ignored - not in manual mode");
        return;
    }
    cansat_d.batch_open[batch - 1] = true;
    Cansat_deployBatch(batch);
}

void Cansat_forceClose(uint8_t batch){
    if(cansat_d.state != CANSAT_MANUAL){
        ESP_LOGE(TAG, "forceClose ignored - not in manual mode");
        return;
    }
    cansat_d.batch_open[batch - 1] = false;
    Cansat_closeBatch(batch);
}

esp_err_t Cansat_compute(uint64_t time_ms){
    Cansat_t * cs   = &cansat_d;
    AHRS_t   * ahrs = AHRS_ptr;

    switch(cansat_d.state){
    case CANSAT_IDLE:
        CansatState_IDLE(time_ms, cs, ahrs);
        break;

    case CANSAT_ARMED:
        CansatState_ARMED(time_ms, cs, ahrs);
        break;

    case CANSAT_DEPLOY_BATCH1:
        CansatState_DEPLOY_BATCH1(time_ms, cs, ahrs);
        break;

    case CANSAT_DEPLOY_BATCH2:
        CansatState_DEPLOY_BATCH2(time_ms, cs, ahrs);
        break;

    case CANSAT_DEPLOY_BATCH3:
        CansatState_DEPLOY_BATCH3(time_ms, cs, ahrs);
        break;

    case CANSAT_DEPLOYED:
        CansatState_DEPLOYED(time_ms, cs, ahrs);
        break;

    case CANSAT_MANUAL:
        CansatState_MANUAL(time_ms, cs, ahrs);
        break;

    default:
        ESP_LOGE(TAG, "Unknown state! Disarming.");
        Cansat_disarm();
        break;
    }

    return ESP_OK;
}


//----- State handlers ----------

static void CansatState_IDLE(uint64_t time_ms, Cansat_t * cs, AHRS_t * ahrs){
    //Executed only once
    if(!(cs->state_ready)){
        cs->state_ready      = true;
        cs->batches_deployed = 0;
        stateChangeTime      = time_ms;
    }

    //Executed every loop
    // (nothing — waiting for Cansat_arm() call)

    //State change conditions: none, arm() transitions externally
}

static void CansatState_ARMED(uint64_t time_ms, Cansat_t * cs, AHRS_t * ahrs){
    //Executed only once
    if(!(cs->state_ready)){
        cs->state_ready = true;
        stateChangeTime = time_ms;
    }

    //Executed every loop

    //State change conditions
    if((ahrs->time_to_apogee_est > 0.0f) && (ahrs->time_to_apogee_est <= CANSAT_BATCH1_TIME_S)){
        cs->state       = CANSAT_DEPLOY_BATCH1;
        cs->state_ready = false;
    }
}

static void CansatState_DEPLOY_BATCH1(uint64_t time_ms, Cansat_t * cs, AHRS_t * ahrs){
    //Executed only once
    if(!(cs->state_ready)){
        cs->state_ready        = true;
        stateChangeTime        = time_ms;
        cs->batches_deployed   = 1;
        cs->batch_open[0]      = true;

        Cansat_deployBatch(1);
    }

    //Executed every loop

    //State change conditions
    if((ahrs->time_to_apogee_est > 0.0f) && (ahrs->time_to_apogee_est <= CANSAT_BATCH2_TIME_S)){
        cs->state       = CANSAT_DEPLOY_BATCH2;
        cs->state_ready = false;
    }
}

static void CansatState_DEPLOY_BATCH2(uint64_t time_ms, Cansat_t * cs, AHRS_t * ahrs){
    //Executed only once
    if(!(cs->state_ready)){
        cs->state_ready        = true;
        stateChangeTime        = time_ms;
        cs->batches_deployed   = 2;
        cs->batch_open[1]      = true;

        Cansat_deployBatch(2);
    }

    //Executed every loop

    //State change conditions
    if((ahrs->time_to_apogee_est > 0.0f) && (ahrs->time_to_apogee_est <= CANSAT_BATCH3_TIME_S)){
        cs->state       = CANSAT_DEPLOY_BATCH3;
        cs->state_ready = false;
    }
}

static void CansatState_DEPLOY_BATCH3(uint64_t time_ms, Cansat_t * cs, AHRS_t * ahrs){
    //Executed only once
    if(!(cs->state_ready)){
        cs->state_ready        = true;
        stateChangeTime        = time_ms;
        cs->batches_deployed   = 3;
        cs->batch_open[2]      = true;

        Cansat_deployBatch(3);
    }

    //Executed every loop

    //State change conditions: apogee passed (ascent_rate went non-positive)
    //or fallback timeout in case ascent_rate estimate is noisy
    if((ahrs->ascent_rate <= 0.0f) || TIME_ELAPSED(stateChangeTime, time_ms, CANSAT_POST_BATCH3_TIMEOUT_MS)){
        cs->state       = CANSAT_DEPLOYED;
        cs->state_ready = false;
    }
}

static void CansatState_DEPLOYED(uint64_t time_ms, Cansat_t * cs, AHRS_t * ahrs){
    //Executed only once
    if(!(cs->state_ready)){
        cs->state_ready = true;
        stateChangeTime = time_ms;
        ESP_LOGI(TAG, "All %d batches deployed. Apogee passed.", cs->batches_deployed);
    }

    //Executed every loop

    //State change conditions: terminal state, no automatic transitions
}

static void CansatState_MANUAL(uint64_t time_ms, Cansat_t * cs, AHRS_t * ahrs){
    //Executed only once
    if(!(cs->state_ready)){
        cs->state_ready = true;
        stateChangeTime = time_ms;
        ESP_LOGW(TAG, "Manual mode active. Batches: 1=%d 2=%d 3=%d",
                 cs->batch_open[0], cs->batch_open[1], cs->batch_open[2]);
    }

    //Executed every loop
    // (control happens via Cansat_forceOpen / Cansat_forceClose)

    //State change conditions: none, disarm() or arm() transitions externally
}


//----- Deployment functions ----------

static void Cansat_deployBatch(uint8_t batch){
    // TODO: replace with real servo/mechanism actuation for each batch
    ESP_LOGI(TAG, "Deploying batch %d", batch);
    Servo_driveSinglePWM(batch, 100);
}

static void Cansat_closeBatch(uint8_t batch){
    // TODO: replace with real servo/mechanism retraction for each batch
    ESP_LOGI(TAG, "Closing batch %d", batch);
    Servo_driveSinglePWM(batch, -100);
}
