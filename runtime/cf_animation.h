#ifndef CF_ANIMATION_H
#define CF_ANIMATION_H
#include <stdint.h>
#define CF_NO_ANIMATION 255
#define CF_ANIM_REVERSE 1
#define CF_ANIM_FLIP_X 2
#define CF_ANIM_FLIP_Y 4
/* Inclusive frame range; duration is the complete animation in 60 Hz ticks. */
typedef struct { uint8_t first,last; uint16_t duration; uint8_t loop,flags; } CFAnimation;
typedef struct { uint8_t frame,playing,finished; uint16_t elapsed,phase; } CFAnimationState;
typedef struct { uint8_t base,move,idle,action; } CFAnimationBindings;
typedef struct { CFAnimationState state; uint8_t clip,action_active; } CFAnimationController;
#ifdef __cplusplus
extern "C" {
#endif
uint8_t cf_animation_transform(const CFAnimationController *,const CFAnimation *,uint8_t count,uint8_t behavior_flags,int8_t facing_x);
void cf_animation_play(CFAnimationState *,const CFAnimation *);
uint8_t cf_animation_tick(CFAnimationState *,const CFAnimation *);
void cf_animation_init(CFAnimationController *,const CFAnimation *,uint8_t count,CFAnimationBindings);
uint8_t cf_animation_update(CFAnimationController *,const CFAnimation *,uint8_t count,CFAnimationBindings,uint8_t moving,uint8_t fire);
#ifdef __cplusplus
}
#endif
#endif
