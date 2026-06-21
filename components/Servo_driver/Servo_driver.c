#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "driver/mcpwm_prelude.h"
#include "driver/mcpwm_timer.h"
#include "driver/mcpwm_oper.h"
#include "driver/mcpwm_cmpr.h"
#include "driver/mcpwm_gen.h"
#include "driver/gpio.h"
#include "BOARD_cfg.h"

#include "Servo_driver.h"

static const char *TAG = "Servo_driver";

// ── MCPWM resource limits ─────────────────────────────────────────────────────
// ESP32 has 2 MCPWM groups (0 and 1), each with 3 timers → 6 channels total.
// Channels 0-2 use group 0, channels 3-5 use group 1.

#if BOARD_SERVO_PWM_NUM > 6
#error "Too many PWM signals required. Max MCPWM PWM channels = 6"
#endif

// Timer resolution: 1 MHz → each tick = 1 µs.
// This means compare values map directly to pulse widths in microseconds.
#define MCPWM_TIMER_RESOLUTION_HZ   1000000u

// ── Per-channel handle bundle ─────────────────────────────────────────────────

typedef struct {
    mcpwm_timer_handle_t timer;
    mcpwm_oper_handle_t  oper;
    mcpwm_cmpr_handle_t  cmpr;
    mcpwm_gen_handle_t   gen;
} servo_hw_ch_t;

// ── Module state ──────────────────────────────────────────────────────────────

servo_t         Servo_d;

#if !((BOARD_SERVO_PWM_NUM > 0) && !(BOARD_SERVO_SBUS_NUM > 0))

// ── Stub implementations when PWM servos are not configured ──────────────────

esp_err_t  Servo_init(int min_pulsewidth, int max_pulsewidth, int frequency) { return ESP_FAIL; }
esp_err_t  Servo_enable(void)   { return ESP_FAIL; }
esp_err_t  Servo_disable(void)  { return ESP_FAIL; }
esp_err_t  Servo_drive(int8_t S1_position, int8_t S2_position,
                        int8_t S3_position, int8_t S4_position) { return ESP_FAIL; }
servo_t   *Servo_get(void)      { return &Servo_d; }

#else

// ── Active implementation ─────────────────────────────────────────────────────

static int             SERVO_PINS[BOARD_SERVO_PWM_NUM]       = BOARD_SERVO_PWM_PINS;
static Servo_config_t  Servo_config_d[BOARD_SERVO_PWM_NUM];
static servo_hw_ch_t   s_hw[BOARD_SERVO_PWM_NUM];

static uint32_t angle_to_PWM(float position, Servo_config_t cfg);
static esp_err_t servo_hw_init_channel(int ch_idx);

// ── Init ──────────────────────────────────────────────────────────────────────

/*!
 * @brief Initialize servo component
 * @param min_pulsewidth  Minimum pulse width in microseconds
 * @param max_pulsewidth  Maximum pulse width in microseconds
 * @param frequency       PWM frequency in Hz (typically 50)
 * @return ESP_OK on success
 */
esp_err_t Servo_init(int min_pulsewidth, int max_pulsewidth, int frequency)
{
    memset(s_hw, 0, sizeof(s_hw));

    for (int i = 0; i < BOARD_SERVO_PWM_NUM; i++) {
        Servo_config_d[i].min_pulsewidth_us  = min_pulsewidth;
        Servo_config_d[i].max_pulsewidth_us  = max_pulsewidth;
        Servo_config_d[i].timebase_frequency = frequency;
    }

#if defined(SERVO_EN_PIN)
    gpio_reset_pin(SERVO_EN_PIN);
    gpio_set_direction(SERVO_EN_PIN, GPIO_MODE_INPUT_OUTPUT);
#endif

    for (int i = 0; i < BOARD_SERVO_PWM_NUM; i++) {
        esp_err_t err = servo_hw_init_channel(i);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Channel %d init failed: %s", i, esp_err_to_name(err));
            return err;
        }
    }

    ESP_LOGI(TAG, "%d servo channel(s) init OK", BOARD_SERVO_PWM_NUM);
    return ESP_OK;
}

/*!
 * @brief Allocate and configure MCPWM hardware for one servo channel.
 *
 * Channel mapping (mirrors the old CH_TO_UNIT / CH_TO_TIMER tables):
 *   ch 0-2 → group 0, timers 0-2
 *   ch 3-5 → group 1, timers 0-2
 */
static esp_err_t servo_hw_init_channel(int i)
{
    const int group = (i < 3) ? 0 : 1;
    const int timer = i % 3;

    const uint32_t period_ticks =
        MCPWM_TIMER_RESOLUTION_HZ / (uint32_t)Servo_config_d[i].timebase_frequency;

    // 1. Timer
    mcpwm_timer_config_t timer_cfg = {
        .group_id      = group,
        .clk_src       = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = MCPWM_TIMER_RESOLUTION_HZ,
        .period_ticks  = period_ticks,
        .count_mode    = MCPWM_TIMER_COUNT_MODE_UP,
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_cfg, &s_hw[i].timer));

    // 2. Operator — bound to the same group as the timer
    mcpwm_operator_config_t oper_cfg = { .group_id = group };
    ESP_ERROR_CHECK(mcpwm_new_operator(&oper_cfg, &s_hw[i].oper));
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(s_hw[i].oper, s_hw[i].timer));

    // 3. Comparator — update on timer-zero so glitch-free
    mcpwm_comparator_config_t cmpr_cfg = {
        .flags.update_cmp_on_tez = true,
    };
    ESP_ERROR_CHECK(mcpwm_new_comparator(s_hw[i].oper, &cmpr_cfg, &s_hw[i].cmpr));

    // 4. Generator — drives the servo GPIO
    mcpwm_generator_config_t gen_cfg = { .gen_gpio_num = SERVO_PINS[i] };
    ESP_ERROR_CHECK(mcpwm_new_generator(s_hw[i].oper, &gen_cfg, &s_hw[i].gen));

    // Go HIGH at timer zero, go LOW when comparator fires → standard PWM pulse
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(s_hw[i].gen,
        MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                     MCPWM_TIMER_EVENT_EMPTY,
                                     MCPWM_GEN_ACTION_HIGH)));

    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(s_hw[i].gen,
        MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                       s_hw[i].cmpr,
                                       MCPWM_GEN_ACTION_LOW)));

    // Set centre position before enabling the output
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(
        s_hw[i].cmpr, angle_to_PWM(0, Servo_config_d[i])));

    // 5. Enable and run continuously
    ESP_ERROR_CHECK(mcpwm_timer_enable(s_hw[i].timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(s_hw[i].timer, MCPWM_TIMER_START_NO_STOP));

    ESP_LOGD(TAG, "ch%d → group%d timer%d GPIO%d  period=%lu ticks",
             i, group, timer, SERVO_PINS[i], period_ticks);

    return ESP_OK;
}

// ── Enable / disable ──────────────────────────────────────────────────────────

/*!
 * @brief Assert SERVO_EN pin (enable servo power stage)
 */
esp_err_t Servo_enable(void)
{
#if defined(SERVO_EN_PIN)
    gpio_set_level(SERVO_EN_PIN, 1);
    Servo_d.servo_en = 1;
    return ESP_OK;
#else
    return ESP_FAIL;
#endif
}

/*!
 * @brief De-assert SERVO_EN pin (disable servo power stage)
 */
esp_err_t Servo_disable(void)
{
#if defined(SERVO_EN_PIN)
    gpio_set_level(SERVO_EN_PIN, 0);
    Servo_d.servo_en = 0;
    return ESP_OK;
#else
    return ESP_FAIL;
#endif
}

// ── Per-channel drive ─────────────────────────────────────────────────────────

/*!
 * @brief Drive a single servo channel to a position.
 * @param servo_num  1-based channel index (1 = first servo)
 * @param position   Position in range -100 to +100
 */
esp_err_t Servo_driveSinglePWM(uint8_t servo_num, int8_t position)
{
#if (BOARD_SERVO_PWM_NUM > 0)
    if (servo_num == 0 || servo_num > BOARD_SERVO_PWM_NUM) {
        return ESP_FAIL;
    }

    uint8_t idx = servo_num - 1;   // convert to 0-based
    return mcpwm_comparator_set_compare_value(
        s_hw[idx].cmpr, angle_to_PWM(position, Servo_config_d[idx]));
#endif
    return ESP_FAIL;
}

/*!
 * @brief Drive up to 4 servos simultaneously.
 * @param S1..S4_position  Position -100..+100 for each channel
 */
esp_err_t Servo_drive(int8_t S1_position, int8_t S2_position,
                       int8_t S3_position, int8_t S4_position)
{
    Servo_driveSinglePWM(1, S1_position);
    Servo_driveSinglePWM(2, S2_position);
    Servo_driveSinglePWM(3, S3_position);
    Servo_driveSinglePWM(4, S4_position);

    Servo_d.S1_pos = S1_position;
    Servo_d.S2_pos = S2_position;
    Servo_d.S3_pos = S3_position;
    Servo_d.S4_pos = S4_position;

    return ESP_OK;
}

// ── Per-channel config ────────────────────────────────────────────────────────

/*!
 * @brief Reconfigure pulse range and frequency for one channel.
 *
 * Note: changing frequency after init requires re-initialising the timer.
 * If only pulse widths change, the new values take effect on the next
 * Servo_driveSinglePWM() call without touching hardware.
 *
 * @param servo_num  1-based channel index
 */
esp_err_t Servo_configSingle(uint8_t servo_num, int min_pulsewidth,
                              int max_pulsewidth, int frequency)
{
#if (BOARD_SERVO_PWM_NUM > 0)
    if (servo_num == 0 || servo_num > BOARD_SERVO_PWM_NUM) {
        return ESP_FAIL;
    }

    uint8_t idx = servo_num - 1;
    Servo_config_d[idx].min_pulsewidth_us  = min_pulsewidth;
    Servo_config_d[idx].max_pulsewidth_us  = max_pulsewidth;
    Servo_config_d[idx].timebase_frequency = frequency;
    return ESP_OK;
#endif
    return ESP_FAIL;
}

// ── State accessor ────────────────────────────────────────────────────────────

servo_t *Servo_get(void)
{
    return &Servo_d;
}

// ── Internal helpers ──────────────────────────────────────────────────────────

/*!
 * @brief Map position (-100..+100) to a pulse width in microseconds.
 *
 * With MCPWM_TIMER_RESOLUTION_HZ = 1 MHz, 1 tick = 1 µs, so the returned
 * value is passed directly to mcpwm_comparator_set_compare_value().
 */
static uint32_t angle_to_PWM(float position, Servo_config_t cfg)
{
    return (uint32_t)((position + 100.0f)
                      * (cfg.max_pulsewidth_us - cfg.min_pulsewidth_us)
                      / 200.0f
                      + cfg.min_pulsewidth_us);
}

#endif  // BOARD_SERVO_PWM_NUM > 0 && !BOARD_SERVO_SBUS_NUM