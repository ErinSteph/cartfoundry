#ifndef CF_GAME_H
#define CF_GAME_H
#include <stdint.h>
#include "cf_flow.h"
#include "cf_sound.h"
#define CF_GAME_VARIABLES 32
#define CF_GAME_ITEMS 16
#define CF_GAME_STORAGES 8
#define CF_RESULT_VARIABLE 31
#define CF_REQUEST_NONE 255
#define CF_OP_SET 1
#define CF_OP_ADD 2
#define CF_OP_GIVE 3
#define CF_OP_TAKE 4
#define CF_OP_MOVE 5
#define CF_OP_OPEN 6
#define CF_OP_DIALOGUE 7
#define CF_OP_IF_VAR 8
#define CF_OP_IF_ITEM 9
#define CF_OP_JUMP 10
#define CF_OP_RANDOM 11
#define CF_OP_RESUME 12
#define CF_OP_SUB 13
#define CF_OP_VIEW 14
#define CF_OP_SEED 15
#define CF_OP_TIMER 16
#define CF_OP_SCENE 17
#define CF_OP_AREA 18
#define CF_OP_SOUND 19
#define CF_OP_MENU 20
#define CF_OP_RETURN 21
#define CF_OP_KEYBOARD 22
#define CF_OP_SAVE 23
#define CF_OP_TEXT 24
#define CF_TEXT_FIELDS 8
#define CF_TEXT_LENGTH 20
#define CF_SAVE_BYTES 8192
#define CF_SAVE_RECORD 768
#define CF_SAVE_VARIABLES 1
#define CF_SAVE_TEXT 2
#define CF_SAVE_STORAGE 4
#define CF_SAVE_TIMERS 8
#define CF_SAVE_RANDOM 16
#define CF_SAVE_POSITION 32
typedef struct {uint8_t op,a,b,c,d;int16_t value;} CFInstruction;
typedef struct {uint8_t count;const CFInstruction *code;uint8_t bank;} CFProgram;
/* type 0=always, 1=variable, 2=item count; rhs_variable selects a variable RHS. */
typedef struct {uint8_t type,a,b,compare,rhs_variable;int16_t value;} CFCondition;
typedef struct {CFCondition condition;CFProgram actions;} CFChoiceRule;
typedef struct {uint8_t capacity;const uint16_t *initial;} CFStorageDef;
typedef struct {uint16_t interval;uint8_t repeat,autostart,pause_dialogue;CFProgram program;} CFTimerDef;
typedef struct {const char *initial;uint8_t limit;} CFTextDef;
typedef struct {const char *title,*characters;uint8_t count,columns,x,y;} CFKeyboardDef;
typedef struct {uint32_t schema,variables;uint8_t texts,storages,flags,slot_count;} CFSaveDef;
typedef struct {uint8_t scene,area;int16_t x,y;int8_t facing_x,facing_y;} CFSavePosition;
typedef struct {uint8_t variable_count,item_count,storage_count;const int16_t *initial;const uint8_t *stack_limits;const CFStorageDef *storages;uint16_t seed;uint8_t timer_count;const CFTimerDef *timers;uint8_t sound_count;const CFSoundClip *sounds;uint8_t text_count,keyboard_count;const CFTextDef *texts;const CFKeyboardDef *keyboards;CFSaveDef save;} CFGameDef;
typedef struct {CFSoundState sound;int16_t variables[CF_GAME_VARIABLES];uint16_t counts[CF_GAME_STORAGES][CF_GAME_ITEMS];uint8_t request_storage,request_prompt,request_world,request_readonly,dirty,request_scene,request_area;uint16_t random;uint16_t timer_remaining[8];uint8_t timer_active[8];
char texts[CF_TEXT_FIELDS][CF_TEXT_LENGTH+1],keyboard_buffer[CF_TEXT_LENGTH+1];
uint8_t keyboard_active,keyboard_id,keyboard_target,keyboard_limit,keyboard_cursor;
uint8_t request_menu,request_return,request_save,save_slot,restoring,after_prompt,save_status;
CFSavePosition position;CFMenuStack menus;CFProgram continuations[4];uint8_t continuation_count,continuation_pc[4];
} CFGameState;
typedef struct {uint8_t tile,every,chance;CFProgram program;} CFStepRule;
typedef struct {int16_t cell;uint8_t steps[8];} CFStepState;
#ifdef __TARGET_gb
#include <gb/gb.h>
#define CF_GAME_CALL BANKED
#else
#define CF_GAME_CALL
#endif
#ifdef __cplusplus
extern "C" {
#endif
void cf_game_save_result(CFGameState *,const CFGameDef *,uint8_t action,uint8_t ok) CF_GAME_CALL;
uint8_t cf_game_continue(CFGameState *,const CFGameDef *) CF_GAME_CALL;
uint8_t cf_keyboard_input(CFGameState *,const CFGameDef *,uint8_t keys) CF_GAME_CALL;
void cf_keyboard_render(const CFGameState *,const CFGameDef *,char *cells) CF_GAME_CALL;
uint8_t cf_save_action(CFGameState *,const CFGameDef *,volatile uint8_t *memory,uint8_t action,uint8_t slot) CF_GAME_CALL;
uint16_t cf_save_size(const CFGameDef *) CF_GAME_CALL;
void cf_game_tick(CFGameState *,const CFGameDef *,uint8_t in_dialogue) CF_GAME_CALL;
uint16_t cf_game_random(CFGameState *,uint16_t maximum) CF_GAME_CALL;
void cf_game_step_init(CFStepState *,int16_t cell) CF_GAME_CALL;
uint8_t cf_game_step(CFStepState *,CFGameState *,const CFGameDef *,const CFStepRule *,uint8_t count,int16_t cell,uint8_t tile) CF_GAME_CALL;
void cf_game_text(const CFGameState *,const char *,char *,uint16_t capacity,uint8_t columns) CF_GAME_CALL;
void cf_game_init(CFGameState *,const CFGameDef *) CF_GAME_CALL;
uint8_t cf_game_condition(const CFGameState *,const CFCondition *) CF_GAME_CALL;
uint8_t cf_storage_change(CFGameState *,const CFGameDef *,uint8_t storage,uint8_t item,int16_t amount) CF_GAME_CALL;
uint8_t cf_storage_transfer(CFGameState *,const CFGameDef *,uint8_t from,uint8_t to,uint8_t item,uint16_t amount) CF_GAME_CALL;
uint8_t cf_game_execute(CFGameState *,const CFGameDef *,const CFProgram *) CF_GAME_CALL;
void cf_story_enter(CFState *,uint8_t prompt,CFGameState *,const CFGameDef *,const CFProgram *enter) CF_GAME_CALL;
void cf_story_normalize(CFState *,const CFPrompt *,const CFChoiceRule *,const CFGameState *) CF_GAME_CALL;
uint8_t cf_story_step(CFState *,const CFPrompt *,uint8_t count,uint8_t entry,uint8_t keys,CFGameState *,const CFGameDef *,const CFChoiceRule *,const CFProgram *enter) CF_GAME_CALL;
#ifdef __cplusplus
}
#endif
#endif
