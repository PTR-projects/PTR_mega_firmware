#include "encryption_chacha.h"

#include <stddef.h>
#include <string.h>

static uint64_t encryption_key = 0;

/* --- ChaCha20 (RFC 8439) --- */

static uint32_t rotl32(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

static void chacha20_quarter_round(uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d) {
    *a += *b; *d ^= *a; *d = rotl32(*d, 16);
    *c += *d; *b ^= *c; *b = rotl32(*b, 12);
    *a += *b; *d ^= *a; *d = rotl32(*d, 8);
    *c += *d; *b ^= *c; *b = rotl32(*b, 7);
}

static void chacha20_block(const uint8_t key[32], const uint8_t nonce[12],
                             uint32_t counter, uint8_t out[64]) {
    static const uint32_t constants[4] = {
        0x61707865U, 0x3320646eU, 0x79622d32U, 0x6b206574U
    };

    uint32_t state[16];
    state[0] = constants[0];
    state[1] = constants[1];
    state[2] = constants[2];
    state[3] = constants[3];

    for (int i = 0; i < 8; i++) {
        state[4 + i] = ((uint32_t)key[i * 4 + 0])
                     | ((uint32_t)key[i * 4 + 1] << 8)
                     | ((uint32_t)key[i * 4 + 2] << 16)
                     | ((uint32_t)key[i * 4 + 3] << 24);
    }

    state[12] = counter;
    state[13] = ((uint32_t)nonce[0])
              | ((uint32_t)nonce[1] << 8)
              | ((uint32_t)nonce[2] << 16)
              | ((uint32_t)nonce[3] << 24);
    state[14] = ((uint32_t)nonce[4])
              | ((uint32_t)nonce[5] << 8)
              | ((uint32_t)nonce[6] << 16)
              | ((uint32_t)nonce[7] << 24);
    state[15] = ((uint32_t)nonce[8])
              | ((uint32_t)nonce[9] << 8)
              | ((uint32_t)nonce[10] << 16)
              | ((uint32_t)nonce[11] << 24);

    uint32_t working[16];
    memcpy(working, state, sizeof(working));

    for (int i = 0; i < 10; i++) {
        chacha20_quarter_round(&working[0], &working[4], &working[8],  &working[12]);
        chacha20_quarter_round(&working[1], &working[5], &working[9],  &working[13]);
        chacha20_quarter_round(&working[2], &working[6], &working[10], &working[14]);
        chacha20_quarter_round(&working[3], &working[7], &working[11], &working[15]);
        chacha20_quarter_round(&working[0], &working[5], &working[10], &working[15]);
        chacha20_quarter_round(&working[1], &working[6], &working[11], &working[12]);
        chacha20_quarter_round(&working[2], &working[7], &working[8],  &working[13]);
        chacha20_quarter_round(&working[3], &working[4], &working[9],  &working[14]);
    }

    for (int i = 0; i < 16; i++) {
        working[i] += state[i];
    }

    for (int i = 0; i < 16; i++) {
        out[i * 4 + 0] = (uint8_t)(working[i] >> 0);
        out[i * 4 + 1] = (uint8_t)(working[i] >> 8);
        out[i * 4 + 2] = (uint8_t)(working[i] >> 16);
        out[i * 4 + 3] = (uint8_t)(working[i] >> 24);
    }
}

static void chacha20_xor(uint8_t *data, size_t len, const uint8_t key[32],
                         const uint8_t nonce[12], uint32_t counter) {
    uint8_t block[64];
    size_t offset = 0;

    while (offset < len) {
        chacha20_block(key, nonce, counter, block);
        counter++;

        size_t chunk = len - offset;
        if (chunk > 64) {
            chunk = 64;
        }

        for (size_t i = 0; i < chunk; i++) {
            data[offset + i] ^= block[i];
        }
        offset += chunk;
    }
}

/* Expand 64-bit API key to ChaCha20's 256-bit key (compatible with encryption.c key source). */
static void expand_key(uint64_t key, uint8_t out[32]) {
    static const uint64_t mix[3] = {
        0x6170786512345678ULL,
        0x3320646e99887766ULL,
        0x79622d3276543210ULL
    };

    for (int i = 0; i < 4; i++) {
        uint64_t word = key;
        if (i > 0) {
            word ^= mix[i - 1];
        }
        word = (word * 0x9E3779B97F4A7C15ULL) ^ (word >> 32);
        word ^= (uint64_t)(i + 1) * 0xD6E8FEB86659FD93ULL;

        out[i * 8 + 0] = (uint8_t)(word >> 0);
        out[i * 8 + 1] = (uint8_t)(word >> 8);
        out[i * 8 + 2] = (uint8_t)(word >> 16);
        out[i * 8 + 3] = (uint8_t)(word >> 24);
        out[i * 8 + 4] = (uint8_t)(word >> 32);
        out[i * 8 + 5] = (uint8_t)(word >> 40);
        out[i * 8 + 6] = (uint8_t)(word >> 48);
        out[i * 8 + 7] = (uint8_t)(word >> 56);
    }
}

/*
 * 12-byte ChaCha20 nonce:
 *   [0..3]  packet_number (LE)
 *   [4]     random byte (0 for encrypting/decrypting byte 0 alone)
 *   [5]     payload size (binds keystream to length)
 *   [6..11] domain separation / reserved (zero)
 */
static void build_nonce(uint8_t nonce[12], uint32_t packet_number,
                        uint8_t random_byte, int size) {
    nonce[0] = (uint8_t)(packet_number >> 0);
    nonce[1] = (uint8_t)(packet_number >> 8);
    nonce[2] = (uint8_t)(packet_number >> 16);
    nonce[3] = (uint8_t)(packet_number >> 24);
    nonce[4] = random_byte;
    nonce[5] = (uint8_t)size;
    nonce[6] = 0x4C; /* 'L' LoRa domain separator */
    nonce[7] = 0x52; /* 'R' */
    nonce[8] = 0;
    nonce[9] = 0;
    nonce[10] = 0;
    nonce[11] = 0;
}

static void crypt_tail(uint8_t *v, int size, uint32_t packet_number,
                       const uint8_t key[32], uint8_t random_byte) {
    if (size <= 1) {
        return;
    }

    uint8_t nonce[12];
    build_nonce(nonce, packet_number, random_byte, size);
    chacha20_xor(v + 1, (size_t)(size - 1), key, nonce, 0);
}

static void crypt_first_byte(uint8_t *v, int size, uint32_t packet_number,
                             const uint8_t key[32]) {
    uint8_t nonce[12];
    build_nonce(nonce, packet_number, 0, size);
    chacha20_xor(v, 1, key, nonce, 0);
}

/* --- PUBLIC API --- */

void Encryption_init(uint64_t key) {
    encryption_key = key;
}

void Encryption_encode(uint8_t *v, int size, uint32_t packet_number) {
    uint8_t key[32];

    if (v == NULL || size <= 0) {
        return;
    }

    expand_key(encryption_key, key);

    uint8_t random_plain = v[0];
    crypt_first_byte(v, size, packet_number, key);
    crypt_tail(v, size, packet_number, key, random_plain);
}

void Encryption_decode(uint8_t *v, int size, uint32_t packet_number) {
    uint8_t key[32];

    if (v == NULL || size <= 0) {
        return;
    }

    expand_key(encryption_key, key);

    crypt_first_byte(v, size, packet_number, key);
    crypt_tail(v, size, packet_number, key, v[0]);
}
