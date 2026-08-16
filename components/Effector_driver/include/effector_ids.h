#pragma once

/**
 * @brief Logical effector identifiers.
 * Edit this file to add or rename effectors for the project.
 * Values are used as direct indices into the registry — keep them
 * zero-based, contiguous, and below EFFECTOR_MAX_COUNT.
 */
typedef enum {
    EFFECTOR_DROGUE     = 0,
    EFFECTOR_MAIN       = 1,
    EFFECTOR_CANSAT_1   = 2,
    EFFECTOR_CANSAT_2   = 3,
    EFFECTOR_CANSAT_3   = 4,
    EFFECTOR_STAGE1_IGN = 5,
    EFFECTOR_STAGE2_IGN = 6,
    EFFECTOR_MOV        = 7,
    EFFECTOR_MFV        = 8
} effector_id_t;
