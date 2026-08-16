#pragma once
#include <stdint.h>

// Command codes for cansat remote control packets (PACKET_CUSTOM_16B).
// High nibble = category, low nibble = subcommand.
// Non-sequential values reduce the risk of noise producing an accidentally valid command.
typedef enum {
    CANSAT_CMD_NOP           = 0x00, // No operation
    CANSAT_CMD_ARM_IGN       = 0x11, // Arm igniters
    CANSAT_CMD_DISARM_IGN    = 0x19, // Disarm igniters
    CANSAT_CMD_ARM_SERVO     = 0x21, // Arm servos
    CANSAT_CMD_DISARM_SERVO  = 0x29, // Disarm servos
    CANSAT_CMD_ARM_FSD       = 0x31, // Arm flight state detector
    CANSAT_CMD_DISARM_FSD    = 0x39, // Disarm flight state detector
    CANSAT_CMD_SET_EFFECTORS = 0x4F, // Apply effector_states to all 32 effectors
    CANSAT_CMD_CANSAT_MANUAL = 0x51, // Set cansat FSM -> MANUAL
    CANSAT_CMD_CANSAT_IDLE   = 0x59, // Set cansat FSM -> IDLE
    CANSAT_CMD_REBOOT        = 0xA5, // System reboot
    CANSAT_CMD_MEM_ERASE     = 0xB5, // Erase flash memory
} cansat_cmd_t;

// Cansat command payload carried in PACKET_CUSTOM_16B.
//
// Receiver validation:
//   1. Verify checksum (CRC16 over bytes [0..13])
//   2. Verify cmd ^ cmd_inv == 0xFF
//   3. If cmd == CANSAT_CMD_SET_EFFECTORS: verify no effector has bit pattern 11 (illegal)
//
// effector_states encoding (2 bits per effector, LSB = effector 0):
//   00 = do nothing   01 = ON   10 = OFF   11 = illegal (reject packet)
typedef struct __attribute__((__packed__)){
    uint8_t  cmd;              // command code (cansat_cmd_t)
    uint8_t  cmd_inv;          // ~cmd — receiver checks cmd ^ cmd_inv == 0xFF
    uint64_t effector_states;  // 2b per effector [0..31], used by CANSAT_CMD_SET_EFFECTORS
    uint8_t  reserved[4];      // reserved for future commands, must be 0x00
    uint16_t checksum;         // CRC16 over bytes [0..13]
} kppacket_payload_cansat_t;  // sizeof == 16
