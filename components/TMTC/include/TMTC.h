#pragma once

#include "esp_err.h"
#include "PTR_DataPacket.h"

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
 */
void TMTC_send(const kppacket_t *pkt);

/**
 * @brief One iteration of the TMTC loop: polls for incoming packets and dispatches
 *        them by type, then transmits any pending outgoing packet using LBT.
 *        Call repeatedly from the telemetry task while(1).
 */
void TMTC_process(void);
