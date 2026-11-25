#ifndef CF_SCENE_H
#define CF_SCENE_H
#include "cf_game.h"
#include "cf_resource.h"
typedef struct {uint16_t count;const CFResource *tiles,*map,*palette;} CFBitmap;
typedef struct {uint8_t type,x,y,w,h,align;const CFResource *text;CFCondition condition;CFProgram action;} CFScreenElement;
typedef struct {uint8_t world,area,count;uint16_t background;const CFBitmap *image;const CFScreenElement *elements;CFProgram enter,back;uint8_t elements_bank;} CFScene;
#endif
