#ifndef ENCRYPTION_CHACHA_H
#define ENCRYPTION_CHACHA_H

#include <stdint.h>

/*
 * Length-preserving ChaCha20 encryption for LoRa packets (2-250 bytes).
 *
 * Compatible with encryption.c API, extended with packet_number for per-packet nonce.
 *
 * Assumptions:
 *   - Byte 0 of the payload is a 1-byte random value (included in encryption).
 *   - packet_number comes from the plaintext header and is passed to encode/decode.
 *   - Nonce = packet_number + random byte (+ payload length binding).
 *
 * Byte 0 is encrypted with nonce random=0; bytes 1..N-1 use the plaintext random
 * recovered from byte 0 (encode) or decrypted byte 0 (decode).
 */

void Encryption_init(uint64_t key);

void Encryption_encode(uint8_t *v, int size, uint32_t packet_number);

void Encryption_decode(uint8_t *v, int size, uint32_t packet_number);

#endif /* ENCRYPTION_CHACHA_H */
