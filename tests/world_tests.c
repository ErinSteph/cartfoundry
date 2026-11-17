#include "cf_world.h"
#include <stdio.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x);return 1;} } while(0)
int main(void){
    uint8_t map[40*32]={0};CFWorldState s={16,16,0,0,0,-1};
    map[2*40+3]=1;
    CHECK(!cf_world_move(&s,40,32,map,1,0));CHECK(s.x==16);CHECK(s.facing_x==1);CHECK(cf_world_facing_cell(&s,40,32)==2*40+3);
    CHECK(cf_world_move(&s,40,32,map,0,1));CHECK(s.y==17);
    s.x=0;s.y=0;CHECK(!cf_world_move(&s,40,32,map,-1,0));CHECK(!cf_world_move(&s,40,32,map,0,-1));CHECK(cf_world_facing_cell(&s,40,32)==-1);
    s.x=160;s.y=130;cf_world_camera(&s,40,32);CHECK(s.camera_x==84&&s.camera_y==62);
    s.x=312;s.y=248;cf_world_camera(&s,40,32);CHECK(s.camera_x==160&&s.camera_y==112);CHECK(!cf_world_move(&s,40,32,map,1,0));
    s.x=16;s.y=17;CHECK(!cf_world_move(&s,40,32,map,1,0)); /* lower collision corner blocks */
    CHECK(!cf_world_move(&s,40,32,map,2,0));
    puts("World bounds, foot-box collision, facing, camera and movement checks passed.");return 0;
}
