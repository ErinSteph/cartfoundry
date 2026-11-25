#ifndef CF_RENDER_H
#define CF_RENDER_H
#include "cf_scene.h"
#include "cf_flow.h"
#include "cf_video.h"
#ifdef __TARGET_gb
#define CF_RENDER_CALL BANKED
#else
#define CF_RENDER_CALL
#endif
/* Callback functions are resident. Buffers and game state are in WRAM. */
typedef struct {
 CFGameState *game;char *body,*source;char control_hint[40],modal_hint[24];CFVideo *video;
} CFRenderContext;
void cf_render_dialogue(const CFRenderContext *,const CFPrompt *,const CFChoiceRule *,uint8_t choice) CF_RENDER_CALL;
void cf_render_screen(const CFRenderContext *,const CFScene *,uint8_t cursor) CF_RENDER_CALL;
void cf_render_keyboard(const CFRenderContext *,const CFGameDef *) CF_RENDER_CALL;
#endif
