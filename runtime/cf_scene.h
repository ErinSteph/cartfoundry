#ifndef CF_SCENE_H
#define CF_SCENE_H
#include "cf_game.h"
typedef struct {uint8_t bank;uint16_t count;const uint8_t *tiles,*map;const uint16_t *palette;} CFBitmap;
typedef struct {uint8_t type,x,y,w,h,align;const char *text;CFCondition condition;CFProgram action;} CFScreenElement;
typedef struct {uint8_t world,area,count;uint16_t background;const CFBitmap *image;const CFScreenElement *elements;CFProgram enter,back;} CFScene;
#endif
