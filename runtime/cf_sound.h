#ifndef CF_SOUND_H
#define CF_SOUND_H
#include <stdint.h>
#ifdef __TARGET_gb
#include <gb/gb.h>
#define CF_SOUND_CALL BANKED
#else
#define CF_SOUND_CALL
#endif
typedef struct {uint8_t clip,step,remaining;uint16_t frequency;} CFSoundVoice;
typedef struct {CFSoundVoice voices[2];} CFSoundState;
#ifdef __cplusplus
extern "C" {
#endif
void cf_sound_init(CFSoundState *) CF_SOUND_CALL;
void cf_sound_start(CFSoundState *,uint8_t clip,uint8_t loop) CF_SOUND_CALL;
void cf_sound_stop(CFSoundState *) CF_SOUND_CALL;
void cf_sound_tick(CFSoundState *) CF_SOUND_CALL;
#ifdef __cplusplus
}
#endif
#endif
