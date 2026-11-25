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
#include "cf_collision.h"
#include "project.h"
#include "cf_render.h"
#include <string.h>
static volatile uint8_t input_held,input_pressed,input_previous,pending_ticks;
/* Latch short button presses even when a renderer/event spans a video frame. */
#pragma codeseg HOME
static void sample_input(void) NONBANKED {uint8_t held=joypad();input_pressed|=held & ~input_previous;input_previous=held;input_held=held;if(pending_ticks<255)++pending_ticks;}
#pragma codeseg MAIN
static CFState state;
static CFPrompt current_prompt;
static CFChoiceRule current_rules[3];
static uint8_t loaded_prompt=255;
static const CFPrompt *prompt_current(void){if(loaded_prompt!=state.prompt){uint8_t i;cf_prompt_read(cf_prompts,state.prompt,&current_prompt);for(i=0;i<3;++i)cf_choice_read(cf_choice_rules,(uint16_t)state.prompt*3+i,&current_rules[i]);loaded_prompt=state.prompt;}return &current_prompt;}
static CFGameState game;
static uint8_t touch_cooldown;
static uint8_t dialog,active_scene,screen_cursor=255;
static const CFScene *scene;
static uint8_t bitmap_buffer[256],screen_map[360];
static void screen_draw(void) ;
static void screen_element(uint8_t index,CFScreenElement *out) NONBANKED;
static void screen_select(int8_t delta) NONBANKED;
static void scene_load(uint8_t id,uint8_t area_override) NONBANKED ;
static void area_enter(uint8_t id) NONBANKED ;
#pragma codeseg HOME
static void bitmap_load(const CFBitmap *image,uint8_t base) NONBANKED {uint16_t offset=0,remaining=image->count*16;uint8_t count;uint16_t palette[4];while(remaining){count=remaining>=256?16:remaining/16;cf_resource_copy(image->tiles,offset,bitmap_buffer,(uint16_t)count*16);set_bkg_data(base+offset/16,count,bitmap_buffer);remaining-=(uint16_t)count*16;offset+=(uint16_t)count*16;}cf_resource_copy(image->map,0,screen_map,360);cf_resource_copy(image->palette,0,(uint8_t*)palette,8);if(CF_COLOR&&_cpu==CGB_TYPE)set_bkg_palette(0,1,palette);}
#pragma codeseg MAIN

static char body_text[512],screen_text[512];
static CFVideo video;
static CFRenderContext render_context;
#define tile_at cf_video_tile_at
#define hidden_map cf_video_hidden_map
#define show_map cf_video_show_map
#define glyph(x,y,c) cf_video_glyph(&video,x,y,c)
#define commit_text() cf_video_commit(&video)
static uint8_t menu_active,menu_storage,menu_side,menu_choice,menu_readonly;
static void process_requests(void);
#if CF_FEATURE_KEYBOARD
static void keyboard_draw(void);
#else
#define keyboard_draw() ((void)0)
#endif
static void sync_position(void);
static void modal_requests(void);
static void menu_draw(void);
static void menu_input(uint8_t pressed);
static const palette_color_t colors[] = {RGB(29,29,25), RGB(23,24,20), RGB(12,15,18), RGB(4,7,11)};
#define origin_x video.origin_x
#define origin_y video.origin_y
#define render_map video.render_map
static uint16_t world_vram_map=0x9800;
#define render_palette video.palette




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
#define cf_solid (area->solid->data)
#define cf_triggers (area->triggers)

static int16_t last_col = -1, last_row = -1;
static void map_cell(uint8_t x, uint8_t y) {
    tile_at(world_vram_map,x,y,x<CF_MAP_W&&y<CF_MAP_H?cf_resource_read(cf_map,(uint16_t)y*CF_MAP_W+x):128,0);
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
static void upload_frame(uint8_t offset,uint8_t tiles,const CFSpriteFrame *frame){
    cf_resource_copy(frame,0,bitmap_buffer,(uint16_t)tiles*16);
    set_sprite_data(offset,tiles,bitmap_buffer);
}
static void upload_animations(void){
    uint8_t i,tiles=CF_PLAYER_W/8*(CF_PLAYER_H/8),offset=tiles,frame=player_animation.state.frame;
    if(loaded_frames[0]!=frame){upload_frame(0,tiles,&area->sprite[frame]);loaded_frames[0]=frame;}
    for(i=0;i<area->actor_count;++i){const CFActorSpawn *a=&area->actors[i];tiles=a->width/8*(a->height/8);frame=actor_animations[i].state.frame;if(loaded_frames[i+1]!=frame){upload_frame(offset,tiles,&a->tiles[frame]);loaded_frames[i+1]=frame;}offset+=tiles;}
}
static void world_reset(void) {
    uint8_t i;
    cf_animation_init(&player_animation,area->animations,area->animation_count,area->bindings);loaded_frames[0]=255;
    for(i=0;i<area->actor_count;++i){actor_health[i]=area->actors[i].health;cf_actor_init(&actors[i],area->actors[i].x,area->actors[i].y);cf_animation_init(&actor_animations[i],area->actors[i].animations,area->actors[i].animation_count,area->actors[i].bindings);loaded_frames[i+1]=255;}
    upload_animations();player_clock=0;player_moving=0;touch_cooldown=60;
    world.x=CF_SPAWN_X;world.y=CF_SPAWN_Y;world.facing_x=0;world.facing_y=-1;
    cf_world_camera(&world,CF_MAP_W,CF_MAP_H);dialog=0;origin_x=origin_y=0;
    if(!game.restoring){world_vram_map=hidden_map();stream_full();player_draw();show_map(world_vram_map,(uint8_t)world.camera_x,(uint8_t)world.camera_y);SHOW_SPRITES;}
}
static void world_tiles(void){uint8_t count,offset=0;while(offset<area->tile_count){count=area->tile_count-offset;if(count>16)count=16;cf_resource_copy(area->tiles,(uint16_t)offset*16,bitmap_buffer,(uint16_t)count*16);set_bkg_data(128+offset,count,bitmap_buffer);offset+=count;}}
static void world_load(uint8_t id) {
    uint8_t i,j,offset;area=&cf_areas[id];cf_collision_resource=area->solid;DISPLAY_OFF;LCDC_REG&=~8;world_vram_map=0x9800;
    for(i=0;i<40;++i)move_sprite(i,0,0);
    world_tiles();SPRITES_8x8;
    for(i=0;i<CF_PLAYER_W/8*(CF_PLAYER_H/8);++i){set_sprite_tile(i,i);set_sprite_prop(i,0);}
    if(CF_COLOR && _cpu==CGB_TYPE){set_bkg_palette(0,1,area->tile_palette);set_sprite_palette(0,1,area->player_palette);}
    offset=CF_PLAYER_W/8*(CF_PLAYER_H/8);
    for(i=0;i<area->actor_count;++i){const CFActorSpawn *spawn=&area->actors[i];uint8_t count=spawn->width/8*(spawn->height/8);for(j=0;j<count;++j){set_sprite_tile(offset+j,offset+j);set_sprite_prop(offset+j,i+1);}if(CF_COLOR && _cpu==CGB_TYPE)set_sprite_palette(i+1,1,spawn->palette);offset+=count;}
    DISPLAY_ON;world_reset();cf_game_step_init(&step_state,(world.y+4)/8*CF_MAP_W+(world.x+4)/8);if(!game.restoring)cf_game_execute(&game,&cf_game_def,&area->start);
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
    if(ray_hit.kind==CF_HIT_ACTOR){const CFActorSpawn *a=&area->actors[index];if(!combat){cf_animation_update_facing(&actor_animations[index],a->animations,a->animation_count,&a->bindings,actors[index].moving,1,actors[index].body.facing_x,actors[index].body.facing_y);upload_animations();player_draw();}if(combat){if(actor_health[index])--actor_health[index];cf_game_execute(&game,&cf_game_def,&a->hit);if(!actor_health[index])cf_game_execute(&game,&cf_game_def,&a->defeat);player_draw();}else if(a->interact.count)cf_game_execute(&game,&cf_game_def,&a->interact);else if(a->behavior.touch_prompt<CF_PROMPT_COUNT)game.request_prompt=a->behavior.touch_prompt;}
    if(!combat&&ray_hit.kind==CF_HIT_EXIT)area_enter(area->exits[index].target);
    if(!combat&&ray_hit.kind==CF_HIT_PROMPT)game.request_prompt=area->triggers[index].target;
}

#endif

static void text(uint8_t x,uint8_t y,const char *s,uint8_t limit) {
    while(*s && limit--) glyph(x++,y,*s++);
}
static void begin_text(void){video.game=&game;video.has_image=scene->image!=0;video.is_world=scene->world;video.color=CF_COLOR;video.bitmap=bitmap_buffer;video.screen=screen_map;
#if CF_WORLD
video.world=&world;video.map=area?area->map:0;video.width=area?area->width:0;video.height=area?area->height:0;
#endif
cf_video_begin(&video);}

static void draw(void){begin_text();cf_render_dialogue(&render_context,prompt_current(),current_rules,state.choice);}
static void cursor_draw(void){uint8_t i;CFTextLayout layout;const CFPrompt *p=prompt_current();cf_text_layout(p,&layout);for(i=0;i<p->choice_count;++i)glyph(p->box_x+p->padding,p->box_y+layout.choice_row+i*p->choice_spacing,cf_game_condition(&game,&current_rules[i].condition)?(i==state.choice?p->cursor:' '):p->disabled_marker);}

#pragma codeseg MAIN
/* Selection stays in MAIN; page composition uses the banked renderer. */
static void screen_draw(void){CFScreenElement element;if(screen_cursor<scene->count)screen_element(screen_cursor,&element);if(screen_cursor>=scene->count||!cf_game_condition(&game,&element.condition))screen_select(1);begin_text();cf_render_screen(&render_context,scene,screen_cursor);}
#pragma codeseg MAIN
#pragma codeseg HOME
static void screen_element(uint8_t index,CFScreenElement *out) NONBANKED {cf_bank_copy(scene->elements_bank,(const uint8_t *)&scene->elements[index],(uint8_t *)out,sizeof *out);}
static void screen_select(int8_t delta) NONBANKED {CFScreenElement element;uint8_t i,next=screen_cursor;for(i=0;i<scene->count;++i){if(next==255)next=delta<0?scene->count-1:0;else next=delta<0?(next?next-1:scene->count-1):(next+1)%scene->count;screen_element(next,&element);if(element.type==1&&cf_game_condition(&game,&element.condition)){screen_cursor=next;return;}}screen_cursor=255;}
#pragma codeseg MAIN
#pragma codeseg HOME
static void scene_load(uint8_t id,uint8_t area_override) NONBANKED {uint16_t palette[4];if(id>=CF_SCENE_COUNT)return;game.menus.depth=0;game.continuation_count=0;game.after_prompt=255;active_scene=id;scene=&cf_scenes[id];dialog=menu_active=0;game.request_prompt=game.request_storage=game.request_scene=game.request_area=255;game.request_world=0;screen_cursor=255;
#if CF_WORLD
if(scene->world){world_load(game.restoring?game.position.area:area_override==255?scene->area:area_override);}else
#endif
{HIDE_SPRITES;DISPLAY_OFF;palette[0]=scene->background;palette[1]=colors[1];palette[2]=colors[2];palette[3]=colors[3];if(scene->image)bitmap_load(scene->image,128);else if(CF_COLOR&&_cpu==CGB_TYPE)set_bkg_palette(0,1,palette);DISPLAY_ON;screen_select(1);screen_draw();}if(!game.restoring)cf_game_execute(&game,&cf_game_def,&scene->enter);}
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
static void restore_view(void){if(game.keyboard_active){keyboard_draw();return;}if(dialog){draw();return;}if(!scene->world){screen_draw();return;}
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
static void menu_input(uint8_t pressed){uint8_t ids[16],count,item;if(!(pressed&(J_UP|J_DOWN|CF_CONTROL_INTERACT|CF_CONTROL_CLOSE|CF_CONTROL_INVENTORY)))return;count=menu_items(ids);if(pressed&CF_CONTROL_CLOSE){menu_active=0;restore_view();return;}if(menu_storage&&!menu_readonly&&(pressed&CF_CONTROL_INVENTORY)){menu_side=!menu_side;menu_choice=0;menu_draw();return;}if(count){if(pressed&J_UP)menu_choice=menu_choice?menu_choice-1:count-1;if(pressed&J_DOWN)menu_choice=(menu_choice+1)%count;if(pressed&CF_CONTROL_INTERACT){item=ids[menu_choice];if(menu_storage&&!menu_readonly)game.variables[31]=cf_storage_transfer(&game,&cf_game_def,menu_source(),menu_side?menu_storage:0,item,1);else if(cf_game_execute(&game,&cf_game_def,&cf_item_use[item])&&!menu_readonly&&cf_item_consumable[item])cf_storage_change(&game,&cf_game_def,0,item,-1);}if(game.request_prompt==255&&game.request_scene==255&&game.request_area==255&&game.request_storage==255&&!game.request_world&&!game.keyboard_active&&game.request_menu==255&&!game.request_return&&game.request_save==255)menu_draw();}}
#if CF_FEATURE_KEYBOARD
static void keyboard_draw(void){begin_text();cf_render_keyboard(&render_context,&cf_game_def);}
#endif
static void sync_position(void){if(game.menus.depth||game.restoring)return;game.position.scene=active_scene;
#if CF_WORLD
 if(scene->world&&area){game.position.area=(uint8_t)(area-cf_areas);game.position.x=world.x;game.position.y=world.y;game.position.facing_x=world.facing_x;game.position.facing_y=world.facing_y;}else game.position.area=255;
#endif
}
static void modal_requests(void){uint8_t step,action,ok,id;CFMenuFrame frame;sync_position();for(step=0;step<8;++step){
 if(game.request_save!=255){action=game.request_save;game.request_save=255;ok=0;
#if CF_SAVE_ENABLED
 ENABLE_RAM_MBC5;SWITCH_RAM(0);ok=cf_save_action(&game,&cf_game_def,(volatile uint8_t *)0xa000,action,game.save_slot);DISABLE_RAM_MBC5;
#endif
 cf_game_save_result(&game,&cf_game_def,action,ok);continue;}
 if(game.request_return){game.request_return=0;if(cf_menu_pop(&game.menus,&frame)){active_scene=frame.scene;scene=&cf_scenes[active_scene];screen_cursor=frame.cursor;dialog=frame.dialog;menu_active=frame.storage!=255;menu_storage=frame.storage;menu_side=frame.side;menu_choice=frame.item;menu_readonly=frame.readonly;
#if CF_WORLD
 if(scene->world){world_tiles();if(CF_COLOR&&_cpu==CGB_TYPE)set_bkg_palette(0,1,area->tile_palette);}else
#endif
 if(scene->image)bitmap_load(scene->image,128);game.variables[31]=1;cf_game_continue(&game,&cf_game_def);restore_view();}else game.variables[31]=0;continue;}
 if(game.request_menu!=255){id=game.request_menu;game.request_menu=255;frame.scene=active_scene;frame.cursor=screen_cursor;frame.dialog=dialog;frame.storage=menu_active?menu_storage:255;frame.side=menu_side;frame.item=menu_choice;frame.readonly=menu_readonly;
 if(id>=CF_SCENE_COUNT||!cf_menu_push(&game.menus,&frame)){game.variables[31]=0;cf_game_continue(&game,&cf_game_def);continue;}active_scene=id;scene=&cf_scenes[id];dialog=menu_active=0;screen_cursor=255;if(scene->image)bitmap_load(scene->image,128);screen_select(1);screen_draw();cf_game_execute(&game,&cf_game_def,&scene->enter);continue;}
 break;}}
static void process_requests(void){uint8_t i;modal_requests();for(i=0;i<8&&(game.request_scene<CF_SCENE_COUNT||game.request_area!=255);++i){if(game.request_scene<CF_SCENE_COUNT){scene_load(game.request_scene,255);if(game.restoring){
#if CF_WORLD
 world.x=game.position.x;world.y=game.position.y;world.facing_x=game.position.facing_x;world.facing_y=game.position.facing_y;cf_world_camera(&world,CF_MAP_W,CF_MAP_H);
#endif
 game.restoring=0;restore_view();}}else{uint8_t target=game.request_area;game.request_area=255;area_enter(target);}}game.request_scene=game.request_area=255;
    if(game.request_world){game.request_world=0;game.request_prompt=game.request_storage=255;menu_active=dialog=0;touch_cooldown=60;restore_view();return;}
for(i=0;i<4&&game.request_prompt<CF_PROMPT_COUNT;++i){uint8_t p=game.request_prompt;game.request_prompt=255;menu_active=0;cf_story_enter(&state,p,&game,&cf_game_def,cf_enter_scripts);cf_story_normalize(&state,cf_prompts,cf_choice_rules,&game);
    dialog=1;
    if(game.request_prompt==255&&!game.request_world&&game.request_storage==255&&game.request_scene==255&&!game.keyboard_active&&game.request_menu==255)draw();}game.request_prompt=255;
    if(game.request_world){game.request_world=0;game.request_storage=255;menu_active=dialog=0;touch_cooldown=60;restore_view();return;}
    if(game.request_storage<cf_game_def.storage_count){menu_storage=game.request_storage;menu_readonly=game.request_readonly;game.request_readonly=0;game.request_storage=255;menu_active=1;menu_side=menu_choice=0;menu_draw();}if(game.keyboard_active&&game.dirty)keyboard_draw();}
void main(void) {
    uint8_t held,pressed,keys,ticks;
#if CF_WORLD
    uint8_t i,step,contact,moving,direction;int16_t old_x,old_y;int8_t dx,dy;
#endif
    cf_prompt_bank=CF_PROMPT_BANK;cf_enter_bank=CF_ENTER_BANK;cf_rule_bank=CF_RULE_BANK;
    cf_game_init(&game,&cf_game_def);
    render_context.game=&game;render_context.body=body_text;render_context.source=screen_text;render_context.video=&video;
    memcpy(render_context.control_hint,CF_CONTROL_HINT,sizeof CF_CONTROL_HINT);memcpy(render_context.modal_hint,CF_MODAL_HINT,sizeof CF_MODAL_HINT);
    DISPLAY_OFF;LCDC_REG|=LCDCF_BG8000;bitmap_load(&cf_splash,0);set_bkg_tiles(0,0,20,18,screen_map);BGP_REG=OBP0_REG=0xe4;if(CF_COLOR&&_cpu==CGB_TYPE){VBK_REG=1;fill_bkg_rect(0,0,32,32,0);VBK_REG=0;}move_bkg(0,0);SHOW_BKG;DISPLAY_ON;{uint16_t wait=CF_SPLASH_TICKS;while(wait--)vsync();}
    DISPLAY_OFF;LCDC_REG&=~LCDCF_BG8000;set_1bpp_colors(3,0);set_bkg_1bpp_data(0,96,cf_font);fill_bkg_rect(0,0,32,32,0);DISPLAY_ON;scene_load(CF_DEFAULT_SCENE,255);input_previous=joypad();input_held=input_previous;input_pressed=pending_ticks=0;add_VBL(sample_input);
    process_requests();
    while(1) {
        vsync();CRITICAL {held=input_held;pressed=input_pressed;input_pressed=0;ticks=pending_ticks;pending_ticks=0;}keys=0;
        {while(ticks--)cf_game_tick(&game,&cf_game_def,dialog||menu_active||game.keyboard_active||game.menus.depth);process_requests();if(game.dirty){if(game.keyboard_active)keyboard_draw();else if(menu_active)menu_draw();else if(dialog){cf_story_normalize(&state,cf_prompts,cf_choice_rules,&game);draw();}else if(!scene->world)screen_draw();game.dirty=0;}}
#if CF_FEATURE_KEYBOARD
        if(game.keyboard_active){uint8_t previous=game.keyboard_cursor;keys=0;if(pressed&J_UP)keys|=CF_KEY_UP;if(pressed&J_DOWN)keys|=CF_KEY_DOWN;if(pressed&J_LEFT)keys|=CF_KEY_LEFT;if(pressed&J_RIGHT)keys|=CF_KEY_RIGHT;if(pressed&CF_CONTROL_INTERACT)keys|=CF_KEY_ACCEPT;if(pressed&CF_CONTROL_CLOSE)keys|=CF_KEY_CANCEL;
 if(keys){cf_keyboard_input(&game,&cf_game_def,keys);if(game.keyboard_active&&!(keys&(CF_KEY_ACCEPT|CF_KEY_CANCEL))){const CFKeyboardDef *k=&cf_keyboards[game.keyboard_id];glyph(k->x+(previous%k->columns)*2,k->y+previous/k->columns,' ');glyph(k->x+(game.keyboard_cursor%k->columns)*2,k->y+game.keyboard_cursor/k->columns,'>');game.dirty=0;}else if(game.keyboard_active)keyboard_draw();else restore_view();process_requests();}continue;}
#endif
        if(scene->world&&!dialog&&!menu_active&&!game.menus.depth){uint8_t handled=cf_game_buttons(&game,&cf_game_def,pressed);if(handled){pressed&=~handled;process_requests();continue;}}
        if((pressed&J_START)&&CF_PAUSE_MENU!=255&&!game.menus.depth&&!(dialog&&prompt_current()->modal)){cf_game_execute(&game,&cf_game_def,&cf_pause_program);process_requests();continue;}
        if(menu_active){menu_input(pressed);process_requests();continue;}
        if(!scene->world&&!dialog){CFScreenElement element;if(pressed&J_UP)screen_select(-1);if(pressed&J_DOWN)screen_select(1);if(pressed&(J_UP|J_DOWN)){uint8_t marker;for(marker=0;marker<scene->count;++marker){screen_element(marker,&element);if(element.type==1)glyph(element.x,element.y,cf_game_condition(&game,&element.condition)?(marker==screen_cursor?'>':' '):'x');}}if((pressed&CF_CONTROL_INTERACT)&&screen_cursor<scene->count){screen_element(screen_cursor,&element);if(cf_game_condition(&game,&element.condition))cf_game_execute(&game,&cf_game_def,&element.action);}if(pressed&CF_CONTROL_CLOSE)cf_game_execute(&game,&cf_game_def,&scene->back);process_requests();if(game.dirty&&!dialog&&!menu_active&&!scene->world)screen_draw();continue;}
        if(dialog&&prompt_current()->modal)pressed&=~(CF_CONTROL_CLOSE|CF_CONTROL_INVENTORY|CF_CONTROL_RESTART);
        if(dialog&&(pressed&CF_CONTROL_CLOSE)){dialog=0;restore_view();continue;}
        if(pressed & CF_CONTROL_INVENTORY){
#if CF_WORLD
            if(dialog&&prompt_current()->modal)continue;
#endif
            game.request_storage=0;game.request_readonly=0;process_requests();continue;}
#if CF_WORLD
        if(scene->world&&(pressed & CF_CONTROL_RESTART)){world_reset();continue;}
        if(scene->world&&!dialog) {
            direction=held&(J_LEFT|J_RIGHT|J_UP|J_DOWN);if(!direction)direction=pressed&(J_LEFT|J_RIGHT|J_UP|J_DOWN);dx=direction & J_LEFT ? -1 : direction & J_RIGHT ? 1 : 0;dy=direction & J_UP ? -1 : direction & J_DOWN ? 1 : 0;
            old_x=world.x;old_y=world.y;
            if(++player_clock>=area->player_behavior.interval){player_clock=0;for(step=0;step<area->player_behavior.speed;++step)cf_world_move(&world,CF_MAP_W,CF_MAP_H,cf_solid,dx,dy);}
            contact=255;if(touch_cooldown)--touch_cooldown;
            if(!dx&&!dy)player_moving=0;else if(!player_clock)player_moving=(world.x!=old_x||world.y!=old_y);
            moving=player_moving;
            cf_animation_update_facing(&player_animation,area->animations,area->animation_count,&area->bindings,moving,(pressed & CF_CONTROL_INTERACT)!=0,world.facing_x,world.facing_y);
            for(i=0;i<area->actor_count;++i){if(!actor_health[i])continue;old_x=actors[i].body.x;old_y=actors[i].body.y;
                if(cf_actor_tick(&actors[i],&area->actors[i].behavior,&world,CF_MAP_W,CF_MAP_H,cf_solid)&&!touch_cooldown)contact=area->actors[i].behavior.touch_prompt;
                cf_animation_update_facing(&actor_animations[i],area->actors[i].animations,area->actors[i].animation_count,&area->actors[i].bindings,actors[i].moving,0,actors[i].body.facing_x,actors[i].body.facing_y);
            }
            if(area->step_count){int16_t cell=(world.y+4)/8*CF_MAP_W+(world.x+4)/8;if(cf_game_step(&step_state,&game,&cf_game_def,area->steps,area->step_count,cell,cf_resource_read(cf_map,cell))){process_requests();if(dialog||menu_active)continue;}}
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

        if(cf_story_step(&state,cf_prompts,CF_PROMPT_COUNT,CF_ENTRY,keys,&game,&cf_game_def,cf_choice_rules,cf_enter_scripts)){if(keys&(CF_KEY_UP|CF_KEY_DOWN))cursor_draw();else if(game.request_prompt==255&&!game.request_world&&game.request_storage==255&&game.request_scene==255&&!game.keyboard_active&&game.request_menu==255)draw();}
        process_requests();
    }
}
