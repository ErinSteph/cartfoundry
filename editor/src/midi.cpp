#include "midi.h"
#include "sound_compile.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <QQueue>
namespace cf {
namespace {
constexpr int MaxTicks=64*255,MaxEvents=200000;
struct Reader {
 const QByteArray &bytes;qsizetype pos=0,end;bool valid=true;
 explicit Reader(const QByteArray &b):bytes(b),end(b.size()){}
 quint8 byte(){if(pos>=end){valid=false;return 0;}return quint8(bytes[pos++]);}
 quint32 be(int n){quint32 result=0;while(n--)result=(result<<8)|byte();return result;}
 QByteArray take(quint32 n){if(n>quint64(end-pos)){valid=false;return {};}auto result=bytes.mid(pos,n);pos+=n;return result;}
 quint32 vlq(){quint32 value=0;for(int n=0;n<4;++n){auto b=byte();value=(value<<7)|(b&127);if(!(b&128))return value;}valid=false;return 0;}
};
struct Event {quint64 tick;int status,a,b;};
struct Note {int pitch,velocity,channel,pan;double start,end=-1;};
int gameTick(double seconds){return int(std::clamp<qint64>(qRound64(seconds*60),0,MaxTicks+1));}
void be(QByteArray &out,quint32 value,int n){while(n--)out.append(char(value>>(n*8)));}
void vlq(QByteArray &out,quint32 value){quint8 data[4];int n=0;data[n++]=value&127;while((value>>=7))data[n++]=(value&127)|128;while(n)out.append(char(data[--n]));}
void meta(QByteArray &out,int kind,const QByteArray &payload){out.append(char(0xff));out.append(char(kind));vlq(out,payload.size());out+=payload;}
void chunk(QByteArray &out,const QByteArray &data){out+="MTrk";be(out,data.size(),4);out+=data;}
int drumPitch(int pitch){if(pitch==35||pitch==36)return 36;if(pitch==37||pitch==38||pitch==39||pitch==40)return 38;if(pitch==42||pitch==44)return 42;if(pitch==46)return 46;if(pitch>=41&&pitch<=50&&pitch!=49)return 45;return 49;}
}
bool importMidi(const QByteArray &bytes,MidiImport &out,QString &error){
 error.clear();auto fail=[&](const QString &why){error="MIDI import: "+why;return false;};
 if(bytes.size()>16*1024*1024)return fail("files must be no larger than 16 MiB.");Reader r(bytes);
 if(r.take(4)!="MThd")return fail("missing Standard MIDI File header.");auto header=r.be(4);if(header<6||header>quint64(r.end-r.pos))return fail("invalid header length.");
 int format=r.be(2),tracks=r.be(2),division=r.be(2);r.take(header-6);
 if(format>1)return fail("type 2 contains independent sequences; export a type 0 or type 1 file first.");
 if(tracks<1||tracks>256||(format==0&&tracks!=1)||!division)return fail("invalid track count or timing division.");
 double tickSeconds=0;if(division&0x8000){int fps=256-(division>>8),sub=division&255;if(!sub||(fps!=24&&fps!=25&&fps!=29&&fps!=30))return fail("unsupported SMPTE time division.");tickSeconds=1.0/((fps==29?30000.0/1001:double(fps))*sub);}
 QVector<Event> events;QString title;int ignored=0,tempoChanges=0,parsedEvents=0;
 for(int track=0;track<tracks;++track){
  if(r.take(4)!="MTrk")return fail("missing or truncated track chunk.");auto length=r.be(4);if(!r.valid||length>quint64(r.end-r.pos))return fail("invalid track length.");auto fileEnd=r.end;r.end=r.pos+length;quint64 tick=0;int running=0;bool ended=false;
  while(r.valid&&r.pos<r.end){if(++parsedEvents>MaxEvents)return fail("too many MIDI events (limit 200,000).");tick+=r.vlq();if(tick>(1ULL<<40))return fail("event timeline is too large.");int status=r.byte();if(status<128){if(!running)return fail("data byte without running status.");--r.pos;status=running;}
   if(status==0xff){running=0;int kind=r.byte();auto size=r.vlq();auto data=r.take(size);if(!r.valid)break;
    if(kind==0x2f){if(size)return fail("invalid end-of-track event.");events<<Event{tick,0,0,0};ended=true;break;}
    if(kind==0x51){if(size!=3)return fail("invalid tempo event.");int tempo=(quint8(data[0])<<16)|(quint8(data[1])<<8)|quint8(data[2]);if(!tempo)return fail("tempo cannot be zero.");events<<Event{tick,0xff,tempo,0};++tempoChanges;}
    else if(kind==3&&title.isEmpty())title=QString::fromUtf8(data).trimmed().left(128);
    else events<<Event{tick,0,0,0};
   }else if(status==0xf0||status==0xf7){running=0;r.take(r.vlq());++ignored;events<<Event{tick,0,0,0};}
   else if(status>=0x80&&status<=0xef){running=status;int a=r.byte(),type=status&0xf0,b=(type==0xc0||type==0xd0)?0:r.byte();if(a>127||b>127)return fail("invalid MIDI channel data.");events<<Event{tick,status,a,b};}
   else return fail("invalid system event in MIDI track.");
  }
  if(!r.valid)return fail("truncated event or invalid variable-length quantity.");if(!ended)return fail("track has no end-of-track event.");r.pos=r.end;r.end=fileEnd;
 }
 if(!r.valid||r.pos!=r.end)return fail("unexpected trailing or truncated data.");
 std::stable_sort(events.begin(),events.end(),[](const Event &a,const Event &b){return a.tick<b.tick;});
 QVector<Note> notes;std::array<std::array<QQueue<int>,128>,16> active;std::array<QVector<int>,16> sustained;bool pedal[16]={};int pan[16];std::fill(std::begin(pan),std::end(pan),3);quint64 last=0;double seconds=0;int tempo=500000,initialTempo=500000;int unmatched=0;
 for(const auto &e:events){seconds+=(e.tick-last)*(tickSeconds?tickSeconds:double(tempo)/1000000/division);last=e.tick;if(e.status==0xff){tempo=e.a;if(e.tick==0)initialTempo=tempo;continue;}if(!e.status)continue;
  int ch=e.status&15,type=e.status&0xf0;auto finish=[&](int id){notes[id].end=seconds;};auto release=[&](int id){if(pedal[ch])sustained[ch]<<id;else finish(id);};
  if(type==0x90&&e.b){notes<<Note{e.a,e.b,ch,pan[ch],seconds};active[ch][e.a].enqueue(notes.size()-1);}
  else if(type==0x80||(type==0x90&&!e.b)){if(active[ch][e.a].isEmpty())++unmatched;else release(active[ch][e.a].dequeue());}
  else if(type==0xb0){if(e.a==64){bool down=e.b>=64;if(pedal[ch]&&!down){for(int id:sustained[ch])finish(id);sustained[ch].clear();}pedal[ch]=down;}
   else if(e.a==10)pan[ch]=e.b<32?1:e.b>95?2:3;
   else if(e.a==120||e.a==123){for(auto &key:active[ch])while(!key.isEmpty()){int id=key.dequeue();if(e.a==120)finish(id);else release(id);}if(e.a==120){for(int id:sustained[ch])finish(id);sustained[ch].clear();}}
   else if(e.a==121){pedal[ch]=false;for(int id:sustained[ch])finish(id);sustained[ch].clear();}
   else ++ignored;
  }else ++ignored;
 }
 if(notes.isEmpty())return fail("the file contains no note-on events.");int unterminated=0;for(auto &n:notes)if(n.end<0){n.end=seconds;++unterminated;}
 MidiImport result;result.sourceNotes=notes.size();auto &clip=result.clip;clip.id="midi_song";clip.name=title.isEmpty()?"Imported MIDI":title;clip.bpm=qBound(30,qRound(60000000.0/initialTempo),300);expandSoundChannels(clip);for(auto &t:clip.channels){t.notes.clear();t.enabled=false;}
 int cursor[4]={},source[4]={-1,-1,-1,-1};int shifted=0,drums=0,overlap=0,capacity=0,stereo=0,splitNotes=0;bool clipped=gameTick(seconds)>MaxTicks;
 // Note-on order is deterministic across tracks; use up to three melodic voices.
 std::stable_sort(notes.begin(),notes.end(),[](const Note &a,const Note &b){return a.start<b.start;});
 for(const auto &n:notes){int start=gameTick(n.start),end=qMin(MaxTicks,qMax(start+1,gameTick(n.end)));if(start>=MaxTicks){++capacity;clipped=true;continue;}int lane=-1;
  if(n.channel==9){if(cursor[3]<=start)lane=3;}else{if(n.channel<3&&cursor[n.channel]<=start)lane=n.channel;for(int i=0;lane<0&&i<3;++i)if(source[i]==n.channel&&cursor[i]<=start)lane=i;for(int i=0;lane<0&&i<3;++i)if(cursor[i]<=start)lane=i;}
  if(lane<0){++overlap;continue;}auto &t=clip.channels[lane];int gap=start-cursor[lane],duration=end-start;int rows=(gap+254)/255+(duration+254)/255;if(t.notes.size()+rows>64){++capacity;continue;}if(duration>255)++splitNotes;
  auto append=[&](int pitch,int length,int velocity){while(length){int count=qMin(255,length);t.notes<<SoundNote{pitch,count,velocity};length-=count;}};
  append(0,gap,-1);int pitch=n.pitch;if(lane==3){pitch=drumPitch(pitch);if(pitch!=n.pitch)++drums;}else{while(pitch<36)pitch+=12;while(pitch>107)pitch-=12;if(pitch!=n.pitch)++shifted;}
  if(!t.enabled){t.pan=n.pan;if(lane==2){int level=qRound(n.velocity*15.0/127);t.waveLevel=level>=12?1:level>=6?2:3;}}else if(t.pan!=n.pan)++stereo;t.enabled=true;append(pitch,duration,qMax(1,qRound(n.velocity*15.0/127)));cursor[lane]=end;source[lane]=n.channel;++result.importedNotes;
 }
 if(!result.importedNotes)return fail("no notes fit the editor's timing and channel limits.");
 int ending=qMin(MaxTicks,gameTick(seconds));for(int lane=0;lane<4;++lane){auto &t=clip.channels[lane];if(t.enabled){int remaining=qMax(0,ending-cursor[lane]);while(remaining&&t.notes.size()<64){int n=qMin(255,remaining);t.notes<<SoundNote{0,n};remaining-=n;}if(remaining)clipped=true;}if(t.notes.isEmpty())t.notes<<SoundNote{0,1};}
 clip.notes=clip.channels[0].notes;clip.duty=clip.channels[0].duty;clip.volume=clip.channels[0].volume;
 auto warn=[&](int count,const QString &message){if(count)result.warnings<<QString::number(count)+" "+message;};warn(splitNotes,"long notes split at 255 ticks; hardware envelopes may retrigger.");if(clip.channels[2].enabled)result.warnings<<"Wave-channel velocity is approximated by a fixed hardware output level.";warn(overlap,"notes omitted: more than three simultaneous tones or one drum voice.");warn(capacity,"notes omitted: channel patterns are limited to 64 note/rest rows and 272 seconds.");warn(shifted,"pitched notes shifted by octaves into C2–B7.");warn(drums,"drum notes mapped to the six supported percussion sounds.");warn(stereo,"pan changes flattened to the lane's initial stereo setting.");warn(ignored,"program, pitch-bend, controller, pressure or SysEx events ignored.");warn(unterminated,"unreleased notes ended at the file's final event.");warn(unmatched,"unmatched note-off events ignored.");if(clipped)result.warnings<<"The song or trailing rests were shortened to fit the editor's pattern limits.";if(tempoChanges>1)result.warnings<<"Tempo changes were baked into note durations; the editor shows the first tempo.";if(clip.bpm!=qRound(60000000.0/initialTempo))result.warnings<<"Displayed tempo clamped to 30–300 BPM; note timing is preserved.";
 auto errors=validateSound(clip);if(!errors.isEmpty())return fail(errors.join('\n'));out=result;return true;
}
bool exportMidi(const SoundClip &input,QByteArray &out,QStringList &warnings,QString &error){
 error.clear();auto errors=validateSound(input);if(!errors.isEmpty()){error=errors.join('\n');return false;}auto clip=input;bool legacyEffect=clip.effect&&clip.channels.isEmpty();expandSoundChannels(clip);if(legacyEffect){clip.channels[1]=clip.channels[0];clip.channels[0].enabled=false;}
 constexpr int ppqn=960;int tempo=qRound(60000000.0/clip.bpm);auto midiTick=[&](int tick){return quint32(qRound64(double(tick)*1000000*ppqn/(60.0*tempo)));};int length=soundDuration(clip);QByteArray data="MThd";be(data,6,4);be(data,1,2);be(data,5,2);be(data,ppqn,2);
 QByteArray conductor;vlq(conductor,0);meta(conductor,3,clip.name.toUtf8());QByteArray tempoData;be(tempoData,tempo,3);vlq(conductor,0);meta(conductor,0x51,tempoData);vlq(conductor,midiTick(length));meta(conductor,0x2f,{});chunk(data,conductor);
 for(int lane=0;lane<4;++lane){const auto &t=clip.channels[lane];int channel=lane==3?9:lane;QByteArray track;vlq(track,0);meta(track,3,QStringList{"Pulse 1","Pulse 2","Wave","Noise drums"}[lane].toUtf8());vlq(track,0);track.append(char(0xb0|channel));track.append(char(10));track.append(char(t.pan==1?0:t.pan==2?127:64));if(lane<3){vlq(track,0);track.append(char(0xc0|channel));track.append(char(lane==2?81:80));}
  int tick=0;quint32 last=0;for(auto n:t.notes){int level=lane==2?(t.waveLevel==1?15:t.waveLevel==2?8:t.waveLevel==3?4:0):n.volume<0?t.volume:n.volume;bool audible=t.enabled&&t.dac&&t.pan&&(!clip.effect||lane==1)&&n.note&&level;
   if(audible){auto start=midiTick(tick),end=qMax(start+1,midiTick(tick+n.ticks));vlq(track,start-last);track.append(char(0x90|channel));track.append(char(lane==3?drumPitch(n.note):n.note));track.append(char(qBound(1,qRound(level*127.0/15),127)));vlq(track,end-start);track.append(char(0x80|channel));track.append(char(lane==3?drumPitch(n.note):n.note));track.append(char(0));last=end;}
   tick+=n.ticks;
  }vlq(track,midiTick(length)-last);meta(track,0x2f,{});chunk(data,track);
 }
 warnings={"MIDI preserves note timing, pitches, velocity and static stereo. Game Boy waveforms, envelopes, sweep, noise-register overrides and master volume are not preserved; playback uses MIDI instruments."};out=data;return true;
}
}
