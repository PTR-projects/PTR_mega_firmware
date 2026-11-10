#define CONFIG_MAIN_LOOP_FREQUENCY 100     // Note - for rate > 100Hz change MS5607 settings
#define CONFIG_TELEMETRY_FREQUENCY 1
#define CONFIG_TELEMETRY_FREQUENCY_FLIGHT 2

/* LoRa modulation. Change these to retune a link without editing the driver. */
#define CONFIG_LORA_TLM_SF  7
#define CONFIG_LORA_TLM_BW  125
#define CONFIG_LORA_TRK_SF  8
#define CONFIG_LORA_TRK_BW  125

#define CONFIG_LORA_TLM_PERIOD_MS  1000
#define CONFIG_LORA_TRK_PERIOD_MS  2000
#define CONFIG_LORA_TRK_OFFSET_MS   500