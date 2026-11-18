#pragma codeseg MAIN
#pragma constseg MAIN
#include <gb/gb.h>
#include <gb/cgb.h>
#include "cf_font.h"
#include "cf_flow.h"
#include "cf_world.h"
#include "cf_area.h"
#include "cf_raycast.h"
#include "cf_scene.h"
#include "cf_bank.h"
#include "project.h"
static volatile uint8_t input_held,input_pressed,input_previous,pending_ticks;
/* Latch short button presses even when a renderer/event spans a video frame. */
#pragma codeseg HOME
static void sample_input(void) NONBANKED {uint8_t held=joypad();input_pressed|=held & ~input_previous;input_previous=held;input_held=held;if(pending_ticks<255)++pending_ticks;}
#pragma codeseg MAIN
static CFState state;
static CFGameState game;
static uint8_t touch_cooldown;
static uint8_t dialog,active_scene,screen_cursor=255;
static const CFScene *scene;
static uint8_t bitmap_buffer[256],screen_map[360];
static void screen_draw(void) NONBANKED ;
static void screen_select(int8_t delta) NONBANKED;
static void scene_load(uint8_t id,uint8_t area_override) NONBANKED ;
static void area_enter(uint8_t id) NONBANKED ;
#pragma codeseg HOME
static void bitmap_load(const CFBitmap *image,uint8_t base) NONBANKED {uint16_t offset=0,remaining=image->count*16;uint8_t count;uint16_t palette[4];while(remaining){count=remaining>=256?16:remaining/16;cf_bank_copy(image->bank,image->tiles+offset,bitmap_buffer,(uint16_t)count*16);set_bkg_data(base+offset/16,count,bitmap_buffer);remaining-=(uint16_t)count*16;offset+=(uint16_t)count*16;}cf_bank_copy(image->bank,image->map,screen_map,360);cf_bank_copy(image->bank,(const uint8_t*)image->palette,(uint8_t*)palette,8);if(CF_COLOR&&_cpu==CGB_TYPE)set_bkg_palette(0,1,palette);}
#pragma codeseg MAIN

static char body_text[512];
static uint8_t menu_active,menu_storage,menu_side,menu_choice,menu_readonly;
static void process_requests(void);
static void menu_draw(void);
static void menu_input(uint8_t pressed);
static const palette_color_t colors[] = {RGB(29,29,25), RGB(23,24,20), RGB(12,15,18), RGB(4,7,11)};
static uint8_t origin_x, origin_y;
static uint16_t render_map=0x9800,world_vram_map=0x9800;
static uint8_t render_palette=7;
static void tile_at(uint16_t map,uint8_t x,uint8_t y,uint8_t tile,uint8_t palette){uint8_t *addr=(uint8_t *)(map+((uint16_t)(y&31)<<5)+(x&31));set_vram_byte(addr,tile);if(CF_COLOR&&_cpu==CGB_TYPE){VBK_REG=1;set_vram_byte(addr,palette);VBK_REG=0;}}
static uint16_t hidden_map(void){return (LCDC_REG&8)?0x9800:0x9c00;}
static void show_map(uint16_t map,uint8_t scroll_x,uint8_t scroll_y){vsync();CRITICAL {if(map==0x9c00)LCDC_REG|=8;else LCDC_REG&=~8;move_bkg(scroll_x,scroll_y);}SHOW_BKG;}

#if CF_WORLD
static CFWorldState world;
static CFStepState step_state;
static const CFArea *area;
static CFActorState actors[4];
static CFAnimationController player_animation,actor_animations[4];
static uint8_t loaded_frames[5],actor_health[4];
static CFHitbox ray_targets[140];
static CFRayHit ray_hit;
static uint8_t player_clock,player_moving;
#define CF_MAP_W (area->width)
#define CF_MAP_H (area->height)
#define CF_SPAWN_X (area->spawn_x)
#define CF_SPAWN_Y (area->spawn_y)
#define CF_PLAYER_W (area->player_w)
#define CF_PLAYER_H (area->player_h)
#define CF_TRIGGER_COUNT (area->trigger_count)
#define cf_map (area->map)
#define cf_solid (area->solid)
#define cf_triggers (area->triggers)

static int16_t last_col = -1, last_row = -1;
static void map_cell(uint8_t x, uint8_t y) {
    tile_at(world_vram_map,x,y,x<CF_MAP_W&&y<CF_MAP_H?cf_map[(uint16_t)y*CF_MAP_W+x]:128,0);
}
static void stream_full(void) {
    uint8_t x,y,tx = world.camera_x >> 3,ty = world.camera_y >> 3;
    for (y=0;y<19;++y) for (x=0;x<21;++x) map_cell(tx+x,ty+y);
    last_col=tx; last_row=ty;

}
static void stream_edges(void) {
    uint8_t i,x,y,tx=world.camera_x>>3,ty=world.camera_y>>3;
    if (last_col != tx) { x = tx > last_col ? tx+20 : tx; for(i=0;i<19;++i) map_cell(x,ty+i); }
    if (last_row != ty) { y = ty > last_row ? ty+18 : ty; for(i=0;i<21;++i) map_cell(tx+i,y); }
    last_col=tx; last_row=ty; move_bkg((uint8_t)world.camera_x,(uint8_t)world.camera_y);
}
static void draw_actor(int16_t px,int16_t py,uint8_t width,uint8_t height,uint8_t offset,uint8_t flip,uint8_t palette) {
    uint8_t x,y,index=offset;int16_t sx,sy;
    for(y=0;y<height/8;++y)for(x=0;x<width/8;++x){
        sx=px+4-width/2-world.camera_x+((flip&1)?width/8-1-x:x)*8;sy=py+8-height-world.camera_y+((flip&2)?height/8-1-y:y)*8;set_sprite_prop(index,palette|((flip&1)?S_FLIPX:0)|((flip&2)?S_FLIPY:0));
        if(sx < -7 || sx > 159 || sy < -7 || sy > 143)move_sprite(index,0,0);else move_sprite(index,(uint8_t)(sx+8),(uint8_t)(sy+16));++index;
    }
}
static void player_draw(void) {
    uint8_t i,offset=CF_PLAYER_W/8*(CF_PLAYER_H/8);draw_actor(world.x,world.y,CF_PLAYER_W,CF_PLAYER_H,0,cf_animation_transform(&player_animation,area->animations,area->animation_count,area->player_behavior.flags,world.facing_x),0);
    for(i=0;i<area->actor_count;++i){if(!actor_health[i]){uint8_t j;for(j=0;j<area->actors[i].width/8*(area->actors[i].height/8);++j)move_sprite(offset+j,0,0);}else draw_actor(actors[i].body.x,actors[i].body.y,area->actors[i].width,area->actors[i].height,offset,cf_animation_transform(&actor_animations[i],area->actors[i].animations,area->actors[i].animation_count,area->actors[i].behavior.flags,actors[i].body.facing_x),i+1);offset+=area->actors[i].width/8*(area->actors[i].height/8);}
}
static void upload_animations(void){
    uint8_t i,tiles=CF_PLAYER_W/8*(CF_PLAYER_H/8),offset=tiles,frame=player_animation.state.frame;
    if(loaded_frames[0]!=frame){set_sprite_data(0,tiles,area->sprite+(uint16_t)frame*tiles*16);loaded_frames[0]=frame;}
    for(i=0;i<area->actor_count;++i){const CFActorSpawn *a=&area->actors[i];tiles=a->width/8*(a->height/8);frame=actor_animations[i].state.frame;if(loaded_frames[i+1]!=frame){set_sprite_data(offset,tiles,a->tiles+(uint16_t)frame*tiles*16);loaded_frames[i+1]=frame;}offset+=tiles;}
}
static void world_reset(void) {
    uint8_t i;
    cf_animation_init(&player_animation,area->animations,area->animation_count,area->bindings);loaded_frames[0]=255;
    for(i=0;i<area->actor_count;++i){actor_health[i]=area->actors[i].health;cf_actor_init(&actors[i],area->actors[i].x,area->actors[i].y);cf_animation_init(&actor_animations[i],area->actors[i].animations,area->actors[i].animation_count,area->actors[i].bindings);loaded_frames[i+1]=255;}
    upload_animations();player_clock=0;player_moving=0;touch_cooldown=60;
    world.x=CF_SPAWN_X;world.y=CF_SPAWN_Y;world.facing_x=0;world.facing_y=-1;
    cf_world_camera(&world,CF_MAP_W,CF_MAP_H);dialog=0;origin_x=origin_y=0;
    world_vram_map=hidden_map();stream_full();player_draw();show_map(world_vram_map,(uint8_t)world.camera_x,(uint8_t)world.camera_y);SHOW_SPRITES;
}
static void world_load(uint8_t id) {
    uint8_t i,j,offset;area=&cf_areas[id];DISPLAY_OFF;LCDC_REG&=~8;world_vram_map=0x9800;
    for(i=0;i<40;++i)move_sprite(i,0,0);
    set_bkg_data(128,area->tile_count,area->tiles);set_sprite_data(0,CF_PLAYER_W/8*(CF_PLAYER_H/8),area->sprite);SPRITES_8x8;
    for(i=0;i<CF_PLAYER_W/8*(CF_PLAYER_H/8);++i){set_sprite_tile(i,i);set_sprite_prop(i,0);}
    if(CF_COLOR && _cpu==CGB_TYPE){set_bkg_palette(0,1,area->tile_palette);set_sprite_palette(0,1,area->player_palette);}
    offset=CF_PLAYER_W/8*(CF_PLAYER_H/8);
    for(i=0;i<area->actor_count;++i){const CFActorSpawn *spawn=&area->actors[i];uint8_t count=spawn->width/8*(spawn->height/8);set_sprite_data(offset,count,spawn->tiles);for(j=0;j<count;++j){set_sprite_tile(offset+j,offset+j);set_sprite_prop(offset+j,i+1);}if(CF_COLOR && _cpu==CGB_TYPE)set_sprite_palette(i+1,1,spawn->palette);offset+=count;}
    DISPLAY_ON;world_reset();cf_game_step_init(&step_state,(world.y+4)/8*CF_MAP_W+(world.x+4)/8);cf_game_execute(&game,&cf_game_def,&area->start);
}
static void cast_ray(uint8_t distance,uint8_t combat){
    uint8_t count=0,i;int16_t cell;
    for(i=0;i<area->object_count;++i){ray_targets[count].x=area->objects[i].x*8;ray_targets[count].y=area->objects[i].y*8;ray_targets[count].width=ray_targets[count].height=8;ray_targets[count].kind=CF_HIT_OBJECT;ray_targets[count++].index=i;}
    for(i=0;i<area->actor_count;++i)if(actor_health[i]){ray_targets[count].x=actors[i].body.x;ray_targets[count].y=actors[i].body.y;ray_targets[count].width=ray_targets[count].height=8;ray_targets[count].kind=CF_HIT_ACTOR;ray_targets[count++].index=i;}
    if(!combat){for(i=0;i<area->exit_count;++i){cell=area->exits[i].cell;ray_targets[count].x=cell%CF_MAP_W*8;ray_targets[count].y=cell/CF_MAP_W*8;ray_targets[count].width=ray_targets[count].height=8;ray_targets[count].kind=CF_HIT_EXIT;ray_targets[count++].index=i;}for(i=0;i<area->trigger_count;++i){cell=area->triggers[i].cell;ray_targets[count].x=cell%CF_MAP_W*8;ray_targets[count].y=cell/CF_MAP_W*8;ray_targets[count].width=ray_targets[count].height=8;ray_targets[count].kind=CF_HIT_PROMPT;ray_targets[count++].index=i;}}
    cf_raycast(world.x+4,world.y+4,world.x+4+world.facing_x*distance,world.y+4+world.facing_y*distance,CF_MAP_W,CF_MAP_H,cf_solid,ray_targets,count,&ray_hit);
}
static void ray_action(uint8_t combat){uint8_t index;cast_ray(combat?area->attack_range:area->interaction_range,combat);index=ray_hit.index;
    if(ray_hit.kind==CF_HIT_OBJECT){const CFObject *o=&area->objects[index];if(combat)cf_game_execute(&game,&cf_game_def,&o->hit);else if(o->interact.count)cf_game_execute(&game,&cf_game_def,&o->interact);else if(o->storage<cf_game_def.storage_count)game.request_storage=o->storage;}
    if(ray_hit.kind==CF_HIT_ACTOR){const CFActorSpawn *a=&area->actors[index];if(!combat){cf_animation_update(&actor_animations[index],a->animations,a->animation_count,a->bindings,actors[index].moving,1);upload_animations();player_draw();}if(combat){if(actor_health[index])--actor_health[index];cf_game_execute(&game,&cf_game_def,&a->hit);if(!actor_health[index])cf_game_execute(&game,&cf_game_def,&a->defeat);player_draw();}else if(a->interact.count)cf_game_execute(&game,&cf_game_def,&a->interact);else if(a->behavior.touch_prompt<CF_PROMPT_COUNT)game.request_prompt=a->behavior.touch_prompt;}
    if(!combat&&ray_hit.kind==CF_HIT_EXIT)area_enter(area->exits[index].target);
    if(!combat&&ray_hit.kind==CF_HIT_PROMPT)game.request_prompt=area->triggers[index].target;
}

#endif
static void glyph(uint8_t x,uint8_t y,char c) {
    tile_at(render_map,origin_x+x,origin_y+y,(uint8_t)c-32,render_palette);
}
static void text(uint8_t x,uint8_t y,const char *s,uint8_t limit) {
    while(*s && limit--) glyph(x++,y,*s++);
}
static void begin_text(void){uint8_t x,y;render_map=hidden_map();render_palette=render_map==0x9800?7:4;
#if CF_WORLD
    if(scene->world){origin_x=world.camera_x>>3;origin_y=world.camera_y>>3;for(y=0;y<18;++y)for(x=0;x<20;++x){uint8_t wx=origin_x+x,wy=origin_y+y;tile_at(render_map,wx,wy,wx<CF_MAP_W&&wy<CF_MAP_H?cf_map[(uint16_t)wy*CF_MAP_W+wx]:128,0);}}else
#endif
    {origin_x=origin_y=0;for(y=0;y<18;++y)for(x=0;x<20;++x)tile_at(render_map,x,y,scene->image?screen_map[(uint16_t)y*20+x]:0,0);}

    if(CF_COLOR&&_cpu==CGB_TYPE)set_bkg_palette(render_palette,1,colors);
}
static void commit_text(void){game.dirty=0;
#if CF_WORLD
    if(scene->world){world.camera_x&=~7;world.camera_y&=~7;show_map(render_map,(uint8_t)world.camera_x,(uint8_t)world.camera_y);}else show_map(render_map,0,0);HIDE_SPRITES;
#else
    show_map(render_map,0,0);
#endif
}
static void draw(void) {
    uint8_t x,y,i,choice_y,body_y,columns;const char *c;CFTextLayout layout;
    const CFPrompt *p=&cf_prompts[state.prompt];begin_text();cf_text_layout(p,&layout);if(p->text_palette&&CF_COLOR&&_cpu==CGB_TYPE)set_bkg_palette(render_palette,1,p->text_palette);
    for(y=0;y<p->box_h;++y) for(x=0;x<p->box_w;++x) {
        uint8_t edge_x=x==0 || x==p->box_w-1,edge_y=y==0 || y==p->box_h-1;
        glyph(p->box_x+x,p->box_y+y,!(p->text_flags&1)?' ':edge_x && edge_y ? '+' : edge_x ? '|' : edge_y ? '-' : ' ');
    }
    columns=layout.columns;choice_y=p->box_y+layout.choice_row;
    if(p->text_flags&2)text(p->box_x+p->padding,p->box_y+p->padding,p->title,columns);
    cf_game_text(&game,p->body,body_text,sizeof body_text,columns);
    y=p->box_y+layout.body_row;c=body_text;body_y=y+layout.body_rows;
    while(*c&&y<body_y){uint8_t length=0;while(c[length]&&c[length]!='\n')++length;x=p->box_x+p->padding;if(p->text_align==1)x+=(columns-length)/2;else if(p->text_align==2)x+=columns-length;while(*c&&*c!='\n')glyph(x++,y,*c++);if(*c=='\n')++c;++y;}
    for(i=0;i<p->choice_count;++i){glyph(p->box_x+p->padding,choice_y+i*p->choice_spacing,cf_game_condition(&game,&cf_choice_rules[(uint16_t)state.prompt*3+i].condition)?(i==state.choice?p->cursor:' '):p->disabled_marker);text(p->box_x+p->padding+1,choice_y+i*p->choice_spacing,p->choices[i],columns-1);}
    if(p->text_flags&4)text(p->box_x+p->padding,p->box_y+layout.footer_row,*p->footer?p->footer:p->modal?CF_MODAL_HINT:CF_CONTROL_HINT,columns);
    if(p->portrait){const CFPortrait *image=p->portrait;uint8_t w=image->width/8,h=image->height/8,px=19-w,xx,yy;uint8_t base=render_map==0x9800?112:96,pal=render_map==0x9800?5:6;set_bkg_data(base,w*h,image->tiles);if(CF_COLOR&&_cpu==CGB_TYPE)set_bkg_palette(pal,1,image->palette);for(yy=0;yy<h;++yy)for(xx=0;xx<w;++xx){tile_at(render_map,origin_x+px+xx,origin_y+yy,base+yy*w+xx,pal);}}
    commit_text();
}
static void cursor_draw(void){uint8_t i;CFTextLayout layout;const CFPrompt *p=&cf_prompts[state.prompt];cf_text_layout(p,&layout);for(i=0;i<p->choice_count;++i)glyph(p->box_x+p->padding,p->box_y+layout.choice_row+i*p->choice_spacing,cf_game_condition(&game,&cf_choice_rules[(uint16_t)state.prompt*3+i].condition)?(i==state.choice?p->cursor:' '):p->disabled_marker);}

#pragma codeseg HOME
static void screen_draw(void) NONBANKED {uint8_t i,x,y,row,length,columns;const CFScreenElement *e;const char *c;if(screen_cursor>=scene->count||!cf_game_condition(&game,&scene->elements[screen_cursor].condition))screen_select(1);begin_text();for(i=0;i<scene->count;++i){e=&scene->elements[i];if(e->type==3)continue;for(y=0;y<e->h;++y)for(x=0;x<e->w;++x)glyph(e->x+x,e->y+y,e->type==2?((x==0||x==e->w-1)?((y==0||y==e->h-1)?'+':'|'):(y==0||y==e->h-1)?'-':' '):' ');if(e->type==2)continue;columns=e->w-(e->type==1);cf_game_text(&game,e->text,body_text,sizeof body_text,columns);c=body_text;row=0;while(*c&&row<e->h){length=0;while(c[length]&&c[length]!='\n')++length;x=e->x+(e->type==1);if(e->align==1)x+=(columns-length)/2;else if(e->align==2)x+=columns-length;while(*c&&*c!='\n')glyph(x++,e->y+row,*c++);if(*c=='\n')++c;++row;}if(e->type==1)glyph(e->x,e->y,cf_game_condition(&game,&e->condition)?(screen_cursor==i?'>':' '):'x');}commit_text();}
#pragma codeseg MAIN
#pragma codeseg HOME
static void screen_select(int8_t delta) NONBANKED {uint8_t i,next=screen_cursor;for(i=0;i<scene->count;++i){if(next==255)next=delta<0?scene->count-1:0;else next=delta<0?(next?next-1:scene->count-1):(next+1)%scene->count;if(scene->elements[next].type==1&&cf_game_condition(&game,&scene->elements[next].condition)){screen_cursor=next;return;}}screen_cursor=255;}
#pragma codeseg MAIN
#pragma codeseg HOME
static void scene_load(uint8_t id,uint8_t area_override) NONBANKED {uint16_t palette[4];if(id>=CF_SCENE_COUNT)return;active_scene=id;scene=&cf_scenes[id];dialog=menu_active=0;game.request_prompt=game.request_storage=game.request_scene=game.request_area=255;game.request_world=0;screen_cursor=255;
#if CF_WORLD
if(scene->world){world_load(area_override==255?scene->area:area_override);}else
#endif
{HIDE_SPRITES;DISPLAY_OFF;palette[0]=scene->background;palette[1]=colors[1];palette[2]=colors[2];palette[3]=colors[3];if(scene->image)bitmap_load(scene->image,128);else if(CF_COLOR&&_cpu==CGB_TYPE)set_bkg_palette(0,1,palette);DISPLAY_ON;screen_select(1);screen_draw();}cf_game_execute(&game,&cf_game_def,&scene->enter);}
#pragma codeseg MAIN
#pragma codeseg HOME
static void area_enter(uint8_t id) NONBANKED {
#if CF_WORLD
if(cf_area_scene[id]!=active_scene)scene_load(cf_area_scene[id],id);else world_load(id);
#else
(void)id;
#endif
}
#pragma codeseg MAIN
static void restore_view(void){if(dialog){draw();return;}if(!scene->world){screen_draw();return;}
#if CF_WORLD
cf_world_camera(&world,CF_MAP_W,CF_MAP_H);world_vram_map=hidden_map();stream_full();player_draw();show_map(world_vram_map,(uint8_t)world.camera_x,(uint8_t)world.camera_y);SHOW_SPRITES;
#endif
}
static uint8_t menu_source(void){return menu_storage&&menu_side?0:menu_storage;}
static uint8_t menu_items(uint8_t *out){uint8_t i,count=0,source=menu_source();for(i=0;i<cf_game_def.item_count;++i)if(game.counts[source][i])out[count++]=i;return count;}
static void number(uint8_t x,uint8_t y,uint16_t n){char digits[6];uint8_t len=0;do{digits[len++]=(char)('0'+n%10);n/=10;}while(n);while(len)glyph(x++,y,digits[--len]);}
static void menu_draw(void){uint8_t x,y,ids[16],count=menu_items(ids),i,start;begin_text();
    for(y=0;y<18;++y)for(x=0;x<20;++x)glyph(x,y,(x==0||x==19)?'|':(y==0||y==17)?'-':' ');text(1,1,cf_storage_names[menu_source()],18);if(menu_choice>=count)menu_choice=count?count-1:0;start=menu_choice>9?menu_choice-9:0;if(!count)text(1,4,"Empty",18);for(i=start;i<count&&i<start+10;++i){glyph(1,3+i-start,i==menu_choice?'>':' ');text(2,3+i-start,cf_item_names[ids[i]],12);number(15,3+i-start,game.counts[menu_source()][ids[i]]);}text(1,15,menu_readonly?CF_VIEW_HINT:menu_storage?CF_CONTAINER_HINT:CF_INVENTORY_HINT,18);text(1,16,CF_CLOSE_HINT,18);commit_text();
}
static void menu_input(uint8_t pressed){uint8_t ids[16],count,item;if(!(pressed&(J_UP|J_DOWN|CF_CONTROL_INTERACT|CF_CONTROL_CLOSE|CF_CONTROL_INVENTORY)))return;count=menu_items(ids);if(pressed&CF_CONTROL_CLOSE){menu_active=0;restore_view();return;}if(menu_storage&&!menu_readonly&&(pressed&CF_CONTROL_INVENTORY)){menu_side=!menu_side;menu_choice=0;menu_draw();return;}if(count){if(pressed&J_UP)menu_choice=menu_choice?menu_choice-1:count-1;if(pressed&J_DOWN)menu_choice=(menu_choice+1)%count;if(pressed&CF_CONTROL_INTERACT){item=ids[menu_choice];if(menu_storage&&!menu_readonly)game.variables[31]=cf_storage_transfer(&game,&cf_game_def,menu_source(),menu_side?menu_storage:0,item,1);else if(cf_game_execute(&game,&cf_game_def,&cf_item_use[item])&&!menu_readonly&&cf_item_consumable[item])cf_storage_change(&game,&cf_game_def,0,item,-1);}menu_draw();}}
static void process_requests(void){uint8_t i;for(i=0;i<8&&(game.request_scene<CF_SCENE_COUNT||game.request_area!=255);++i){if(game.request_scene<CF_SCENE_COUNT)scene_load(game.request_scene,255);else{uint8_t target=game.request_area;game.request_area=255;area_enter(target);}}game.request_scene=game.request_area=255;
    if(game.request_world){game.request_world=0;game.request_prompt=game.request_storage=255;menu_active=dialog=0;touch_cooldown=60;restore_view();return;}
for(i=0;i<4&&game.request_prompt<CF_PROMPT_COUNT;++i){uint8_t p=game.request_prompt;game.request_prompt=255;menu_active=0;cf_story_enter(&state,p,&game,&cf_game_def,cf_enter_scripts);cf_story_normalize(&state,cf_prompts,cf_choice_rules,&game);
    dialog=1;
    if(game.request_prompt==255&&!game.request_world&&game.request_storage==255&&game.request_scene==255)draw();}game.request_prompt=255;
    if(game.request_world){game.request_world=0;game.request_storage=255;menu_active=dialog=0;touch_cooldown=60;restore_view();return;}
    if(game.request_storage<cf_game_def.storage_count){menu_storage=game.request_storage;menu_readonly=game.request_readonly;game.request_readonly=0;game.request_storage=255;menu_active=1;menu_side=menu_choice=0;menu_draw();}}
void main(void) {
    uint8_t held,pressed,keys,ticks;
#if CF_WORLD
    uint8_t i,step,contact,moving;int16_t old_x,old_y;int8_t dx,dy;
#endif
    cf_game_init(&game,&cf_game_def);
    DISPLAY_OFF;LCDC_REG|=LCDCF_BG8000;bitmap_load(&cf_splash,0);set_bkg_tiles(0,0,20,18,screen_map);BGP_REG=OBP0_REG=0xe4;if(CF_COLOR&&_cpu==CGB_TYPE){VBK_REG=1;fill_bkg_rect(0,0,32,32,0);VBK_REG=0;}move_bkg(0,0);SHOW_BKG;DISPLAY_ON;{uint16_t wait=CF_SPLASH_TICKS;while(wait--)vsync();}
    DISPLAY_OFF;LCDC_REG&=~LCDCF_BG8000;set_1bpp_colors(3,0);set_bkg_1bpp_data(0,96,cf_font);fill_bkg_rect(0,0,32,32,0);DISPLAY_ON;scene_load(CF_DEFAULT_SCENE,255);input_previous=joypad();input_held=input_previous;input_pressed=pending_ticks=0;add_VBL(sample_input);
    process_requests();
    while(1) {
        vsync();CRITICAL {held=input_held;pressed=input_pressed;input_pressed=0;ticks=pending_ticks;pending_ticks=0;}keys=0;
        {while(ticks--)cf_game_tick(&game,&cf_game_def,dialog||menu_active);process_requests();if(game.dirty){if(menu_active)menu_draw();else if(dialog){cf_story_normalize(&state,cf_prompts,cf_choice_rules,&game);draw();}else if(!scene->world)screen_draw();game.dirty=0;}}
        if(menu_active){menu_input(pressed);process_requests();continue;}
        if(!scene->world&&!dialog){if(pressed&J_UP)screen_select(-1);if(pressed&J_DOWN)screen_select(1);if(pressed&(J_UP|J_DOWN))screen_draw();if((pressed&CF_CONTROL_INTERACT)&&screen_cursor<scene->count&&cf_game_condition(&game,&scene->elements[screen_cursor].condition))cf_game_execute(&game,&cf_game_def,&scene->elements[screen_cursor].action);if(pressed&CF_CONTROL_CLOSE)cf_game_execute(&game,&cf_game_def,&scene->back);process_requests();if(game.dirty&&!dialog&&!menu_active)screen_draw();continue;}
        if(dialog&&cf_prompts[state.prompt].modal)pressed&=~(CF_CONTROL_CLOSE|CF_CONTROL_INVENTORY|CF_CONTROL_RESTART);
        if(dialog&&(pressed&CF_CONTROL_CLOSE)){dialog=0;restore_view();continue;}
        if(pressed & CF_CONTROL_INVENTORY){
#if CF_WORLD
            if(dialog&&cf_prompts[state.prompt].modal)continue;
#endif
            game.request_storage=0;game.request_readonly=0;process_requests();continue;}
#if CF_WORLD
        if(scene->world&&(pressed & CF_CONTROL_RESTART)){world_reset();continue;}
        if(scene->world&&!dialog) {
            dx=held & J_LEFT ? -1 : held & J_RIGHT ? 1 : 0;dy=held & J_UP ? -1 : held & J_DOWN ? 1 : 0;
            old_x=world.x;old_y=world.y;
            if(++player_clock>=area->player_behavior.interval){player_clock=0;for(step=0;step<area->player_behavior.speed;++step)cf_world_move(&world,CF_MAP_W,CF_MAP_H,cf_solid,dx,dy);}
            contact=255;if(touch_cooldown)--touch_cooldown;
            if(!dx&&!dy)player_moving=0;else if(!player_clock)player_moving=(world.x!=old_x||world.y!=old_y);
            moving=player_moving;
            cf_animation_update(&player_animation,area->animations,area->animation_count,area->bindings,moving,(pressed & CF_CONTROL_INTERACT)!=0);
            for(i=0;i<area->actor_count;++i){if(!actor_health[i])continue;old_x=actors[i].body.x;old_y=actors[i].body.y;
                if(cf_actor_tick(&actors[i],&area->actors[i].behavior,&world,CF_MAP_W,CF_MAP_H,cf_solid)&&!touch_cooldown)contact=area->actors[i].behavior.touch_prompt;
                cf_animation_update(&actor_animations[i],area->actors[i].animations,area->actors[i].animation_count,area->actors[i].bindings,actors[i].moving,0);
            }
            if(area->step_count){int16_t cell=(world.y+4)/8*CF_MAP_W+(world.x+4)/8;if(cf_game_step(&step_state,&game,&cf_game_def,area->steps,area->step_count,cell,cf_map[cell])){process_requests();if(dialog||menu_active)continue;}}
            upload_animations();
            stream_edges();player_draw();
            if(contact<CF_PROMPT_COUNT){cf_story_enter(&state,contact,&game,&cf_game_def,cf_enter_scripts);cf_story_normalize(&state,cf_prompts,cf_choice_rules,&game);dialog=1;touch_cooldown=60;draw();process_requests();continue;}
            if(pressed & CF_CONTROL_CLOSE)ray_action(1);else if(pressed & CF_CONTROL_INTERACT)ray_action(0);process_requests();
            continue;
        }
#endif
        if(pressed & J_UP) keys |= CF_KEY_UP;
        if(pressed & J_DOWN) keys |= CF_KEY_DOWN;
        if(pressed & CF_CONTROL_INTERACT) keys |= CF_KEY_ACCEPT;

        if(cf_story_step(&state,cf_prompts,CF_PROMPT_COUNT,CF_ENTRY,keys,&game,&cf_game_def,cf_choice_rules,cf_enter_scripts)){if(keys&(CF_KEY_UP|CF_KEY_DOWN))cursor_draw();else if(game.request_prompt==255&&!game.request_world&&game.request_storage==255&&game.request_scene==255)draw();}
        process_requests();
    }
}
