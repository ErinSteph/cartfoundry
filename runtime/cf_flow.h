#ifndef CF_FLOW_H
#define CF_FLOW_H
#include <stdint.h>
#define CF_MAX_CHOICES 3
#define CF_NO_TARGET 255
#define CF_KEY_UP 1
#define CF_KEY_DOWN 2
#define CF_KEY_ACCEPT 4
#define CF_KEY_RESET 8
typedef struct {uint8_t width,height;const uint8_t *tiles;const uint16_t *palette;} CFPortrait;
typedef struct {
    const char *title;
    const char *body;
    uint8_t choice_count;
    const char *choices[CF_MAX_CHOICES];
    uint8_t targets[CF_MAX_CHOICES];
    uint8_t box_x, box_y, box_w, box_h;
    uint8_t modal;const CFPortrait *portrait;
    uint8_t text_flags,padding,choice_spacing,text_align;char cursor,disabled_marker;const char *footer;const uint16_t *text_palette;
} CFPrompt;
typedef struct { uint8_t prompt, choice; } CFState;
typedef struct {uint8_t columns,body_rows,body_row,choice_row,footer_row;} CFTextLayout;
#ifdef __cplusplus
extern "C"
#endif
void cf_text_layout(const CFPrompt *,CFTextLayout *);
/* Returns 1 when the screen should be redrawn. Uses newly pressed keys. */
#ifdef __cplusplus
extern "C"
#endif
uint8_t cf_step(CFState *state, const CFPrompt *prompts, uint8_t count, uint8_t entry, uint8_t keys);
#endif
