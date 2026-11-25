#include "cf_collision.h"
#include "cf_world.h"
void cf_world_camera(CFWorldState *s, uint8_t width, uint8_t height) {
    int16_t max_x = (int16_t)width * 8 - 160, max_y = (int16_t)height * 8 - 144;
    s->camera_x = s->x + 4 - 80; s->camera_y = s->y + 4 - 72;
    if (s->camera_x < 0) s->camera_x = 0;
    if (s->camera_y < 0) s->camera_y = 0;
    if (s->camera_x > max_x) s->camera_x = max_x > 0 ? max_x : 0;
    if (s->camera_y > max_y) s->camera_y = max_y > 0 ? max_y : 0;
}
static uint8_t passable(int16_t x, int16_t y, uint8_t w, uint8_t h, const uint8_t *solid) {
    uint8_t left, right, top, bottom;
    if (x < 0 || y < 0 || x + 7 >= (int16_t)w * 8 || y + 7 >= (int16_t)h * 8) return 0;
    left = x >> 3; right = (x + 7) >> 3; top = y >> 3; bottom = (y + 7) >> 3;
    return !(cf_collision_at(solid,(uint16_t)top * w + left) || cf_collision_at(solid,(uint16_t)top * w + right) || cf_collision_at(solid,(uint16_t)bottom * w + left) || cf_collision_at(solid,(uint16_t)bottom * w + right));
}
uint8_t cf_world_move(CFWorldState *s, uint8_t w, uint8_t h, const uint8_t *solid, int8_t dx, int8_t dy) {
    uint8_t moved = 0;
    if (dx < -1 || dx > 1 || dy < -1 || dy > 1) return 0;
    if (dx) { s->facing_x = dx; s->facing_y = 0; }
    else if (dy) { s->facing_x = 0; s->facing_y = dy; }
    if (dx && passable(s->x + dx, s->y, w, h, solid)) { s->x += dx; moved = 1; }
    if (!dx && dy && passable(s->x, s->y + dy, w, h, solid)) { s->y += dy; moved = 1; }
    cf_world_camera(s, w, h); return moved;
}
int16_t cf_world_facing_cell(const CFWorldState *s, uint8_t w, uint8_t h) {
    int16_t x = (s->x + 4) / 8 + s->facing_x, y = (s->y + 4) / 8 + s->facing_y;
    if (x < 0 || y < 0 || x >= w || y >= h) return -1;
    return y * w + x;
}
