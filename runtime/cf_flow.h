#ifndef CF_FLOW_H
#define CF_FLOW_H
#include <stdint.h>
#include "cf_resource.h"
#ifdef __TARGET_gb
#include <gb/gb.h>
#define CF_FLOW_CALL NONBANKED
#else
#define CF_FLOW_CALL
#endif
#define CF_MAX_CHOICES 3
#define CF_NO_TARGET 255
#define CF_KEY_UP 1
#define CF_KEY_DOWN 2
#define CF_KEY_ACCEPT 4
#define CF_KEY_RESET 8
#define CF_KEY_LEFT 16
#define CF_KEY_RIGHT 32
#define CF_KEY_CANCEL 64
#ifdef __TARGET_gb
typedef const CFResource *CFPromptText;
#else
typedef const char *CFPromptText;
#endif
typedef struct {uint8_t width,height;const CFResource *tiles,*palette;} CFPortrait;
typedef struct {
    CFPromptText title;
    CFPromptText body;
    uint8_t choice_count;
    CFPromptText choices[CF_MAX_CHOICES];
    uint8_t targets[CF_MAX_CHOICES];
    uint8_t box_x, box_y, box_w, box_h;
    uint8_t modal;const CFPortrait *portrait;
    uint8_t text_flags,padding,choice_spacing,text_align;char cursor,disabled_marker;CFPromptText footer;const uint16_t *text_palette;
} CFPrompt;
typedef struct { uint8_t prompt, choice; } CFState;
typedef struct {uint8_t scene,cursor,dialog,storage,side,item,readonly;} CFMenuFrame;
typedef struct {uint8_t depth;CFMenuFrame frames[4];} CFMenuStack;
#ifdef __cplusplus
extern "C" {
#endif
extern uint8_t cf_prompt_bank;
void cf_prompt_read(const CFPrompt *,uint8_t,CFPrompt *) CF_FLOW_CALL;
uint8_t cf_menu_push(CFMenuStack *,const CFMenuFrame *) CF_FLOW_CALL;
uint8_t cf_menu_pop(CFMenuStack *,CFMenuFrame *) CF_FLOW_CALL;
#ifdef __cplusplus
}
#endif
typedef struct {uint8_t columns,body_rows,body_row,choice_row,footer_row;} CFTextLayout;
#ifdef __cplusplus
extern "C"
#endif
void cf_text_layout(const CFPrompt *,CFTextLayout *) CF_FLOW_CALL;
/* Returns 1 when the screen should be redrawn. Uses newly pressed keys. */
#ifdef __cplusplus
extern "C"
#endif
uint8_t cf_step(CFState *state, const CFPrompt *prompts, uint8_t count, uint8_t entry, uint8_t keys) CF_FLOW_CALL;
#endif
