#pragma once

/**
 * @file algo_utils.h
 * @brief Small hardware-agnostic math helpers for control/guidance.
 *
 * All pure functions over floats - no state, no I/O.
 */

/**
 * @brief Clamp @p value to the closed interval [min, max].
 *
 * Used as the fin-deflection saturation filter (e.g. min=-10, max=+10 degrees).
 */
float Algo_saturate(float value, float min, float max);

/**
 * @brief Division with a division-by-zero guard.
 *
 * The denominator's MAGNITUDE is clamped to at least @p min_abs_denominator
 * (sign preserved) before dividing, so the result is always finite. If
 * @p min_abs_denominator <= 0 a tiny internal epsilon is used instead.
 *
 * For dynamic-pressure gain scheduling pass the dynamic pressure q as the
 * denominator and a small POSITIVE q floor as @p min_abs_denominator - that
 * both prevents divide-by-zero and caps the huge deflection you would otherwise
 * get at very low speed (q -> 0 just after launch).
 *
 * @param numerator            Dividend.
 * @param denominator          Divisor (may be 0 or near-0).
 * @param min_abs_denominator  Minimum allowed |denominator|.
 * @return numerator / clamped_denominator, always finite.
 */
float Algo_safeDivide(float numerator, float denominator, float min_abs_denominator);

/**
 * @brief Dynamic pressure  q = 0.5 * rho * v^2   [Pa].
 * @param air_density  rho [kg/m^3].
 * @param velocity     airspeed v [m/s] (uses v^2, so sign does not matter).
 */
float Algo_dynamicPressure(float air_density, float velocity);

/**
 * @brief Air density from the ideal gas law  rho = P / (R_specific * T)  [kg/m^3].
 *        R_specific(dry air) = 287.05 J/(kg*K). Temperature is guarded against
 *        non-physical values (T_kelvin floored at 1 K).
 * @param pressure_pa     static pressure P [Pa].
 * @param temperature_c   temperature T [deg C].
 */
float Algo_airDensity(float pressure_pa, float temperature_c);
