#include "cf_animation.h"
#include <stdio.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){CFAnimation once={2,4,10,0};CFAnimationState s;int i;
cf_animation_play(&s,&once);CHECK(s.frame==2&&s.playing&&!s.finished);
for(i=0;i<3;++i)cf_animation_tick(&s,&once);CHECK(s.frame==2);cf_animation_tick(&s,&once);CHECK(s.frame==3);
for(i=0;i<6;++i)cf_animation_tick(&s,&once);CHECK(s.frame==4&&!s.playing&&s.finished);for(i=0;i<20;++i)cf_animation_tick(&s,&once);CHECK(s.frame==4&&s.elapsed==10);
cf_animation_play(&s,&once);CHECK(s.frame==2&&s.elapsed==0&&!s.finished);
CFAnimation loop={0,3,8,1};cf_animation_play(&s,&loop);for(i=0;i<8;++i)cf_animation_tick(&s,&loop);CHECK(s.frame==0&&s.elapsed==0&&s.playing&&!s.finished);
CFAnimation clips[]={{0,0,1,1},{1,2,4,1},{3,4,4,0}};CFAnimationBindings b={0,1,0,2};CFAnimationController c;cf_animation_init(&c,clips,3,b);
cf_animation_update(&c,clips,3,b,1,0);CHECK(c.clip==1&&c.state.frame==1);cf_animation_update(&c,clips,3,b,1,0);cf_animation_update(&c,clips,3,b,1,0);CHECK(c.state.frame==2);
cf_animation_update(&c,clips,3,b,0,1);CHECK(c.clip==2&&c.action_active);for(i=0;i<4;++i)cf_animation_update(&c,clips,3,b,0,0);CHECK(c.state.finished);cf_animation_update(&c,clips,3,b,0,0);CHECK(c.clip==0&&!c.action_active);
cf_animation_update(&c,clips,3,b,0,1);CHECK(c.clip==2&&c.state.frame==3);/* Directional bindings: clips 0 base, 1/2 move up/down, 3 move left (right unset), 4/5 idle up/down, 6 idle left */
{CFAnimation dc[]={{0,0,1,1},{1,1,4,1},{2,2,4,1},{3,3,4,1},{4,4,4,1},{5,5,4,1},{6,6,4,1}};
CFAnimationBindings d={0,255,255,255,1,{1,2,3,255},{4,5,6,255}};CFAnimationController dcn;
cf_animation_init(&dcn,dc,7,d);
cf_animation_update_facing(&dcn,dc,7,&d,1,0,0,-1);CHECK(dcn.clip==1);
cf_animation_update_facing(&dcn,dc,7,&d,1,0,0,1);CHECK(dcn.clip==2);
cf_animation_update_facing(&dcn,dc,7,&d,1,0,-1,0);CHECK(dcn.clip==3);
cf_animation_update_facing(&dcn,dc,7,&d,0,0,-1,0);CHECK(dcn.clip==6);
cf_animation_update_facing(&dcn,dc,7,&d,0,0,0,-1);CHECK(dcn.clip==4);
cf_animation_update_facing(&dcn,dc,7,&d,0,0,0,1);CHECK(dcn.clip==5);
/* unset right slots fall back to the single clips, then base */
cf_animation_update_facing(&dcn,dc,7,&d,1,0,1,0);CHECK(dcn.clip==0);
d.move=2;d.idle=4;cf_animation_update_facing(&dcn,dc,7,&d,1,0,1,0);CHECK(dcn.clip==2);cf_animation_update_facing(&dcn,dc,7,&d,0,0,1,0);CHECK(dcn.clip==4);
/* single mode ignores direction tables, and zero-initialised bindings are single mode */
d.directional=0;cf_animation_update_facing(&dcn,dc,7,&d,1,0,0,-1);CHECK(dcn.clip==2);
{CFAnimationBindings z={0,1,0,255};cf_animation_init(&dcn,dc,7,z);cf_animation_update_facing(&dcn,dc,7,&z,1,0,-1,0);CHECK(dcn.clip==1);}
}
puts("Animation ranges, exact duration, loops, one-shots, replay, bindings and directional sprites passed.");return 0;}
