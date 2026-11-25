#include "cf_resource.h"
#ifdef __TARGET_gb
#include "cf_bank.h"
#pragma codeseg HOME
#endif

static const CFResource *cached_resource;
static uint8_t cache_valid;
static uint16_t cached_block;
static uint8_t cache[64];

void cf_resource_invalidate(void) CF_RESOURCE_CALL { cache_valid = 0; }

/* Only this resident function maps resource ROM. No caller can execute from
   an unmapped bank, and the interrupt state and caller's bank are restored. */
static void block_read(const CFResource *r, uint16_t block, uint8_t *out) CF_RESOURCE_CALL {
    uint16_t length = r->size - block * 64, offset;
    uint8_t token, n, i = 0;
    const uint8_t *p;
#ifdef __TARGET_gb
    uint8_t saved;
#endif
    if (length > 64) length = 64;
#ifdef __TARGET_gb
    CRITICAL {
        saved = CURRENT_BANK;
        SWITCH_ROM(r->bank);
#endif
        if (!r->codec) {
            p = r->data + block * 64;
            while (i < length) out[i++] = *p++;
        } else {
            p = r->data + block * 2;
            offset = p[0] | ((uint16_t)p[1] << 8);
            p = r->data + offset;
            while (i < length) {
                token = *p++;
                n = (token & 127) + 1;
                if (n > length - i) n = length - i;
                if (token & 128) {
                    token = *p++;
                    while (n--) out[i++] = token;
                } else {
                    while (n--) out[i++] = *p++;
                }
            }
        }
#ifdef __TARGET_gb
        SWITCH_ROM(saved);
    }
#endif
}

static void ensure_block(const CFResource *r, uint16_t block) CF_RESOURCE_CALL {
    if (!cache_valid || cached_resource != r || cached_block != block) {
        block_read(r, block, cache);
        cached_resource = r;
        cached_block = block;
        cache_valid = 1;
    }
}

void cf_resource_copy(const CFResource *r, uint16_t offset, uint8_t *out, uint16_t length) CF_RESOURCE_CALL {
    uint8_t chunk, start, i;
    if (offset >= r->size) return;
    if (length > r->size - offset) length = r->size - offset;
    while (length) {
        start = offset & 63;
        chunk = 64 - start;
        if (chunk > length) chunk = length;
        ensure_block(r, offset / 64);
        for (i = 0; i < chunk; ++i) *out++ = cache[start + i];
        offset += chunk;
        length -= chunk;
    }
}

uint8_t cf_resource_read(const CFResource *r, uint16_t offset) CF_RESOURCE_CALL {
    if (offset >= r->size) return 0;
    ensure_block(r, offset / 64);
    return cache[offset & 63];
}
