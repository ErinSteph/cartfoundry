#ifdef __TARGET_gb
#pragma bank 2
#endif
#include "cf_render.h"
#include "cf_bank.h"
static void render_text(const CFRenderContext *r,uint8_t x,uint8_t y,const char *text,uint8_t limit){while(*text&&limit--)cf_video_glyph(r->video,x++,y,*text++);}
static const char *prompt_text(const CFRenderContext *r,const CFResource *text){uint16_t n=text->size<512?text->size:511;cf_resource_copy(text,0,(uint8_t *)r->source,n);r->source[n]=0;return r->source;}
void cf_render_dialogue(const CFRenderContext *r,const CFPrompt *p,const CFChoiceRule *rules,uint8_t choice) BANKED {
    uint8_t x,y,i,choice_y,body_y,columns;const char *c;CFTextLayout layout;
    cf_text_layout(p,&layout);if(p->text_palette)cf_video_palette(r->video,p->text_palette);
    for(y=0;y<p->box_h;++y) for(x=0;x<p->box_w;++x) {
        uint8_t edge_x=x==0 || x==p->box_w-1,edge_y=y==0 || y==p->box_h-1;
        cf_video_glyph(r->video,p->box_x+x,p->box_y+y,!(p->text_flags&1)?' ':edge_x && edge_y ? '+' : edge_x ? '|' : edge_y ? '-' : ' ');
    }
    columns=layout.columns;choice_y=p->box_y+layout.choice_row;
    if(p->text_flags&2)render_text(r,p->box_x+p->padding,p->box_y+p->padding,prompt_text(r,p->title),columns);
    cf_game_text(r->game,prompt_text(r,p->body),r->body,512,columns);
    y=p->box_y+layout.body_row;c=r->body;body_y=y+layout.body_rows;
    while(*c&&y<body_y){uint8_t length=0;while(c[length]&&c[length]!='\n')++length;x=p->box_x+p->padding;if(p->text_align==1)x+=(columns-length)/2;else if(p->text_align==2)x+=columns-length;while(*c&&*c!='\n')cf_video_glyph(r->video,x++,y,*c++);if(*c=='\n')++c;++y;}
    for(i=0;i<p->choice_count;++i){cf_video_glyph(r->video,p->box_x+p->padding,choice_y+i*p->choice_spacing,cf_game_condition(r->game,&rules[i].condition)?(i==choice?p->cursor:' '):p->disabled_marker);render_text(r,p->box_x+p->padding+1,choice_y+i*p->choice_spacing,prompt_text(r,p->choices[i]),columns-1);}
    if(p->text_flags&4)render_text(r,p->box_x+p->padding,p->box_y+layout.footer_row,p->footer->size>1?prompt_text(r,p->footer):p->modal?r->modal_hint:r->control_hint,columns);
    if(p->portrait)cf_video_portrait(r->video,p->portrait);
    cf_video_commit(r->video);
}
void cf_render_screen(const CFRenderContext *r,const CFScene *scene,uint8_t screen_cursor) BANKED {uint8_t i,x,y,row,length,columns;CFScreenElement element;const CFScreenElement *e=&element;const char *c;for(i=0;i<scene->count;++i){cf_bank_copy(scene->elements_bank,(const uint8_t *)&scene->elements[i],(uint8_t *)&element,sizeof element);if(e->type==3)continue;for(y=0;y<e->h;++y)for(x=0;x<e->w;++x)cf_video_glyph(r->video,e->x+x,e->y+y,e->type==2?((x==0||x==e->w-1)?((y==0||y==e->h-1)?'+':'|'):(y==0||y==e->h-1)?'-':' '):' ');if(e->type==2)continue;columns=e->w-(e->type==1);cf_resource_copy(e->text,0,(uint8_t *)r->source,e->text->size<512?e->text->size:512-1);r->source[512-1]=0;cf_game_text(r->game,r->source,r->body,512,columns);c=r->body;row=0;while(*c&&row<e->h){length=0;while(c[length]&&c[length]!='\n')++length;x=e->x+(e->type==1);if(e->align==1)x+=(columns-length)/2;else if(e->align==2)x+=columns-length;while(*c&&*c!='\n')cf_video_glyph(r->video,x++,e->y+row,*c++);if(*c=='\n')++c;++row;}if(e->type==1)cf_video_glyph(r->video,e->x,e->y,cf_game_condition(r->game,&e->condition)?(screen_cursor==i?'>':' '):'x');}cf_video_commit(r->video);}
#if CF_FEATURE_KEYBOARD
void cf_render_keyboard(const CFRenderContext *r,const CFGameDef *definition) BANKED{uint8_t x,y;cf_keyboard_render(r->game,definition,r->body);for(y=0;y<18;++y)for(x=0;x<20;++x)cf_video_glyph(r->video,x,y,r->body[(uint16_t)y*20+x]);cf_video_commit(r->video);}
#endif
