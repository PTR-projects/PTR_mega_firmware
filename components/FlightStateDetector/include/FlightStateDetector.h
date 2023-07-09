#pragma once

typedef enum{
	FLIGHTSTATE_STARTUP,
	FLIGHTSTATE_PREFLIGHT,
	FLIGHTSTATE_ME_ACCELERATING,
	FLIGHTSTATE_FREEFLIGHT,
	FLIGHTSTATE_FREEFALL,
	FLIGHTSTATE_DRAGCHUTE_FALL,
	FLIGHTSTATE_MAINSHUTE_FALL,
	FLIGHTSTATE_LANDING
} flightstate_t;

typedef struct{
	flightstate_t state;
	uint8_t state_ready;
} FlightState_t;

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
