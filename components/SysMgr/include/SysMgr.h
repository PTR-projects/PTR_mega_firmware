#pragma once

typedef enum{
	checkout_sysmgr,
	checkout_main,
	checkout_storage,
	checkout_lora,
	checkout_analog,
	checkout_utils
} sysmgr_checkout_component_t;

typedef enum{
	check_ready = 0x01,
	check_void  = 0x02,
	check_fail  = 0x04
} sysmgr_checkout_state_t;

typedef enum{
	system_arming_error,
	system_armed,
	system_dissarmed
} sysmgr_arming_state_t;

typedef struct{
	sysmgr_checkout_component_t component;
	sysmgr_checkout_state_t		state;
} sysmgr_checkout_msg_t;

typedef union{
	struct{
		sysmgr_checkout_state_t sysmgr;
		sysmgr_checkout_state_t main;
		sysmgr_checkout_state_t storage;
		sysmgr_checkout_state_t lora;
		sysmgr_checkout_state_t analog;
		sysmgr_checkout_state_t utils;
	};
	sysmgr_checkout_state_t table[6];
}sysmgr_checkout_status_t;

esp_err_t SysMgr_init();
esp_err_t SysMgr_checkout(sysmgr_checkout_component_t component, sysmgr_checkout_state_t state);
esp_err_t SysMgr_update();
sysmgr_checkout_state_t SysMgr_getCheckoutStatus();
esp_err_t SysMgr_setArm(sysmgr_arming_state_t state);
sysmgr_arming_state_t SysMgr_getArm();
