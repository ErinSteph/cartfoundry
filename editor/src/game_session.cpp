#include "game_session.h"
#include <cmath>
#include "sound_compile.h"
namespace cf {
void GameSession::configure(const Project &p,bool restart){project=p;initial.clear();for(auto &v:p.variables)initial<<int16_t(v.initial);limits.clear();for(auto &i:p.items)limits<<uint8_t(i.stackLimit);counts.resize(p.storages.size());stores.resize(p.storages.size());for(int s=0;s<p.storages.size();++s){counts[s].fill(0,p.items.size());for(auto i=p.storages[s].contents.begin();i!=p.storages[s].contents.end();++i){int index=p.itemIndex(i.key());if(index>=0)counts[s][index]=i.value();}stores[s]={uint8_t(p.storages[s].capacity),counts[s].data()};}definition={uint8_t(initial.size()),uint8_t(limits.size()),uint8_t(stores.size()),initial.data(),limits.data(),stores.data()};GameCompiler compiler(p);QString error;timerCode.resize(p.timers.size());timerDefs.resize(p.timers.size());for(int i=0;i<p.timers.size();++i){auto &t=p.timers[i];if(!compiler.program(t.source,timerCode[i],error))timerCode[i].clear();timerDefs[i]={uint16_t(t.interval),uint8_t(t.repeat),uint8_t(t.autostart),uint8_t(t.pauseDialogue),{uint8_t(timerCode[i].size()),timerCode[i].data()}};}cf_sound_stop(&state.sound);soundNotes.resize(p.sounds.size()*4);soundTracks.resize(p.sounds.size());soundClips.resize(p.sounds.size());for(int i=0;i<p.sounds.size();++i){auto &c=p.sounds[i];CFSoundClip out{};out.count=c.notes.size();out.duty=c.duty;out.volume=c.volume;
 if(c.channels.isEmpty()){auto &notes=soundNotes[i*4];notes.clear();SoundTrack legacy;for(auto &n:c.notes)notes<<compileSoundNote(n,legacy,0);out.notes=notes.data();}
 else{soundTracks[i].resize(4);for(int ch=0;ch<4;++ch){auto &t=c.channels[ch];auto &notes=soundNotes[i*4+ch];notes.clear();for(auto &n:t.notes)notes<<compileSoundNote(n,t,ch);auto track=compileSoundTrack(t);track.notes=notes.data();soundTracks[i][ch]=track;if(t.enabled)out.mask|=1<<ch;}out.tracks=soundTracks[i].data();out.duration=soundDuration(c);out.master=(c.masterLeft<<4)|c.masterRight;out.effect=c.effect;}soundClips[i]=out;}
 definition.sound_count=soundClips.size();definition.sounds=soundClips.data();state.sound.clips=definition.sounds;state.sound.count=definition.sound_count;definition.seed=p.rngSeed;definition.timer_count=timerDefs.size();definition.timers=timerDefs.data();prompts.fill({},p.prompts.size());rules.fill({},p.prompts.size()*3);enter.resize(p.prompts.size());enterCode.resize(p.prompts.size());choiceCode.resize(p.prompts.size()*3);
for(int i=0;i<p.prompts.size();++i){auto &prompt=p.prompts[i];prompts[i].choice_count=prompt.choices.size();if(!compiler.program(prompt.onEnter,enterCode[i],error))enterCode[i].clear();enter[i]={uint8_t(enterCode[i].size()),enterCode[i].data()};for(int c=0;c<prompt.choices.size();++c){int index=i*3+c;compiler.condition(prompt.choices[c].condition,rules[index].condition,error);if(!compiler.program(prompt.choices[c].actions,choiceCode[index],error))choiceCode[index].clear();rules[index].actions={uint8_t(choiceCode[index].size()),choiceCode[index].data()};int target=p.indexOf(prompt.choices[c].target);prompts[i].targets[c]=target<0?255:target;}}
if(restart)reset();}
void GameSession::reset(){cf_game_init(&state,&definition);for(auto &p:phase)p=0;noiseLfsr=0x7fff;lastNoiseTrigger=0;}
void GameSession::enterPrompt(CFState &flow,int index){if(index>=0&&index<prompts.size()){cf_story_enter(&flow,index,&state,&definition,enter.data());cf_story_normalize(&flow,prompts.data(),rules.data(),&state);}}
bool GameSession::step(CFState &flow,int key){return cf_story_step(&flow,prompts.data(),prompts.size(),qMax(0,project.indexOf(project.entry)),key,&state,&definition,rules.data(),enter.data());}
bool GameSession::enabled(int p,int c)const{return p>=0&&p<prompts.size()&&c>=0&&c<prompts[p].choice_count&&cf_game_condition(&state,&rules[p*3+c].condition);}
bool GameSession::run(const QString &source){QVector<CFInstruction> code;QString error;if(!GameCompiler(project).program(source,code,error))return false;CFProgram p{uint8_t(code.size()),code.data()};return cf_game_execute(&state,&definition,&p);}
}

namespace cf {
QByteArray GameSession::renderAudio(int frames){
 QByteArray pcm(frames*4,0);auto *out=reinterpret_cast<qint16 *>(pcm.data());
 for(int n=0;n<frames;++n){int left=0,right=0;for(int c=0;c<4;++c){auto &v=state.sound.voices[c];if(!v.frequency||!v.instrument.dac)continue;double sample=0;
 if(c<2){phase[c]+=double(v.frequency)/48000;phase[c]-=int(phase[c]);const double duty[]={0.125,0.25,0.5,0.75};sample=(phase[c]<duty[v.duty&3]?1:-1)*v.volume*180;}
 else if(c==2){phase[c]+=double(v.frequency)/48000;phase[c]-=int(phase[c]);int index=qMin(31,int(phase[c]*32));int byte=v.instrument.wave[index/2];int value=index%2?byte&15:byte>>4;int level=v.instrument.wave_level;sample=level?(value-7.5)*320/(1<<(level-1)):0;}
 else{if(lastNoiseTrigger!=v.trigger_serial){noiseLfsr=0x7fff;phase[c]=0;lastNoiseTrigger=v.trigger_serial;}int code=v.frequency-1,divider=code&7;double rate=524288.0/(divider?divider:0.5)/std::pow(2.0,(code>>4)+1);if((code>>4)>=14)rate=0;phase[c]+=rate/48000;while(phase[c]>=1){phase[c]-=1;int bit=(noiseLfsr^(noiseLfsr>>1))&1;noiseLfsr=(noiseLfsr>>1)|(bit<<14);if(code&8)noiseLfsr=(noiseLfsr&~64)|(bit<<6);}sample=(noiseLfsr&1?-1:1)*v.volume*180;}
 if(v.instrument.pan&1)left+=int(sample);if(v.instrument.pan&2)right+=int(sample);
 }out[n*2]=qBound(-32768,left*(((state.sound.master>>4)&7)+1)/8,32767);out[n*2+1]=qBound(-32768,right*((state.sound.master&7)+1)/8,32767);}
 return pcm;
}
void GameSession::tick(bool inDialogue){audio.write(renderAudio());cf_game_tick(&state,&definition,inDialogue);}
}
