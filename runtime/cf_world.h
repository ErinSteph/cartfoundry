#ifndef CF_WORLD_H
#define CF_WORLD_H
#include <stdint.h>
typedef struct { int16_t x, y, camera_x, camera_y; int8_t facing_x, facing_y; } CFWorldState;
#ifdef __cplusplus
extern "C" {
#endif
void cf_world_camera(CFWorldState *s, uint8_t width, uint8_t height);
uint8_t cf_world_move(CFWorldState *s, uint8_t width, uint8_t height, const uint8_t *solid, int8_t dx, int8_t dy);
int16_t cf_world_facing_cell(const CFWorldState *s, uint8_t width, uint8_t height);
#ifdef __cplusplus
}
#endif
#endif
