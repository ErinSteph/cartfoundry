#include "cf_raycast.h"
static int16_t absolute(int16_t n){return n<0?-n:n;}
void cf_raycast(int16_t x0,int16_t y0,int16_t x1,int16_t y1,uint8_t width,uint8_t height,const uint8_t *solid,const CFHitbox *targets,uint8_t count,CFRayHit *out){int16_t dx=absolute(x1-x0),dy=-absolute(y1-y0),sx=x0<x1?1:-1,sy=y0<y1?1:-1,error=dx+dy,e2;uint16_t steps=0;uint8_t i;out->kind=0;out->index=255;out->cell=-1;out->distance=0;while(steps<=512){out->x=x0;out->y=y0;out->distance=steps;if(x0<0||y0<0||x0>=(int16_t)width*8||y0>=(int16_t)height*8){out->kind=CF_HIT_WALL;return;}out->cell=y0/8*width+x0/8;
if(steps){for(i=0;i<count;++i){const CFHitbox *t=&targets[i];if(t->kind&&x0>=t->x&&y0>=t->y&&x0<t->x+t->width&&y0<t->y+t->height){out->kind=t->kind;out->index=t->index;return;}}if(solid&&solid[out->cell]){out->kind=CF_HIT_WALL;return;}}
if(x0==x1&&y0==y1)return;e2=2*error;if(e2>=dy){error+=dy;x0+=sx;}if(e2<=dx){error+=dx;y0+=sy;}++steps;}}
