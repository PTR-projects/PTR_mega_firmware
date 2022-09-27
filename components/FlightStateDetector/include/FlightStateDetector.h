#pragma once

typedef enum{
	FLIGHTSTATE_PREFLIGHT
} flightstate_t;

typedef struct{
	flightstate_t state;
} FlightState_t;
