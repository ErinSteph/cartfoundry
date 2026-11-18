#ifndef CF_SOUND_H
#define CF_SOUND_H
#include <stdint.h>
#ifdef __TARGET_gb
#include <gb/gb.h>
#define CF_SOUND_CALL BANKED
#else
#define CF_SOUND_CALL
#endif
typedef struct {uint16_t frequency;uint8_t ticks,volume;} CFSoundNote;
/* Packed hardware controls: envelope = pace | increase<<3; sweep = NR10 bits. */
typedef struct {uint8_t count,duty,volume,pan,envelope,sweep;uint16_t length;uint8_t dac,wave_level,wave[16];const CFSoundNote *notes;uint8_t bank;} CFSoundTrack;
typedef struct {uint8_t count,duty,volume;const CFSoundNote *notes;uint8_t bank;const CFSoundTrack *tracks;uint8_t track_bank,mask,master;uint16_t duration;uint8_t effect;} CFSoundClip;
typedef struct {uint8_t clip,step,remaining,duty,volume,track,loop;uint16_t frequency,period,age,envelope_clock,sweep_clock,trigger_serial;CFSoundTrack instrument;} CFSoundVoice;
typedef struct {CFSoundVoice voices[4];const CFSoundClip *clips;uint8_t count,song_clip,song_loop,master;uint16_t song_tick,song_duration;} CFSoundState;
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
