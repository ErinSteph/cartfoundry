#ifndef CF_RAYCAST_H
#define CF_RAYCAST_H
#include <stdint.h>
#define CF_HIT_NONE 0
#define CF_HIT_WALL 1
#define CF_HIT_OBJECT 2
#define CF_HIT_ACTOR 3
#define CF_HIT_PROMPT 4
#define CF_HIT_EXIT 5
typedef struct {int16_t x,y;uint8_t width,height,kind,index;} CFHitbox;
typedef struct {uint8_t kind,index;int16_t cell,x,y;uint16_t distance;} CFRayHit;
#ifdef __cplusplus
extern "C" {
#endif
void cf_raycast(int16_t x0,int16_t y0,int16_t x1,int16_t y1,uint8_t width,uint8_t height,const uint8_t *solid,const CFHitbox *targets,uint8_t count,CFRayHit *out);
#ifdef __cplusplus
}
#endif
#endif
