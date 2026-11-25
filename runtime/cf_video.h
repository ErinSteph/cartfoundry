#ifndef CF_VIDEO_H
#define CF_VIDEO_H
#include "cf_scene.h"
#include "cf_flow.h"
#include "cf_world.h"
/* Resident hardware composition primitives. No application data is stored in
   this module's code bank; the caller supplies current world/scene state. */
typedef struct {
 CFGameState *game;CFWorldState *world;const CFResource *map;
 uint8_t *bitmap,*screen;uint8_t width,height,is_world,has_image,color;
 uint8_t origin_x,origin_y,palette;uint16_t render_map;
} CFVideo;
void cf_video_tile_at(uint16_t,uint8_t,uint8_t,uint8_t,uint8_t) NONBANKED;
uint16_t cf_video_hidden_map(void) NONBANKED;
void cf_video_show_map(uint16_t,uint8_t,uint8_t) NONBANKED;
void cf_video_begin(CFVideo *) NONBANKED;
void cf_video_commit(CFVideo *) NONBANKED;
void cf_video_glyph(CFVideo *,uint8_t,uint8_t,char) NONBANKED;
void cf_video_palette(CFVideo *,const uint16_t *) NONBANKED;
void cf_video_portrait(CFVideo *,const CFPortrait *) NONBANKED;
#endif
