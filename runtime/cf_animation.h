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
#define CF_DIR_UP 0
#define CF_DIR_DOWN 1
#define CF_DIR_LEFT 2
#define CF_DIR_RIGHT 3
/* directional==0 (also what zero-initialised bindings give) keeps the single move/idle clips.
   With directional==1, move_dir/idle_dir (indexed by CF_DIR_*) override move/idle; a slot of
   CF_NO_ANIMATION falls back to the single move/idle clip, then to base. */
typedef struct { uint8_t base,move,idle,action; uint8_t directional,move_dir[4],idle_dir[4]; } CFAnimationBindings;
typedef struct { CFAnimationState state; uint8_t clip,action_active; } CFAnimationController;
#ifdef __cplusplus
extern "C" {
#endif
uint8_t cf_animation_transform(const CFAnimationController *,const CFAnimation *,uint8_t count,uint8_t behavior_flags,int8_t facing_x);
void cf_animation_play(CFAnimationState *,const CFAnimation *);
uint8_t cf_animation_tick(CFAnimationState *,const CFAnimation *);
void cf_animation_init(CFAnimationController *,const CFAnimation *,uint8_t count,CFAnimationBindings);
/* Bindings are passed by pointer: the table is large enough that copying it per call costs ROM on the Game Boy. */
uint8_t cf_animation_update_facing(CFAnimationController *,const CFAnimation *,uint8_t count,const CFAnimationBindings *,uint8_t moving,uint8_t fire,int8_t facing_x,int8_t facing_y);
#ifdef __cplusplus
}
static inline uint8_t cf_animation_update(CFAnimationController *c,const CFAnimation *clips,uint8_t count,CFAnimationBindings b,uint8_t moving,uint8_t fire){return cf_animation_update_facing(c,clips,count,&b,moving,fire,0,1);}
extern "C" {
#else
static uint8_t cf_animation_update(CFAnimationController *c,const CFAnimation *clips,uint8_t count,CFAnimationBindings b,uint8_t moving,uint8_t fire){return cf_animation_update_facing(c,clips,count,&b,moving,fire,0,1);}
#endif
#ifdef __cplusplus
}
#endif
#endif
