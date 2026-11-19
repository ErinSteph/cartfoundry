#ifdef __TARGET_gb
#pragma bank 2
#endif
#include "cf_game.h"
/* Stable little-endian save records. Two journal copies per slot; commit byte is
   written last, so an interrupted write preserves the previous valid copy. */
static uint16_t save_get16(const volatile uint8_t *p){return p[0]|((uint16_t)p[1]<<8);}
static void save_put16(volatile uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static uint16_t save_transfer(CFGameState *s,const CFGameDef *d,volatile uint8_t *p,uint8_t load){
 uint16_t offset=0;uint8_t i,j;uint32_t bit;
#define SAVE8(v) do{if(load)(v)=p[offset];else p[offset]=(uint8_t)(v);++offset;}while(0)
#define SAVE16(v) do{if(load)(v)=save_get16(p+offset);else save_put16(p+offset,(uint16_t)(v));offset+=2;}while(0)
 for(i=0,bit=1;i<d->variable_count;++i,bit<<=1)if(d->save.variables&bit)SAVE16(s->variables[i]);
 for(i=0;i<d->text_count;++i)if(d->save.texts&(1<<i))for(j=0;j<21;++j)SAVE8(s->texts[i][j]);
 for(i=0;i<d->storage_count;++i)if(d->save.storages&(1<<i))for(j=0;j<d->item_count;++j)SAVE16(s->counts[i][j]);
 if(d->save.flags&CF_SAVE_TIMERS)for(i=0;i<d->timer_count;++i){SAVE16(s->timer_remaining[i]);SAVE8(s->timer_active[i]);}
 if(d->save.flags&CF_SAVE_RANDOM)SAVE16(s->random);
 if(d->save.flags&CF_SAVE_POSITION){SAVE8(s->position.scene);SAVE8(s->position.area);SAVE16(s->position.x);SAVE16(s->position.y);SAVE8(s->position.facing_x);SAVE8(s->position.facing_y);}
#undef SAVE8
#undef SAVE16
 return offset;
}
uint16_t cf_save_size(const CFGameDef *d) CF_GAME_CALL {
 uint16_t n=16;uint8_t i;uint32_t bit;for(i=0,bit=1;i<d->variable_count;++i,bit<<=1)if(d->save.variables&bit)n+=2;
 for(i=0;i<d->text_count;++i)if(d->save.texts&(1<<i))n+=21;
 for(i=0;i<d->storage_count;++i)if(d->save.storages&(1<<i))n+=(uint16_t)d->item_count*2;
 if(d->save.flags&CF_SAVE_TIMERS)n+=(uint16_t)d->timer_count*3;if(d->save.flags&CF_SAVE_RANDOM)n+=2;if(d->save.flags&CF_SAVE_POSITION)n+=8;return n;
}
static uint16_t save_crc(const volatile uint8_t *p,uint16_t n){uint16_t crc=0xffff,i;uint8_t b;for(i=4;i<n;++i){if(i>=12&&i<16)continue;crc^=(uint16_t)p[i]<<8;for(b=0;b<8;++b)crc=crc&0x8000?(crc<<1)^0x1021:crc<<1;}return crc;}
static uint8_t save_valid(const volatile uint8_t *p,const CFGameDef *d){uint8_t i;uint32_t schema=0;uint16_t n=cf_save_size(d);if(n>CF_SAVE_RECORD||p[0]!='C'||p[1]!='F'||p[2]!='S'||p[3]!=1||save_get16(p+10)!=n)return 0;for(i=0;i<4;++i)schema|=(uint32_t)p[4+i]<<(8*i);return schema==d->save.schema&&save_get16(p+12)==save_crc(p,n);}
uint8_t cf_save_action(CFGameState *s,const CFGameDef *d,volatile uint8_t *memory,uint8_t action,uint8_t slot) CF_GAME_CALL {
 volatile uint8_t *a,*b,*latest=0,*dest;uint8_t av,bv,i;uint16_t n,sequence;
 if(!memory||slot>=d->save.slot_count||d->save.slot_count>3)return 0;n=cf_save_size(d);if(n>CF_SAVE_RECORD)return 0;
 a=memory+(uint16_t)slot*CF_SAVE_RECORD*2;b=a+CF_SAVE_RECORD;av=save_valid(a,d);bv=save_valid(b,d);
 if(av&&bv)latest=(int16_t)(save_get16(b+8)-save_get16(a+8))>0?b:a;else if(av)latest=a;else if(bv)latest=b;
 if(action==2)return latest!=0;
 if(action==3){a[0]=b[0]=0;return 1;}
 if(action==1){if(!latest)return 0;save_transfer(s,d,latest+16,1);for(i=0;i<d->text_count;++i)s->texts[i][d->texts[i].limit]=0;
 if(d->save.flags&CF_SAVE_POSITION){s->menus.depth=s->continuation_count=s->keyboard_active=0;s->request_menu=s->request_save=s->after_prompt=s->request_prompt=s->request_storage=255;s->request_world=s->request_return=0;
 s->restoring=1;s->request_scene=s->position.scene;}s->dirty=1;return 1;}
 if(action!=0)return 0;sequence=latest?save_get16(latest+8)+1:1;dest=latest==a?b:a;dest[0]=0;
 dest[1]='F';dest[2]='S';dest[3]=1;for(i=0;i<4;++i)dest[4+i]=(uint8_t)(d->save.schema>>(8*i));save_put16(dest+8,sequence);save_put16(dest+10,n);dest[14]=dest[15]=0;
 save_transfer(s,d,dest+16,0);save_put16(dest+12,save_crc(dest,n));dest[0]='C';return 1;
}
