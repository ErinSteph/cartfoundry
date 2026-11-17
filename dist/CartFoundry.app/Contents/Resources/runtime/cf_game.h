#ifndef CF_GAME_H
#define CF_GAME_H
#include <stdint.h>
#include "cf_flow.h"
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
typedef struct {uint8_t op,a,b,c,d;int16_t value;} CFInstruction;
typedef struct {uint8_t count;const CFInstruction *code;} CFProgram;
/* type 0=always, 1=variable, 2=item count; rhs_variable selects a variable RHS. */
typedef struct {uint8_t type,a,b,compare,rhs_variable;int16_t value;} CFCondition;
typedef struct {CFCondition condition;CFProgram actions;} CFChoiceRule;
typedef struct {uint8_t capacity;const uint16_t *initial;} CFStorageDef;
typedef struct {uint16_t interval;uint8_t repeat,autostart,pause_dialogue;CFProgram program;} CFTimerDef;
typedef struct {uint8_t variable_count,item_count,storage_count;const int16_t *initial;const uint8_t *stack_limits;const CFStorageDef *storages;uint16_t seed;uint8_t timer_count;const CFTimerDef *timers;} CFGameDef;
typedef struct {int16_t variables[CF_GAME_VARIABLES];uint16_t counts[CF_GAME_STORAGES][CF_GAME_ITEMS];uint8_t request_storage,request_prompt,request_world,request_readonly,dirty;uint16_t random;uint16_t timer_remaining[8];uint8_t timer_active[8];} CFGameState;
typedef struct {uint8_t tile,every,chance;CFProgram program;} CFStepRule;
typedef struct {int16_t cell;uint8_t steps[8];} CFStepState;
#ifdef __cplusplus
extern "C" {
#endif
void cf_game_tick(CFGameState *,const CFGameDef *,uint8_t in_dialogue);
uint16_t cf_game_random(CFGameState *,uint16_t maximum);
void cf_game_step_init(CFStepState *,int16_t cell);
uint8_t cf_game_step(CFStepState *,CFGameState *,const CFGameDef *,const CFStepRule *,uint8_t count,int16_t cell,uint8_t tile);
void cf_game_text(const CFGameState *,const char *,char *,uint16_t capacity,uint8_t columns);
void cf_game_init(CFGameState *,const CFGameDef *);
uint8_t cf_game_condition(const CFGameState *,const CFCondition *);
uint8_t cf_storage_change(CFGameState *,const CFGameDef *,uint8_t storage,uint8_t item,int16_t amount);
uint8_t cf_storage_transfer(CFGameState *,const CFGameDef *,uint8_t from,uint8_t to,uint8_t item,uint16_t amount);
uint8_t cf_game_execute(CFGameState *,const CFGameDef *,const CFProgram *);
void cf_story_enter(CFState *,uint8_t prompt,CFGameState *,const CFGameDef *,const CFProgram *enter);
void cf_story_normalize(CFState *,const CFPrompt *,const CFChoiceRule *,const CFGameState *);
uint8_t cf_story_step(CFState *,const CFPrompt *,uint8_t count,uint8_t entry,uint8_t keys,CFGameState *,const CFGameDef *,const CFChoiceRule *,const CFProgram *enter);
#ifdef __cplusplus
}
#endif
#endif
