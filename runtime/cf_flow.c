#include "cf_flow.h"
#ifdef __TARGET_gb
#include "cf_bank.h"
#endif
uint8_t cf_prompt_bank;
void cf_prompt_read(const CFPrompt *p,uint8_t index,CFPrompt *out) CF_FLOW_CALL {
#ifdef __TARGET_gb
cf_bank_copy(cf_prompt_bank,(const uint8_t *)&p[index],(uint8_t *)out,sizeof *out);
#else
*out=p[index];
#endif
}
uint8_t cf_step(CFState *state, const CFPrompt *prompts, uint8_t count, uint8_t entry, uint8_t keys) CF_FLOW_CALL {
    CFPrompt snapshot;const CFPrompt *p=&snapshot;
    uint8_t target;
    if (!count || entry >= count) return 0;
    if (state->prompt >= count || (keys & CF_KEY_RESET)) {
        state->prompt = entry; state->choice = 0; return 1;
    }
    cf_prompt_read(prompts,state->prompt,&snapshot);
    if (!p->choice_count || p->choice_count > CF_MAX_CHOICES) return 0;
    if (state->choice >= p->choice_count) state->choice = 0;
    if (keys & CF_KEY_UP) {
        state->choice = state->choice ? state->choice - 1 : p->choice_count - 1; return 1;
    }
    if (keys & CF_KEY_DOWN) { state->choice = (state->choice + 1) % p->choice_count; return 1; }
    if (keys & CF_KEY_ACCEPT) {
        target = p->targets[state->choice];
        if (target >= count) return 0;
        state->prompt = target; state->choice = 0; return 1;
    }
    return 0;
}

void cf_text_layout(const CFPrompt *p,CFTextLayout *out) CF_FLOW_CALL {int16_t cols=p->box_w-2*p->padding,body=p->padding+((p->text_flags&2)?2:0),footer=p->box_h-1-p->padding,choice=footer+((p->text_flags&4)?0:1)-p->choice_count*p->choice_spacing,rows=choice-1-body;out->columns=cols>0?cols:1;out->body_rows=rows>0?rows:0;out->body_row=body;out->choice_row=choice>0?choice:0;out->footer_row=footer>0?footer:0;}

uint8_t cf_menu_push(CFMenuStack *s,const CFMenuFrame *frame) CF_FLOW_CALL {if(s->depth>=4)return 0;s->frames[s->depth++]=*frame;return 1;}
uint8_t cf_menu_pop(CFMenuStack *s,CFMenuFrame *frame) CF_FLOW_CALL {if(!s->depth)return 0;*frame=s->frames[--s->depth];return 1;}
