#include "cf_behavior.h"
static int16_t magnitude(int16_t x){return x<0?-x:x;}
void cf_actor_init(CFActorState *a,int16_t x,int16_t y){a->body.x=x;a->body.y=y;a->body.camera_x=a->body.camera_y=0;a->body.facing_x=0;a->body.facing_y=1;a->origin_x=x;a->origin_y=y;a->direction=1;a->clock=0;a->moving=0;}
uint8_t cf_actor_tick(CFActorState *a,const CFBehavior *b,const CFWorldState *player,uint8_t w,uint8_t h,const uint8_t *solid){
    uint8_t i;int8_t dx=0,dy=0;int16_t x,y;
    if(++a->clock >= b->interval){a->clock=0;a->moving=0;
        if(b->kind==CF_BEHAVIOR_PATROL){
            int16_t position=b->axis?a->body.y:a->body.x,origin=b->axis?a->origin_y:a->origin_x;
            if(position>=origin+b->range)a->direction=-1;if(position<=origin-b->range)a->direction=1;
            if(b->axis)dy=a->direction;else dx=a->direction;
        }
        if(b->kind==CF_BEHAVIOR_CHASE){x=player->x-a->body.x;y=player->y-a->body.y;if(magnitude(x)+magnitude(y)<=b->sight){if(magnitude(x)>magnitude(y))dx=x<0?-1:1;else if(y)dy=y<0?-1:1;}}
        for(i=0;i<b->speed;++i){
            if(b->kind==CF_BEHAVIOR_PATROL){int16_t position=b->axis?a->body.y:a->body.x,origin=b->axis?a->origin_y:a->origin_x;if(position>=origin+b->range)a->direction=-1;if(position<=origin-b->range)a->direction=1;if(b->axis)dy=a->direction;else dx=a->direction;}
            if(!cf_world_move(&a->body,w,h,solid,dx,dy)){if(b->kind==CF_BEHAVIOR_PATROL)a->direction=-a->direction;break;}else a->moving=1;}
    }
    return magnitude(a->body.x-player->x)<8 && magnitude(a->body.y-player->y)<8;
}
