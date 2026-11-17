#include "cf_game.h"
static uint8_t compare(int16_t a,int16_t b,uint8_t op){switch(op){case 0:return a==b;case 1:return a!=b;case 2:return a<b;case 3:return a<=b;case 4:return a>b;case 5:return a>=b;}return 0;}
static int16_t operand(const CFGameState *s,int16_t v,uint8_t variable){return variable&&v>=0&&v<32?s->variables[v]:v;}
void cf_game_init(CFGameState *s,const CFGameDef *d){uint8_t i,j;for(i=0;i<32;++i)s->variables[i]=i<d->variable_count?d->initial[i]:0;s->variables[31]=1;for(i=0;i<8;++i)for(j=0;j<16;++j)s->counts[i][j]=i<d->storage_count&&j<d->item_count?d->storages[i].initial[j]:0;s->request_storage=s->request_prompt=255;s->request_world=s->request_readonly=s->dirty=0;s->random=d->seed?d->seed:0xace1;for(i=0;i<8;++i){s->timer_remaining[i]=i<d->timer_count?d->timers[i].interval:0;s->timer_active[i]=i<d->timer_count?d->timers[i].autostart:0;}}
uint8_t cf_game_condition(const CFGameState *s,const CFCondition *c){if(!c->type)return 1;if(c->type==1&&c->a<32)return compare(s->variables[c->a],operand(s,c->value,c->rhs_variable),c->compare);if(c->type==2&&c->a<8&&c->b<16)return compare((int16_t)s->counts[c->a][c->b],operand(s,c->value,c->rhs_variable),c->compare);return 0;}
static uint8_t fits(const CFGameState *s,const CFGameDef *d,uint8_t storage,uint8_t item,uint16_t count){uint8_t i;uint16_t slots=0;for(i=0;i<d->item_count;++i){uint16_t n=i==item?count:s->counts[storage][i];uint8_t limit=d->stack_limits[i];if(!limit)return 0;slots+=(n+limit-1)/limit;}return slots<=d->storages[storage].capacity;}
uint8_t cf_storage_change(CFGameState *s,const CFGameDef *d,uint8_t storage,uint8_t item,int16_t amount){int32_t count;if(storage>=d->storage_count||item>=d->item_count)return 0;count=(int32_t)s->counts[storage][item]+amount;if(count<0||count>32767||!fits(s,d,storage,item,(uint16_t)count))return 0;s->counts[storage][item]=(uint16_t)count;s->dirty=1;return 1;}
uint8_t cf_storage_transfer(CFGameState *s,const CFGameDef *d,uint8_t from,uint8_t to,uint8_t item,uint16_t amount){if(from>=d->storage_count||to>=d->storage_count||item>=d->item_count||!amount||s->counts[from][item]<amount)return 0;if(from==to)return 1;if((uint32_t)s->counts[to][item]+amount>32767||!fits(s,d,to,item,s->counts[to][item]+amount))return 0;s->counts[from][item]-=amount;s->counts[to][item]+=amount;s->dirty=1;return 1;}
uint8_t cf_game_execute(CFGameState *s,const CFGameDef *d,const CFProgram *p){uint8_t pc=0,steps=0,all=1;while(pc<p->count&&steps++<128){const CFInstruction *i=&p->code[pc];uint8_t ok=1;int16_t value=operand(s,i->value,i->op==CF_OP_MOVE?i->d:i->c);++pc;switch(i->op){
case CF_OP_SEED:s->random=value?(uint16_t)value:0xace1;break;
case CF_OP_TIMER:if(i->a<d->timer_count){if(i->b==1)s->timer_active[i->a]=0;else{s->timer_remaining[i->a]=d->timers[i->a].interval;s->timer_active[i->a]=i->b==0;}}else ok=0;break;
case CF_OP_RANDOM:if(i->a<d->variable_count){uint16_t max=(uint16_t)i->b|((uint16_t)i->c<<8);s->variables[i->a]=i->value+cf_game_random(s,max-i->value);}else ok=0;break;
case CF_OP_RESUME:s->request_world=1;break;
case CF_OP_SET:if(i->a<d->variable_count)s->variables[i->a]=value;else ok=0;break;
case CF_OP_SUB:
case CF_OP_ADD:if(i->a<d->variable_count){int32_t n=(int32_t)s->variables[i->a]+(i->op==CF_OP_SUB?-(int32_t)value:value);s->variables[i->a]=n>32767?32767:n<-32768?-32768:(int16_t)n;}else ok=0;break;
case CF_OP_GIVE:ok=value>0&&cf_storage_change(s,d,i->a,i->b,value);break;
case CF_OP_TAKE:ok=value>0&&cf_storage_change(s,d,i->a,i->b,-value);break;
case CF_OP_MOVE:ok=value>0&&cf_storage_transfer(s,d,i->a,i->b,i->c,(uint16_t)value);break;
case CF_OP_VIEW:
case CF_OP_OPEN:if(i->a<d->storage_count){s->request_storage=i->a;s->request_readonly=i->op==CF_OP_VIEW;}else ok=0;break;
case CF_OP_DIALOGUE:s->request_prompt=i->a;break;
case CF_OP_IF_VAR:if(!compare(s->variables[i->a],operand(s,i->value,i->c),i->b))pc=i->d;continue;
case CF_OP_IF_ITEM:if(!compare((int16_t)s->counts[i->a][i->b],operand(s,i->value,(i->c&128)!=0),i->c&127))pc=i->d;continue;
case CF_OP_JUMP:pc=i->a;continue;
default:ok=0;break;}
s->variables[31]=ok;s->dirty=1;all&=ok;}return all;}
void cf_story_enter(CFState *flow,uint8_t prompt,CFGameState *s,const CFGameDef *d,const CFProgram *enter){flow->prompt=prompt;flow->choice=0;cf_game_execute(s,d,&enter[prompt]);}
void cf_story_normalize(CFState *f,const CFPrompt *p,const CFChoiceRule *rules,const CFGameState *s){uint8_t i;if(f->choice<p[f->prompt].choice_count&&cf_game_condition(s,&rules[(uint16_t)f->prompt*3+f->choice].condition))return;f->choice=255;for(i=0;i<p[f->prompt].choice_count;++i)if(cf_game_condition(s,&rules[(uint16_t)f->prompt*3+i].condition)){f->choice=i;break;}}
uint8_t cf_story_step(CFState *f,const CFPrompt *p,uint8_t count,uint8_t entry,uint8_t keys,CFGameState *s,const CFGameDef *d,const CFChoiceRule *rules,const CFProgram *enter){uint8_t i,target;if(!count||entry>=count)return 0;if(f->prompt>=count||(keys&CF_KEY_RESET)){cf_story_enter(f,entry,s,d,enter);cf_story_normalize(f,p,rules,s);return 1;}cf_story_normalize(f,p,rules,s);if(f->choice==255)return 0;
if(keys&(CF_KEY_UP|CF_KEY_DOWN)){for(i=0;i<3;++i){cf_step(f,p,count,entry,keys&(CF_KEY_UP|CF_KEY_DOWN));if(cf_game_condition(s,&rules[(uint16_t)f->prompt*3+f->choice].condition))break;}return 1;}
if(keys&CF_KEY_ACCEPT){target=p[f->prompt].targets[f->choice];if(target>=count)return 0;cf_game_execute(s,d,&rules[(uint16_t)f->prompt*3+f->choice].actions);cf_story_enter(f,target,s,d,enter);cf_story_normalize(f,p,rules,s);return 1;}return 0;}

uint16_t cf_game_random(CFGameState *s,uint16_t maximum){uint16_t n=s->random;if(!n)n=0xace1;n^=n<<7;n^=n>>9;n^=n<<8;s->random=n;return maximum==65535?n:n%(maximum+1);}
void cf_game_step_init(CFStepState *s,int16_t cell){uint8_t i;s->cell=cell;for(i=0;i<8;++i)s->steps[i]=0;}
uint8_t cf_game_step(CFStepState *step,CFGameState *state,const CFGameDef *def,const CFStepRule *rules,uint8_t count,int16_t cell,uint8_t tile){uint8_t i;if(cell==step->cell)return 0;step->cell=cell;for(i=0;i<count&&i<8;++i)if(rules[i].tile==tile){if(++step->steps[i]<rules[i].every)continue;step->steps[i]=0;if(cf_game_random(state,99)<rules[i].chance){cf_game_execute(state,def,&rules[i].program);return 1;}}return 0;}
/* Inline byte 31 followed by variable index+1; ASCII remains ordinary text. */
void cf_game_text(const CFGameState *s,const char *in,char *out,uint16_t capacity,uint8_t columns){uint16_t pos=0;uint8_t col=0;if(!capacity)return;while(*in&&pos+1<capacity){char word[64];uint8_t len=0,k;if(*in=='\n'){out[pos++]='\n';col=0;++in;continue;}if(*in==' '||*in=='\t'){++in;continue;}while(*in&&*in!=' '&&*in!='\t'&&*in!='\n'){if((uint8_t)*in==31){char digits[6];uint8_t n=0,index=(uint8_t)*++in;int16_t value=index>=1&&index<=32?s->variables[index-1]:0;uint16_t magnitude=value<0?(uint16_t)(-(int32_t)value):value;if(value<0&&len<63)word[len++]='-';do{digits[n++]='0'+magnitude%10;magnitude/=10;}while(magnitude);while(n&&len<63)word[len++]=digits[--n];if(*in)++in;}else{if(len<63)word[len++]=*in;++in;}}if(col&&col+1+len>columns){out[pos++]='\n';col=0;}if(col&&pos+1<capacity){out[pos++]=' ';++col;}for(k=0;k<len&&pos+1<capacity;++k){out[pos++]=word[k];++col;}}out[pos]=0;}

void cf_game_tick(CFGameState *s,const CFGameDef *d,uint8_t in_dialogue){uint8_t i;for(i=0;i<d->timer_count&&i<8;++i)if(s->timer_active[i]&&!(in_dialogue&&d->timers[i].pause_dialogue)){if(s->timer_remaining[i])--s->timer_remaining[i];if(!s->timer_remaining[i]){s->timer_remaining[i]=d->timers[i].interval;s->timer_active[i]=d->timers[i].repeat;cf_game_execute(s,d,&d->timers[i].program);}}}
