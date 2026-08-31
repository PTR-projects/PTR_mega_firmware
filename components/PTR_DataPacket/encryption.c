#include <stdint.h>
#include <stddef.h>

// Global variable holding the key from the API
static uint64_t encryption_key = 0;

// Internal function: single XXTEA step for a 64-bit block (2 words)
// Used as a secure keystream generator
static void xxtea_encrypt_block(uint32_t *v, const uint32_t *k) {
    uint32_t v0 = v[0], v1 = v[1], sum = 0;
    uint32_t delta = 0x9E3779B9;
    
    // 32 rounds for a small block guarantee full cryptanalytic security
    for (int i = 0; i < 32; i++) {
        sum += delta;
        v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + k[sum & 3]);
        v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + k[(sum >> 11) & 3]);
    }
    v[0] = v0; v[1] = v1;
}

// Internal function: Bidirectional key-dependent diffusion (avalanche effect)
static void internal_diffuse(uint8_t *data, int size, uint64_t key) {
    if (size < 2) return;
    
    uint8_t k_byte[8];
    for (int i = 0; i < 8; i++) {
        k_byte[i] = (uint8_t)(key >> (i * 8));
    }

    // Forward pass: a change to any byte avalanches to the end of the buffer
    for (int i = 1; i < size; i++) {
        data[i] ^= data[i - 1] + k_byte[i % 8] + (uint8_t)i;
    }
    // Backward pass: a change from the end avalanches back to the start of the buffer
    for (int i = size - 1; i > 0; i--) {
        data[i - 1] ^= data[i] + k_byte[(i + 3) % 8] + (uint8_t)i;
    }
}

// Internal function: Reverse diffusion for decryption
static void internal_undiffuse(uint8_t *data, int size, uint64_t key) {
    if (size < 2) return;

    uint8_t k_byte[8];
    for (int i = 0; i < 8; i++) {
        k_byte[i] = (uint8_t)(key >> (i * 8));
    }

    for (int i = 0; i < size - 1; i++) {
        data[i] ^= data[i + 1] + k_byte[(i + 1 + 3) % 8] + (uint8_t)(i + 1);
    }
    for (int i = size - 1; i > 0; i--) {
        data[i] ^= data[i - 1] + k_byte[i % 8] + (uint8_t)i;
    }
}

// Internal function: Stream cipher (XOR) without changing packet length
static void internal_xor_stream(uint8_t *data, int size, uint64_t key) {
    // Prepare a 128-bit key for XXTEA (64 bits from API + 64 bits of constants)
    uint32_t xxtea_key[4];
    xxtea_key[0] = (uint32_t)(key & 0xFFFFFFFF);
    xxtea_key[1] = (uint32_t)(key >> 32);
    xxtea_key[2] = 0x1337C0DE; // Constant securing the full 128-bit key
    xxtea_key[3] = 0xDEADBEEF;

    int bytes_processed = 0;
    uint64_t block_counter = 0;

    while (bytes_processed < size) {
        uint32_t cipher_block[2];
        
        // Build a unique input block from the counter (internal IV)
        cipher_block[0] = (uint32_t)(block_counter & 0xFFFFFFFF);
        cipher_block[1] = (uint32_t)(block_counter >> 32);

        // Generate 8 secure keystream bytes
        xxtea_encrypt_block(cipher_block, xxtea_key);
        uint8_t *keystream = (uint8_t*)cipher_block;

        // XOR the stream onto data of any length
        for (int i = 0; i < 8 && bytes_processed < size; i++) {
            data[bytes_processed] ^= keystream[i];
            bytes_processed++;
        }
        block_counter++;
    }
}

// --- PUBLIC API ---

void Encryption_init(uint64_t key) {
    encryption_key = key;
}

void Encryption_encode(uint8_t *v, int size) {
    if (v == NULL || size <= 0) return;
    
    // 1. Apply diffusion: changing 1 byte immediately changes the whole buffer (avalanche effect)
    internal_diffuse(v, size, encryption_key);
    
    // 2. Apply stream cipher: protects data without changing its length (zero padding)
    internal_xor_stream(v, size, encryption_key);
}

void Encryption_decode(uint8_t *v, int size) {
    if (v == NULL || size <= 0) return;
    
    // 1. Remove the stream cipher mask
    internal_xor_stream(v, size, encryption_key);
    
    // 2. Reverse the diffusion process to restore the original plaintext
    internal_undiffuse(v, size, encryption_key);
}
