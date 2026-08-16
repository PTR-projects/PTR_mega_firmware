#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/temperature_sensor.h"

#include "BOARD_cfg.h"

#include "Analog_driver.h"

static const char* TAG = "Analog";

// Size of the channel->index lookup table. Must cover every channel id used in
// ADC_CHANNELS_LIST: the ESP32-S3 ADC1 exposes channels 0..9, and boards do use
// the upper ones (e.g. VBAT on ADC_CHANNEL_9), so size this from the SoC caps
// rather than hardcoding - a too-small value silently discards those samples.
#define ADC_CHANNEL_MAX  SOC_ADC_MAX_CHANNEL_NUM

// ── Channel layout ────────────────────────────────────────────────────────────
// Channel 0  (index 0) = VBAT   — same order as original ADC_CHANNELS_LIST
// Channels 1..IGN_NUM  (index 1+) = IGN inputs
// All must be on ADC1 (GPIO 32–39); ADC2 cannot use continuous mode.

#define ADC_SAMPLES_PER_CH      128
#define ADC_TOTAL_SAMPLES       (ADC_CHANNELS_NUM * ADC_SAMPLES_PER_CH)

// Total ADC clock rate (across all channels). The ESP32-S3 continuous ADC only
// accepts SOC_ADC_SAMPLE_FREQ_THRES_LOW..SOC_ADC_SAMPLE_FREQ_THRES_HIGH
// (611 Hz..83.333 kHz), so run at the hardware maximum. A full burst of
// ADC_TOTAL_SAMPLES then completes in ADC_TOTAL_SAMPLES / ADC_SAMPLE_FREQ_HZ
// seconds (e.g. ~6 ms for 4 channels × 128 samples).
#define ADC_SAMPLE_FREQ_HZ      SOC_ADC_SAMPLE_FREQ_THRES_HIGH

// ── Calibration scheme selection ──────────────────────────────────────────────
// ESP32 supports curve-fitting calibration; earlier chips only have line-fitting.
// The preprocessor selects whichever is available in your IDF version.
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    #define USE_CALI_CURVE_FIT  1
#else
    #define USE_CALI_CURVE_FIT  0
#endif

// ── Static state ──────────────────────────────────────────────────────────────

static adc_continuous_handle_t  s_adc_handle  = NULL;
static adc_cali_handle_t        s_cali_handle = NULL;
static bool                     s_cali_ok     = false;

static uint32_t ign_det_thr = 50;

// Averaged raw 12-bit results, filled by make_adc_read()
static uint32_t VBAT_RAW       = 0;
static uint32_t IGN_RAW[IGN_NUM] = {0};

// IIR filter state — same coefficients and logic as original
static float    filter_coeff     = 0.6f;
static float    filter_coeff_ign = 0.5f;
static uint32_t voltage_ign[IGN_NUM] = {0};
static uint32_t voltage_vbat = 0;
static float    mcu_temp     = 0.0f;
static uint32_t vbat_mV_raw  = 0;

// Temperature sensor handle (new driver API)
static temperature_sensor_handle_t s_temp_handle = NULL;

// ── Forward declarations ──────────────────────────────────────────────────────

static uint32_t Analog_getIGN(uint32_t ign_num, uint32_t vbat);
static uint32_t Analog_getVBAT(void);
static esp_err_t init_analog(void);
static esp_err_t make_adc_read(void);

// ── Calibration helpers ───────────────────────────────────────────────────────

static esp_err_t cali_init(void)
{
    esp_err_t err = ESP_FAIL;

#if USE_CALI_CURVE_FIT
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id  = ADC_UNIT_1,
        .atten    = ADC_ATTEN_DB_2_5,
        .bitwidth = ADC_BITWIDTH_12,
    };
    err = adc_cali_create_scheme_curve_fitting(&cali_cfg, &s_cali_handle);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Calibration: curve-fitting scheme");
    }
#else
    adc_cali_line_fitting_config_t cali_cfg = {
        .unit_id       = ADC_UNIT_1,
        .atten         = ADC_ATTEN_DB_2_5,
        .bitwidth      = ADC_BITWIDTH_12,
        .default_vref  = 1100,
    };
    err = adc_cali_create_scheme_line_fitting(&cali_cfg, &s_cali_handle);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Calibration: line-fitting scheme");
    }
#endif

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Calibration not available (%s) — using raw values", esp_err_to_name(err));
        s_cali_handle = NULL;
    }
    return err;
}

// Convert a 12-bit raw value to millivolts, using calibration if available.
static uint32_t raw_to_mv(uint32_t raw)
{
    if (s_cali_ok && s_cali_handle != NULL) {
        int mv = 0;
        esp_err_t err = adc_cali_raw_to_voltage(s_cali_handle, (int)raw, &mv);
        if (err == ESP_OK) return (uint32_t)mv;
    }
    // Fallback: linear approximation for ADC_ATTEN_DB_2_5 (0–1250 mV range)
    return (raw * 1250) / 4095;
}

// ── Public API ────────────────────────────────────────────────────────────────

esp_err_t Analog_init(uint32_t ign_det_thr_val, float filter)
{
    ign_det_thr  = ign_det_thr_val;
    filter_coeff = filter;

    // Calibration
    s_cali_ok = (cali_init() == ESP_OK);

    // ADC continuous + temperature sensor
    esp_err_t err = init_analog();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ADC init failed: %s", esp_err_to_name(err));
        return ESP_FAIL;
    }

    return ESP_OK;
}

static uint32_t Analog_getIGN(uint32_t ign_num, uint32_t vbat)
{
    uint32_t voltage = raw_to_mv(IGN_RAW[ign_num]);
    ESP_LOGV(TAG, "IGN%lu voltage: %lumV", ign_num, voltage);

    voltage_ign[ign_num] = (uint32_t)(filter_coeff_ign * voltage
                           + (1.0f - filter_coeff_ign) * voltage_ign[ign_num]);
    return voltage_ign[ign_num];
}

static uint32_t Analog_getVBAT(void)
{
    uint32_t voltage = raw_to_mv(VBAT_RAW);
    vbat_mV_raw = (uint32_t)(voltage * 11.0f * 1.024f);   // resistor divider correction

    voltage_vbat = (uint32_t)(filter_coeff * vbat_mV_raw
                   + (1.0f - filter_coeff) * voltage_vbat);
    return voltage_vbat;
}

float Analog_getTempMCU(void)
{
    float result = 0.0f;
    if (s_temp_handle != NULL) {
        temperature_sensor_get_celsius(s_temp_handle, &result);
    }
    mcu_temp = filter_coeff * result + (1.0f - filter_coeff) * mcu_temp;
    return mcu_temp;
}

void Analog_update(Analog_meas_t *meas)
{
    // Trigger DMA burst: 128 samples × ADC_CHANNELS_NUM channels
    if (make_adc_read() != ESP_OK) {
        ESP_LOGE(TAG, "ADC burst read failed");
        return;
    }

    meas->vbat_mV = Analog_getVBAT();

    if (vbat_mV_raw > 3200) {
        ign_det_thr = (meas->vbat_mV * 12 - 12619) / 1000;
        for (uint8_t i = 0; i < IGN_NUM; i++) {
            meas->IGN_det[i] = (Analog_getIGN(i, vbat_mV_raw) < ign_det_thr);
        }
    } else if (meas->vbat_mV < 3200) {
        for (uint8_t i = 0; i < IGN_NUM; i++) {
            meas->IGN_det[i] = -1;
        }
    }

    meas->temp = Analog_getTempMCU();
}

int8_t Analog_getIGNstate(Analog_meas_t *meas, uint8_t ign_no)
{
    if (ign_no >= IGN_NUM) return -1;
    if (meas == NULL)      return -1;
    return meas->IGN_det[ign_no];
}

// ── Private: init ─────────────────────────────────────────────────────────────

static esp_err_t init_analog(void)
{
    // ── Continuous ADC handle ─────────────────────────────────────────────────
    const uint32_t frame_bytes = ADC_TOTAL_SAMPLES * SOC_ADC_DIGI_RESULT_BYTES;

    adc_continuous_handle_cfg_t handle_cfg = {
        .max_store_buf_size = frame_bytes * 2,  // 2× so DMA never stalls
        .conv_frame_size    = frame_bytes,       // one burst = exactly our samples
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&handle_cfg, &s_adc_handle));

    // ── Channel pattern — must match ADC_CHANNELS_LIST order ─────────────────
    // Index 0 = VBAT channel, indices 1..IGN_NUM = ignition channels.
    // Edit channel numbers here to match your board's ADC_CHANNELS_LIST macro.
    adc_digi_pattern_config_t pattern[ADC_CHANNELS_NUM];
    uint32_t ch_list[ADC_CHANNELS_NUM] = {ADC_CHANNELS_LIST};

    for (uint8_t i = 0; i < ADC_CHANNELS_NUM; i++) {
        pattern[i].atten     = ADC_ATTEN_DB_2_5;
        pattern[i].channel   = (adc_channel_t)ch_list[i];
        pattern[i].unit      = ADC_UNIT_1;
        pattern[i].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;
    }

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = ADC_SAMPLE_FREQ_HZ,
        .conv_mode      = ADC_CONV_SINGLE_UNIT_1,
        .format         = ADC_DIGI_OUTPUT_FORMAT_TYPE2,  // embeds channel_id per result
        .pattern_num    = ADC_CHANNELS_NUM,
        .adc_pattern    = pattern,
    };
    ESP_ERROR_CHECK(adc_continuous_config(s_adc_handle, &dig_cfg));

    // ── Temperature sensor ────────────────────────────────────────────────────
    temperature_sensor_config_t temp_cfg = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);
    esp_err_t err = temperature_sensor_install(&temp_cfg, &s_temp_handle);
    if (err == ESP_OK) {
        temperature_sensor_enable(s_temp_handle);
        ESP_LOGI(TAG, "Temperature sensor OK");
    } else {
        ESP_LOGW(TAG, "Temperature sensor init failed: %s", esp_err_to_name(err));
        s_temp_handle = NULL;
    }

    ESP_LOGI(TAG, "ADC continuous init OK — %d ch × %d samples @ %lu Hz",
             ADC_CHANNELS_NUM, ADC_SAMPLES_PER_CH, (uint32_t)ADC_SAMPLE_FREQ_HZ);
    return ESP_OK;
}

// ── Private: DMA burst read ───────────────────────────────────────────────────
//
// Pattern:
//   1. Start continuous conversion
//   2. vTaskDelay(1 ms) — hardware fills DMA, CPU yields
//   3. Poll adc_continuous_read() with zero timeout
//   4. If not ready yet, delay 1 ms more and retry
//   5. Stop conversion, accumulate averages into VBAT_RAW / IGN_RAW[]

static esp_err_t make_adc_read(void)
{
    // Static buffer — avoids putting ~1.5 KB on the task stack every call
    static uint8_t s_raw_buf[ADC_TOTAL_SAMPLES * SOC_ADC_DIGI_RESULT_BYTES];

    const uint32_t frame_bytes = ADC_TOTAL_SAMPLES * SOC_ADC_DIGI_RESULT_BYTES;
    const uint32_t timeout_ms  = 50;
    const TickType_t deadline  = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);

    // ── 1. Start ──────────────────────────────────────────────────────────────
    esp_err_t err = adc_continuous_start(s_adc_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "adc_continuous_start failed: %s", esp_err_to_name(err));
        return err;
    }

    // ── 2 & 3. Delay → poll loop ──────────────────────────────────────────────
    uint32_t bytes_read = 0;
    bool     got_frame  = false;

    while (xTaskGetTickCount() < deadline) {
        vTaskDelay(pdMS_TO_TICKS(1));   // yield; DMA keeps filling in background

        bytes_read = 0;
        err = adc_continuous_read(s_adc_handle, s_raw_buf, frame_bytes,
                                  &bytes_read, 0 /* non-blocking */);

        if (err == ESP_OK && bytes_read == frame_bytes) {
            got_frame = true;
            break;
        }
        // ESP_ERR_TIMEOUT = buffer not full yet — loop again
    }

    // ── 4. Stop — always, even on timeout ────────────────────────────────────
    adc_continuous_stop(s_adc_handle);

    if (!got_frame) {
        ESP_LOGE(TAG, "ADC burst timed out (got %lu / %lu bytes)", bytes_read, frame_bytes);
        return ESP_ERR_TIMEOUT;
    }

    // ── 5. Accumulate per-channel sums ───────────────────────────────────────
    uint64_t acc[ADC_CHANNELS_NUM]   = {0};
    uint32_t cnt[ADC_CHANNELS_NUM]   = {0};
    uint32_t ch_list[ADC_CHANNELS_NUM] = {ADC_CHANNELS_LIST};

    // Build a fast channel→index lookup (sparse array indexed by adc_channel_t)
    uint8_t ch_to_idx[ADC_CHANNEL_MAX];
    memset(ch_to_idx, 0xFF, sizeof(ch_to_idx));
    for (uint8_t i = 0; i < ADC_CHANNELS_NUM; i++) {
        if (ch_list[i] < ADC_CHANNEL_MAX) {
            ch_to_idx[ch_list[i]] = i;
        }
    }

    const uint32_t num_results = bytes_read / SOC_ADC_DIGI_RESULT_BYTES;
    for (uint32_t i = 0; i < num_results; i++) {
        adc_digi_output_data_t *p =
            (adc_digi_output_data_t *)&s_raw_buf[i * SOC_ADC_DIGI_RESULT_BYTES];

        uint8_t  chan = p->type2.channel;
        uint16_t val  = p->type2.data;

        if (chan < ADC_CHANNEL_MAX && ch_to_idx[chan] != 0xFF) {
            uint8_t idx = ch_to_idx[chan];
            acc[idx] += val;
            cnt[idx]++;
        }
    }

    // ── 6. Write averaged results → module-level raw variables ───────────────
    VBAT_RAW = (cnt[0] > 0) ? (uint32_t)(acc[0] / cnt[0]) : 0;

    for (uint8_t i = 0; i < IGN_NUM; i++) {
        uint8_t idx = i + 1;    // IGN channels start at index 1
        IGN_RAW[i] = (cnt[idx] > 0) ? (uint32_t)(acc[idx] / cnt[idx]) : 0;
    }

    ESP_LOGD(TAG, "burst done — VBAT_RAW=%lu  (n=%lu)", VBAT_RAW, cnt[0]);
    return ESP_OK;
}