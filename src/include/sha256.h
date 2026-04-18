/*
 * ============================================================================
 * SHA256.H - SHA-256 Hash Fonksiyonu
 * ============================================================================
 */

#ifndef _SHA256_H
#define _SHA256_H

#include "types.h"

#define SHA256_DIGEST_SIZE 32

typedef struct {
    u32 state[8];
    u32 count[2];
    u8 buffer[64];
} sha256_context;

void sha256_init(sha256_context* ctx);
void sha256_update(sha256_context* ctx, const u8* data, size_t len);
void sha256_final(sha256_context* ctx, u8 digest[SHA256_DIGEST_SIZE]);
void sha256_hash(const u8* data, size_t len, u8 digest[SHA256_DIGEST_SIZE]);

#endif /* _SHA256_H */