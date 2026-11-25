#ifndef CF_COLLISION_H
#define CF_COLLISION_H
#include <stdint.h>
#ifdef __TARGET_gb
#include "cf_resource.h"
extern const CFResource *cf_collision_resource;
uint8_t cf_collision_at(const uint8_t *,uint16_t cell) NONBANKED;
#else
#define cf_collision_at(data,cell) ((data)[cell])
#endif
#endif
