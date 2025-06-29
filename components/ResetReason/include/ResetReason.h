#ifndef RESET_REASON_H
#define RESET_REASON_H

/**
 * @brief Gets a human-readable string describing the last reset reason.
 *
 * This function checks the cause of the last reset and returns a corresponding
 * descriptive string.
 *
 * @return A constant string describing the reset reason.
 */
const char* get_reset_reason_string(void);

/**
 * @brief Logs the last reset reason to the console.
 *
 * A convenient wrapper function that calls get_reset_reason_string() and
 * prints the result using ESP_LOGI.
 */
void log_reset_reason(void);

#endif // RESET_REASON_H