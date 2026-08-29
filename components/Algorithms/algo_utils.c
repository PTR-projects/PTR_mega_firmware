/*
 * algo_utils.c - hardware-agnostic control/guidance math helpers.
 * See algo_utils.h for the contract.
 */
#include "algo_utils.h"

#define ALGO_AIR_R_SPECIFIC   287.05f   /* J/(kg*K), dry air */
#define ALGO_DIV_EPSILON      1e-6f

float Algo_saturate(float value, float min, float max)
{
    if (value > max) return max;
    if (value < min) return min;
    return value;
}

float Algo_safeDivide(float numerator, float denominator, float min_abs_denominator)
{
    float floor_abs = (min_abs_denominator > 0.0f) ? min_abs_denominator : ALGO_DIV_EPSILON;

    if (denominator >= 0.0f) {
        if (denominator < floor_abs)  denominator = floor_abs;   /* clamp up toward +floor */
    } else {
        if (denominator > -floor_abs) denominator = -floor_abs;  /* clamp toward -floor, keep sign */
    }

    return numerator / denominator;
}

float Algo_dynamicPressure(float air_density, float velocity)
{
    return 0.5f * air_density * velocity * velocity;
}

float Algo_airDensity(float pressure_pa, float temperature_c)
{
    float t_kelvin = temperature_c + 273.15f;
    if (t_kelvin < 1.0f) t_kelvin = 1.0f;   /* guard non-physical / bad sensor reading */
    return pressure_pa / (ALGO_AIR_R_SPECIFIC * t_kelvin);
}
