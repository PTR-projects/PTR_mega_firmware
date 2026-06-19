#pragma once

#include "esp_err.h"
#include "AHRS_driver.h"

/**
 * @brief Enum representing the cansat deployment states.
 */
typedef enum {
    CANSAT_IDLE,            /*!< Inactive, waiting to be armed. */
    CANSAT_ARMED,           /*!< Armed, monitoring time-to-apogee estimate. */
    CANSAT_DEPLOY_BATCH1,   /*!< Batch 1 deployed (~3 s before apogee). */
    CANSAT_DEPLOY_BATCH2,   /*!< Batch 2 deployed (~2 s before apogee). */
    CANSAT_DEPLOY_BATCH3,   /*!< Batch 3 deployed (~1 s before apogee). */
    CANSAT_DEPLOYED,        /*!< All batches deployed, apogee passed. */
    CANSAT_MANUAL,          /*!< Manual override — batches controlled individually via forceOpen/forceClose. */
} cansat_state_t;

/**
 * @brief Data structure representing the cansat deployment state.
 */
typedef struct {
    cansat_state_t state;            /*!< Current deployment state. */
    uint8_t        state_ready;      /*!< Flag: entry actions for current state have run. */
    uint8_t        batches_deployed; /*!< Number of batches deployed by the automatic sequence (0–3). */
    uint8_t        batch_open[3];    /*!< Per-batch open state: 1 = open, 0 = closed. Index 0 = batch 1. */
} Cansat_t;

/**
 * @brief Initializes the Cansat deployment module.
 * @param[in] ahrs Pointer to the AHRS data structure.
 * @return ESP_OK on success, ESP_FAIL if ahrs is NULL.
 */
esp_err_t Cansat_init(AHRS_t * ahrs);

/**
 * @brief Runs one step of the deployment state machine. Call at main loop rate.
 * @param[in] time_ms Current system time in milliseconds.
 * @return ESP_OK always.
 */
esp_err_t Cansat_compute(uint64_t time_ms);

/**
 * @brief Arms the deployment state machine (transitions from IDLE to ARMED).
 */
void Cansat_arm();

/**
 * @brief Disarms the deployment state machine (returns to IDLE, resets all batch states).
 */
void Cansat_disarm();

/**
 * @brief Transitions to CANSAT_MANUAL state, allowing individual batch control.
 */
void Cansat_enterManual();

/**
 * @brief Exits CANSAT_MANUAL state and returns to CANSAT_IDLE.
 */
void Cansat_exitManual();

/**
 * @brief Opens (deploys) the specified batch. Only executes when in CANSAT_MANUAL state.
 * @param[in] batch Batch number to open (1–3).
 */
void Cansat_forceOpen(uint8_t batch);

/**
 * @brief Closes the specified batch. Only executes when in CANSAT_MANUAL state.
 * @param[in] batch Batch number to close (1–3).
 */
void Cansat_forceClose(uint8_t batch);

/**
 * @brief Returns a pointer to the current cansat deployment data.
 * @return Pointer to the internal Cansat_t structure.
 */
Cansat_t * Cansat_getData();
