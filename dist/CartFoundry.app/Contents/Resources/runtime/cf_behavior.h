#ifndef CF_BEHAVIOR_H
#define CF_BEHAVIOR_H
#include "cf_world.h"
#define CF_BEHAVIOR_IDLE 0
#define CF_BEHAVIOR_PLAYER 1
#define CF_BEHAVIOR_PATROL 2
#define CF_BEHAVIOR_CHASE 3
typedef struct {uint8_t kind,speed,interval,axis,range,sight,touch_prompt,flags;} CFBehavior;
typedef struct {CFWorldState body;int16_t origin_x,origin_y;int8_t direction;uint8_t clock,moving;} CFActorState;
#ifdef __cplusplus
extern "C" {
#endif
void cf_actor_init(CFActorState *,int16_t x,int16_t y);
uint8_t cf_actor_tick(CFActorState *,const CFBehavior *,const CFWorldState *player,uint8_t w,uint8_t h,const uint8_t *solid);
#ifdef __cplusplus
}
#endif
#endif
