#include "cf_behavior.h"
#include <stdio.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Failure line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){uint8_t solid[40*32]={0};CFWorldState player={80,40,0,0,0,1};CFActorState enemy;CFBehavior patrol={CF_BEHAVIOR_PATROL,1,1,0,8,80,255};int i;
cf_actor_init(&enemy,40,40);for(i=0;i<8;++i)cf_actor_tick(&enemy,&patrol,&player,40,32,solid);CHECK(enemy.body.x==48);cf_actor_tick(&enemy,&patrol,&player,40,32,solid);CHECK(enemy.body.x==47);
solid[5*40+7]=1;cf_actor_init(&enemy,48,40);cf_actor_tick(&enemy,&patrol,&player,40,32,solid);CHECK(enemy.body.x==48&&enemy.direction==-1);
CFBehavior chase={CF_BEHAVIOR_CHASE,1,2,0,32,80,2};cf_actor_init(&enemy,16,40);cf_actor_tick(&enemy,&chase,&player,40,32,solid);CHECK(enemy.body.x==16);cf_actor_tick(&enemy,&chase,&player,40,32,solid);CHECK(enemy.body.x==17);
player.x=300;for(i=0;i<8;++i)cf_actor_tick(&enemy,&chase,&player,40,32,solid);CHECK(enemy.body.x==17);player.x=20;CHECK(cf_actor_tick(&enemy,&chase,&player,40,32,solid));puts("Patrol bounds, wall reversal, chase distance, timing and contact passed.");return 0;}
