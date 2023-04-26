#pragma once

/**
 * @brief Enum representing the different flight states.
 */
typedef enum{
	FLIGHTSTATE_STARTUP,
	FLIGHTSTATE_PREFLIGHT,
	FLIGHTSTATE_PREFLIGHT_ERROR,
	FLIGHTSTATE_ME_ACCELERATING,
	FLIGHTSTATE_ME_ERROR,
	FLIGHTSTATE_FREEFLIGHT,
	FLIGHTSTATE_FREEFALL,
	FLIGHTSTATE_DRAGCHUTE_FALL,
	FLIGHTSTATE_MAINSHUTE_FALL,
	FLIGHTSTATE_LANDING
} flightstate_t;

/**
 * @brief Data structure representing the flight state.
 * @var FlightState_t::state Current flight state.
 * @var FlightState_t::state_ready Flag indicating if the state is ready.
 */
typedef struct{
	flightstate_t state;
	uint8_t state_ready;
} FlightState_t;


/**
 * @brief Enum representing the arming status.
 */
typedef enum{
	ARMED,
	DISARMED,
	ARMING_ERROR
} armingstatus_t;
