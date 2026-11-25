#ifdef __TARGET_gb
#pragma bank 2
#endif
#include "cf_sound.h"
#ifdef __TARGET_gb
#include <gb/gb.h>
#include "cf_bank.h"
#endif
static const uint16_t notes[5][8]={{880,1175,1568,0,0,0,0,0},{220,165,110,73,0,0,0,0},{523,659,784,1047,784,659,523,0},{131,0,196,0,165,0,196,0},{1047,0,784,0,659,0,523,0}};
static const uint8_t lengths[5]={4,4,8,8,8};
static void silence(uint8_t c){
#ifdef __TARGET_gb
 switch(c){case 0:NR12_REG=0;break;case 1:NR22_REG=0;break;case 2:NR30_REG=0;break;case 3:NR42_REG=0;break;}
#else
 (void)c;
#endif
}
static void output(CFSoundState *s,uint8_t channel){
#ifdef __TARGET_gb
 CFSoundVoice *v=&s->voices[channel];CFSoundTrack *t=&v->instrument;uint8_t routing=0,i,length_flag=t->length?0x40:0;
 NR52_REG=0x80;NR50_REG=s->master;
 for(i=0;i<4;++i){uint8_t pan=s->voices[i].instrument.pan;if(pan&1)routing|=(uint8_t)(0x10<<i);if(pan&2)routing|=(uint8_t)(1<<i);}NR51_REG=routing;
 if(!v->frequency||!t->dac){silence(channel);return;}
 switch(channel){
 case 0:NR10_REG=t->sweep;NR11_REG=(v->duty<<6)|(t->length?(64-t->length)&63:0);NR12_REG=(v->volume<<4)|t->envelope;NR13_REG=(uint8_t)v->period;NR14_REG=0x80|length_flag|(v->period>>8);break;
 case 1:NR21_REG=(v->duty<<6)|(t->length?(64-t->length)&63:0);NR22_REG=(v->volume<<4)|t->envelope;NR23_REG=(uint8_t)v->period;NR24_REG=0x80|length_flag|(v->period>>8);break;
 case 2:NR30_REG=0;for(i=0;i<16;++i)*((volatile uint8_t *)0xff30+i)=t->wave[i];NR31_REG=t->length?(uint8_t)(256-t->length):0;NR32_REG=t->wave_level<<5;NR30_REG=0x80;NR33_REG=(uint8_t)v->period;NR34_REG=0x80|length_flag|(v->period>>8);break;
 case 3:NR41_REG=t->length?(64-t->length)&63:0;NR42_REG=(v->volume<<4)|t->envelope;NR43_REG=(uint8_t)(v->frequency-1);NR44_REG=0x80|length_flag;break;
 }
#else
 (void)s;(void)channel;
#endif
}
void cf_sound_stop(CFSoundState *s) CF_SOUND_CALL {uint8_t i;s->song_clip=0;s->song_tick=0;s->master=0x77;for(i=0;i<4;++i){s->voices[i].clip=0;s->voices[i].frequency=0;s->voices[i].remaining=0;s->voices[i].step=0;s->voices[i].instrument.pan=3;silence(i);}}
void cf_sound_init(CFSoundState *s) CF_SOUND_CALL {uint8_t i;s->clips=0;s->count=0;cf_sound_stop(s);for(i=0;i<4;++i)s->voices[i].trigger_serial=0;}
static void load_track(const CFSoundClip *clip,uint8_t channel,CFSoundTrack *track){
#ifdef __TARGET_gb
 if(clip->track_bank)cf_bank_copy(clip->track_bank,(const uint8_t *)&clip->tracks[channel],(uint8_t *)track,sizeof *track);else *track=clip->tracks[channel];
#else
 *track=clip->tracks[channel];
#endif
}
static uint8_t length(const CFSoundState *s,const CFSoundVoice *v){return v->clip<=5?lengths[v->clip-1]:v->track?v->instrument.count:s->clips[v->clip-6].count;}
static void note(CFSoundState *s,uint8_t channel){CFSoundVoice *v=&s->voices[channel];CFSoundNote n;++v->trigger_serial;v->age=v->envelope_clock=v->sweep_clock=0;
 if(v->clip<=5){v->frequency=notes[v->clip-1][v->step];v->remaining=v->clip<=2?4:10;v->duty=2;v->volume=channel?10:8;}
 else{const CFSoundClip *c=&s->clips[v->clip-6];const CFSoundNote *source=v->track?v->instrument.notes:c->notes;uint8_t bank=v->track?v->instrument.bank:c->bank;
#ifdef __TARGET_gb
 if(bank)cf_bank_copy(bank,(const uint8_t *)&source[v->step],(uint8_t *)&n,sizeof n);else n=source[v->step];
#else
 (void)bank;n=source[v->step];
#endif
 v->frequency=n.frequency;v->remaining=n.ticks;v->duty=v->track?v->instrument.duty:c->duty;v->volume=n.volume?n.volume-1:v->track?v->instrument.volume:c->volume;}
 if(v->frequency&&channel<3){v->period=2048-(channel==2?65536UL:131072UL)/v->frequency;if(channel==0&&(v->instrument.sweep&7)&&!(v->instrument.sweep&8)&&v->period+(v->period>>(v->instrument.sweep&7))>2047)v->frequency=0;}
 output(s,channel);
}
static void legacy(CFSoundVoice *v){v->track=0;v->instrument.pan=3;v->instrument.envelope=0;v->instrument.sweep=0;v->instrument.length=0;v->instrument.dac=1;}
void cf_sound_start(CFSoundState *s,uint8_t clip,uint8_t loop) CF_SOUND_CALL {uint8_t i;if(clip<1||clip>5+s->count)return;
 if(clip>5&&s->clips[clip-6].tracks){const CFSoundClip *c=&s->clips[clip-6];if(!c->effect){cf_sound_stop(s);s->song_clip=clip;s->song_loop=loop;s->song_duration=c->duration;}if(!c->effect||!s->song_clip)s->master=c->master;for(i=0;i<4;++i)if(c->mask&(1<<i)){CFSoundVoice *v=&s->voices[i];load_track(c,i,&v->instrument);if(!v->instrument.count)continue;v->clip=clip;v->track=i+1;v->loop=c->effect?loop:0;v->step=0;note(s,i);}return;}
 if(clip>5&&!s->clips[clip-6].count)return;if(loop&&s->song_clip)cf_sound_stop(s);i=loop?0:1;s->voices[i].clip=clip;s->voices[i].step=0;s->voices[i].loop=loop;legacy(&s->voices[i]);note(s,i);
}
static void modulation(CFSoundVoice *v,uint8_t channel){uint8_t pace=v->instrument.envelope&7;uint16_t delta;
 ++v->age;if(v->instrument.length&&(uint16_t)(v->age<<8)>=(uint16_t)(v->instrument.length*60))v->frequency=0;
 if(channel!=2&&pace){v->envelope_clock+=64;while(v->envelope_clock>=(uint16_t)pace*60){v->envelope_clock-=(uint16_t)pace*60;if(v->instrument.envelope&8){if(v->volume<15)++v->volume;}else if(v->volume)--v->volume;}}
 pace=(v->instrument.sweep>>4)&7;if(channel==0&&pace&&(v->instrument.sweep&7)&&v->frequency){v->sweep_clock+=128;while(v->sweep_clock>=(uint16_t)pace*60){v->sweep_clock-=(uint16_t)pace*60;delta=v->period>>(v->instrument.sweep&7);if(v->instrument.sweep&8)v->period-=delta;else v->period+=delta;if(v->period>2047){v->frequency=0;break;}v->frequency=131072UL/(2048-v->period);}}
}
void cf_sound_tick(CFSoundState *s) CF_SOUND_CALL {uint8_t i;for(i=0;i<4;++i){CFSoundVoice *v=&s->voices[i];if(!v->clip)continue;if(v->clip>5+s->count){v->clip=0;v->frequency=0;silence(i);continue;}modulation(v,i);if(v->remaining&&--v->remaining)continue;if(++v->step>=length(s,v)){if(!v->loop){v->clip=0;v->frequency=0;silence(i);continue;}v->step=0;}note(s,i);}if(s->song_clip&&++s->song_tick>=s->song_duration){if(s->song_loop)cf_sound_start(s,s->song_clip,1);else s->song_clip=0;}}
