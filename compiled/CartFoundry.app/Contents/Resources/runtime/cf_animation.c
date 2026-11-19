#include "cf_animation.h"
void cf_animation_play(CFAnimationState *s,const CFAnimation *a) {
    s->frame=(a->flags&CF_ANIM_REVERSE)?a->last:a->first;s->elapsed=0;s->phase=0;s->playing=1;s->finished=0;
}
uint8_t cf_animation_tick(CFAnimationState *s,const CFAnimation *a) {
    uint8_t old=s->frame;
    if(!s->playing || !a->duration || a->last<a->first)return 0;
    ++s->elapsed;
    if(s->elapsed>=a->duration){
        if(a->loop){s->frame=(a->flags&CF_ANIM_REVERSE)?a->last:a->first;s->elapsed=0;s->phase=0;}
        else{s->frame=(a->flags&CF_ANIM_REVERSE)?a->first:a->last;s->playing=0;s->finished=1;}
    }else{
        s->phase+=(uint16_t)a->last-a->first+1;
        if(s->phase>=a->duration){s->phase-=a->duration;if(a->flags&CF_ANIM_REVERSE){if(s->frame>a->first)--s->frame;}else if(s->frame<a->last)++s->frame;}
    }
    return old!=s->frame;
}
void cf_animation_init(CFAnimationController *c,const CFAnimation *clips,uint8_t count,CFAnimationBindings b){
    c->clip=b.base<count?b.base:0;c->action_active=0;
    if(count)cf_animation_play(&c->state,&clips[c->clip]);else{c->state.frame=0;c->state.playing=0;c->state.finished=1;c->state.elapsed=c->state.phase=0;}
}
uint8_t cf_animation_update_facing(CFAnimationController *c,const CFAnimation *clips,uint8_t count,const CFAnimationBindings *bp,uint8_t moving,uint8_t fire,int8_t facing_x,int8_t facing_y){
    uint8_t desired,move=bp->move,idle=bp->idle,base=bp->base,action=bp->action,old=c->state.frame;
    if(bp->directional){uint8_t d=facing_x<0?CF_DIR_LEFT:facing_x>0?CF_DIR_RIGHT:facing_y<0?CF_DIR_UP:CF_DIR_DOWN;if(bp->move_dir[d]<count)move=bp->move_dir[d];if(bp->idle_dir[d]<count)idle=bp->idle_dir[d];}
    if(!count)return 0;
    if(fire && action<count){c->clip=action;c->action_active=1;cf_animation_play(&c->state,&clips[c->clip]);return old!=c->state.frame;}
    if(c->action_active && (c->state.finished || (moving && clips[c->clip].loop)))c->action_active=0;
    desired=moving && move<count?move:!moving && idle<count?idle:base<count?base:0;
    if(!c->action_active && desired!=c->clip){c->clip=desired;cf_animation_play(&c->state,&clips[desired]);}
    else cf_animation_tick(&c->state,&clips[c->clip]);
    return old!=c->state.frame;
}

uint8_t cf_animation_transform(const CFAnimationController *c,const CFAnimation *clips,uint8_t count,uint8_t flags,int8_t facing_x){uint8_t flip=flags&3;if(c->clip<count)flip^=(clips[c->clip].flags>>1)&3;if((flags&4)&&facing_x<0)flip^=1;return flip;}
