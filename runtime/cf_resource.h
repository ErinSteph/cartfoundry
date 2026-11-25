#ifndef CF_RESOURCE_H
#define CF_RESOURCE_H
#include <stdint.h>
#ifdef __TARGET_gb
#include <gb/gb.h>
#define CF_RESOURCE_CALL NONBANKED
#else
#define CF_RESOURCE_CALL
#endif
/* Codec 0 is raw. Codec 1 has a little-endian offset per 64-byte block,
   followed by PackBits packets: high bit=repeat, low seven bits=length-1. */
typedef struct {uint8_t bank,codec;uint16_t size;const uint8_t *data;} CFResource;
#ifdef __cplusplus
extern "C" {
#endif
/* Resources are immutable. Invalidate before replacing their backing memory. */
void cf_resource_invalidate(void) CF_RESOURCE_CALL;
void cf_resource_copy(const CFResource *,uint16_t offset,uint8_t *,uint16_t length) CF_RESOURCE_CALL;
uint8_t cf_resource_read(const CFResource *,uint16_t offset) CF_RESOURCE_CALL;
#ifdef __cplusplus
}
#endif
#endif
