#pragma once

#include "esp_err.h"
#include "PTR_DataPacket.h"
#include "LORA_driver.h"

/**
 * @brief Initializes the TMTC layer: creates internal TX queue and enters RX mode.
 *        Call once after LORA_init() succeeds.
 * @return ESP_OK on success.
 */
esp_err_t TMTC_init(void);

/**
 * @brief Queues a packet for transmission. Overwrites any previously queued packet.
 *        Non-blocking. Safe to call from any task.
 * @param[in] pkt Pointer to the packet to send.
 * @param[in] tx Frequency and modulation for this packet.
 * @param[in] rx_after Settings restored for receive after the transmit.
 */
void TMTC_send(const kppacket_t *pkt, const lora_phy_t *tx, const lora_phy_t *rx_after);

/**
 * @brief Queues a rocket telemetry packet. Transmit and receive stay on the
 *        telemetry frequency and modulation.
 */
void TMTC_sendTelemetry(const kppacket_t *pkt);

/**
 * @brief Queues a tracker packet on the tracker frequency and modulation,
 *        then returns the radio to the telemetry settings for receive.
 */
void TMTC_sendTracker(const kppacket_t *pkt);

/**
 * @brief One iteration of the TMTC loop: polls for incoming packets and dispatches
 *        them by type, then transmits any pending outgoing packet using LBT.
 *        Call repeatedly from the telemetry task while(1).
 */
void TMTC_process(void);
