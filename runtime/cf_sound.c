#ifdef __TARGET_gb
#pragma bank 3
#endif
#include "cf_sound.h"
#ifdef __TARGET_gb
#include <gb/gb.h>
#endif
/* Channel 1 is the loop; channel 2 is a retriggerable one-shot. Hz, ticks. */
static const uint16_t notes[5][8]={
 {880,1175,1568,0,0,0,0,0},{220,165,110,73,0,0,0,0},
 {523,659,784,1047,784,659,523,0},{131,0,196,0,165,0,196,0},
 {1047,0,784,0,659,0,523,0}};
static const uint8_t lengths[5]={4,4,8,8,8};
static void output(uint8_t channel,uint16_t hz){
#ifdef __TARGET_gb
 uint16_t pitch;
 NR52_REG=0x80;NR50_REG=0x77;NR51_REG=0x33;
 if(!hz){if(channel)NR22_REG=0;else NR12_REG=0;return;}
 pitch=2048-131072UL/hz;
 if(channel){NR21_REG=0x80;NR22_REG=0xa0;NR23_REG=(uint8_t)pitch;NR24_REG=0x80|(pitch>>8);}
 else {NR10_REG=0;NR11_REG=0x80;NR12_REG=0x80;NR13_REG=(uint8_t)pitch;NR14_REG=0x80|(pitch>>8);}
#else
 (void)channel;(void)hz;
#endif
}
void cf_sound_stop(CFSoundState *s) CF_SOUND_CALL {uint8_t i;for(i=0;i<2;++i){s->voices[i].clip=0;s->voices[i].frequency=0;s->voices[i].remaining=0;s->voices[i].step=0;output(i,0);}}
void cf_sound_init(CFSoundState *s) CF_SOUND_CALL {cf_sound_stop(s);}
static void note(CFSoundState *s,uint8_t i){CFSoundVoice *v=&s->voices[i];v->frequency=notes[v->clip-1][v->step];v->remaining=v->clip<=2?4:10;output(i,v->frequency);}
void cf_sound_start(CFSoundState *s,uint8_t clip,uint8_t loop) CF_SOUND_CALL {uint8_t i=loop?0:1;if(clip<1||clip>5)return;s->voices[i].clip=clip;s->voices[i].step=0;note(s,i);}
void cf_sound_tick(CFSoundState *s) CF_SOUND_CALL {uint8_t i;for(i=0;i<2;++i){CFSoundVoice *v=&s->voices[i];if(!v->clip)continue;if(v->remaining&&--v->remaining)continue;if(++v->step>=lengths[v->clip-1]){if(i){v->clip=0;v->frequency=0;output(i,0);continue;}v->step=0;}note(s,i);}}
