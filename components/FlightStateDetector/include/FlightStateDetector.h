#pragma once

/**
 * @brief Enum representing the different flight states.
 * 
 */
typedef enum{
	FLIGHTSTATE_STARTUP,
	FLIGHTSTATE_PREFLIGHT,
	FLIGHTSTATE_BOOST,
	FLIGHTSTATE_SECOND_STAGE_DELAY,
	FLIGHTSTATE_SECOND_STAGE_IGNITION,
	FLIGHTSTATE_SECOND_STAGE_BOOST,
	FLIGHTSTATE_FREEFLIGHT,
	FLIGHTSTATE_FREEFALL,
	FLIGHTSTATE_DRAGCHUTE_FALL,
	FLIGHTSTATE_DRAGCHUTE_FAILURE,
	FLIGHTSTATE_MAINSHUTE_FALL,
	FLIGHTSTATE_RECOVERY_FAILURE,
	FLIGHTSTATE_LANDING,
	FLIGHTSTATE_HARD_ERROR,
	FLIGHTSTATE_SOFT_ERROR,
	FLIGHTSTATE_SHUTDOWN,
} flightstate_t;


/**
 * @brief Data structure representing the flight state.
 * @var FlightState_t::state Current flight state.
 * @var FlightState_t::state_ready Flag indicating if the state is ready.
 * @var FlightState_t::ignition_time_ms Time of the motor ignition in ms
 */
typedef struct{
	flightstate_t state;
	uint8_t state_ready;
	uint64_t ignition_time_ms; 
} FlightState_t;

typedef struct{
	float main_alt;
	float drouge_alt;

	float rail_height;
	float max_tilt;

	int staging_delay_ms;
	float staging_max_tilt;

	int expected_motor_burnout_time_s;
	int expected_apogee_time_s;
	int expected_main_time_s;
} FSD_settings_t;

/**
 * @brief Enum representing the arming status.
 */
typedef enum{
	ARMED,
	DISARMED,
	ARMING_ERROR
} armingstatus_t;

esp_err_t FSD_init(AHRS_t * ahrs);
esp_err_t FSD_detect(uint64_t time_ms);
void FSD_forceState(flightstate_t new_state);
flightstate_t FSD_getState();
armingstatus_t FSD_checkArmed();
void FSD_disarming();
void FSD_arming();
