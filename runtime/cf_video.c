#include <gb/gb.h>
#include <gb/cgb.h>
#include "cf_video.h"
#pragma codeseg HOME
#pragma constseg HOME
static const palette_color_t colors[]={RGB(29,29,25),RGB(23,24,20),RGB(12,15,18),RGB(4,7,11)};
void cf_video_tile_at(uint16_t map,uint8_t x,uint8_t y,uint8_t tile,uint8_t palette) NONBANKED {
 uint8_t *address=(uint8_t *)(map+((uint16_t)(y&31)<<5)+(x&31));set_vram_byte(address,tile);
 if(CF_COLOR&&_cpu==CGB_TYPE){VBK_REG=1;set_vram_byte(address,palette);VBK_REG=0;}
}
uint16_t cf_video_hidden_map(void) NONBANKED {return (LCDC_REG&8)?0x9800:0x9c00;}
void cf_video_show_map(uint16_t map,uint8_t x,uint8_t y) NONBANKED {
 vsync();CRITICAL {if(map==0x9c00)LCDC_REG|=8;else LCDC_REG&=~8;move_bkg(x,y);}SHOW_BKG;
}
void cf_video_glyph(CFVideo *v,uint8_t x,uint8_t y,char c) NONBANKED {
 cf_video_tile_at(v->render_map,v->origin_x+x,v->origin_y+y,(uint8_t)c-32,v->palette);
}
void cf_video_palette(CFVideo *v,const uint16_t *palette) NONBANKED {
 if(v->color&&_cpu==CGB_TYPE)set_bkg_palette(v->palette,1,palette);
}
void cf_video_begin(CFVideo *v) NONBANKED {
 uint8_t x,y,wx,wy;v->render_map=cf_video_hidden_map();v->palette=v->render_map==0x9800?7:4;
 if(v->is_world){v->origin_x=v->world->camera_x>>3;v->origin_y=v->world->camera_y>>3;
  for(y=0;y<18;++y)for(x=0;x<20;++x){wx=v->origin_x+x;wy=v->origin_y+y;cf_video_tile_at(v->render_map,wx,wy,wx<v->width&&wy<v->height?cf_resource_read(v->map,(uint16_t)wy*v->width+wx):128,0);}
 }else{v->origin_x=v->origin_y=0;for(y=0;y<18;++y)for(x=0;x<20;++x)cf_video_tile_at(v->render_map,x,y,v->has_image?v->screen[(uint16_t)y*20+x]:0,0);}
 cf_video_palette(v,colors);
}
void cf_video_commit(CFVideo *v) NONBANKED {
 v->game->dirty=0;
 if(v->is_world){v->world->camera_x&=~7;v->world->camera_y&=~7;cf_video_show_map(v->render_map,(uint8_t)v->world->camera_x,(uint8_t)v->world->camera_y);}else cf_video_show_map(v->render_map,0,0);
 HIDE_SPRITES;
}
void cf_video_portrait(CFVideo *v,const CFPortrait *image) NONBANKED {
 uint16_t palette[4];uint8_t w=image->width/8,h=image->height/8,x,y;
 uint8_t base=v->render_map==0x9800?112:96,pal=v->render_map==0x9800?5:6;
 cf_resource_copy(image->tiles,0,v->bitmap,(uint16_t)w*h*16);set_bkg_data(base,w*h,v->bitmap);
 if(v->color&&_cpu==CGB_TYPE){cf_resource_copy(image->palette,0,(uint8_t *)palette,8);set_bkg_palette(pal,1,palette);}
 for(y=0;y<h;++y)for(x=0;x<w;++x)cf_video_tile_at(v->render_map,v->origin_x+19-w+x,v->origin_y+y,base+y*w+x,pal);
}
