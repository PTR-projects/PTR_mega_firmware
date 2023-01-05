#pragma once

/**
 * @brief Enumeration of the system manager checkout components
 *
 * This enumeration defines the different system manager checkout components
 * that can be checked by the system manager.
 */
typedef enum{
	checkout_sysmgr,   ///< System manager checkout
	checkout_main,     ///< Main checkout
	checkout_storage,  ///< Storage checkout
	checkout_lora,     ///< LoRa checkout
	checkout_analog,   ///< Analog checkout
	checkout_utils     ///< Utilities checkout
} sysmgr_checkout_component_t;

/**
 * @brief Enumeration of the possible checkout states
 *
 * This enumeration defines the possible states for a checkout.
 */
typedef enum{
	check_ready = 0x01, ///< Checkout is ready
	check_void  = 0x02, ///< Checkout is not implemented
	check_fail  = 0x04  ///< Checkout has failed
} sysmgr_checkout_state_t;

/**
 * @brief Structure for a checkout message
 *
 * This structure is used to represent a checkout message sent by a component
 * to the system manager.
 */
typedef struct{
	sysmgr_checkout_component_t component; ///< Component that sent the message
	sysmgr_checkout_state_t     state;     ///< Checkout state of the component
} sysmgr_checkout_msg_t;

/**
 * @brief Union for representing the checkout status of all components
 *
 * This union is used to represent the checkout status of all components.
 * The status can be accessed either as a table of states indexed by
 * `sysmgr_checkout_component_t` values, or as individual fields for each
 * component.
 */
typedef union{
	struct{
		sysmgr_checkout_state_t sysmgr;   ///< System manager checkout status
		sysmgr_checkout_state_t main;     ///< Main checkout status
		sysmgr_checkout_state_t storage;  ///< Storage checkout status
		sysmgr_checkout_state_t lora;     ///< LoRa checkout status
		sysmgr_checkout_state_t analog;   ///< Analog checkout status
		sysmgr_checkout_state_t utils;    ///< Utilities checkout status
	};
	sysmgr_checkout_state_t table[6]; ///< Table of checkout statuses indexed by `sysmgr_checkout_component_t`
}sysmgr_checkout_status_t;

/**
 * @brief Initialize the system manager
 *
 * This function initializes the system manager, including creating the checkout
 * queue and initializing the checkout status for all components.
 *
 * @return ESP_OK if initialization was successful, ESP_FAIL otherwise
 */
esp_err_t SysMgr_init();

/**
 * @brief Sends a checkout message to the system manager
 *
 * This function sends a checkout message to the system manager, indicating the
 * checkout state of the specified component. This function should be called by
 * each component in the system when it is ready to be checked out.
 *
 * @param[in] component The component that is sending the checkout message
 * @param[in] state The checkout state of the component
 *
 * @return
 * - ESP_OK if the checkout message was successfully sent
 * - ESP_FAIL if there was an error sending the checkout message
 *
 * @usage
 * To send a checkout message to the system manager, a component can call
 * `SysMgr_checkout()` as follows:
 *
 * @code
 * esp_err_t res = SysMgr_checkout(checkout_main, check_ready);
 * if(res != ESP_OK){
 *     // Handle error
 * }
 * @endcode
 */
esp_err_t SysMgr_checkout(sysmgr_checkout_component_t component, sysmgr_checkout_state_t state);

/**
 * @brief Updates the system manager by checking for checkout messages in the queue.
 *
 * @return ESP_OK if successful, ESP_FAIL otherwise.
 *
 * @note This function must be called regularly in order for the system manager to function properly.
 *
 * @code
 * esp_err_t err = SysMgr_update();
 * if (err != ESP_OK) {
 *     // Handle error
 * }
 * @endcode
 */
esp_err_t SysMgr_update();

/**
 * @brief Gets the current checkout status of the system manager.
 *
 * This function retrieves the current checkout status of the system manager by checking the
 * checkout status table. The function checks each component in the table and calculates a
 * checkout status based on the states of the components.
 *
 * @return A value from the sysmgr_checkout_state_t enum indicating the checkout status.
 *
 * Example:
 *
 * @code
 *
 * sysmgr_checkout_state_t status = SysMgr_getCheckoutStatus();
 *
 * if (status == check_fail) {
 *     // Handle checkout failure
 * } else if (status == check_void) {
 *     // Handle checkout void
 * } else if (status == check_ready) {
 *     // Handle checkout ready
 * }
 * @endcode
 */
sysmgr_checkout_state_t SysMgr_getCheckoutStatus();
