#ifndef CF_AREA_H
#define CF_AREA_H
#include <stdint.h>
#include "cf_behavior.h"
#include "cf_animation.h"
#include "cf_game.h"
typedef struct {uint16_t x,y;uint8_t width,height;const uint8_t *tiles;const uint16_t *palette;CFBehavior behavior;uint8_t animation_count;const CFAnimation *animations;CFAnimationBindings bindings;uint8_t health;CFProgram interact,hit,defeat;} CFActorSpawn;
typedef struct {uint8_t x,y,storage;CFProgram interact,hit;} CFObject;
typedef struct {uint16_t cell;uint8_t target;} CFAreaLink;
typedef struct {
    uint8_t width,height;uint16_t spawn_x,spawn_y;
    uint8_t tile_count,player_w,player_h,trigger_count,exit_count;
    const uint8_t *tiles,*sprite,*map,*solid;
    const uint16_t *tile_palette,*player_palette;
    const CFAreaLink *triggers,*exits;
    CFBehavior player_behavior;uint8_t actor_count;const CFActorSpawn *actors;
    uint8_t animation_count;const CFAnimation *animations;CFAnimationBindings bindings;
    uint8_t object_count;const CFObject *objects;CFProgram start;uint8_t interaction_range,attack_range;uint8_t step_count;const CFStepRule *steps;
} CFArea;
#endif
