#include "sound_compile.h"
#include <cmath>
namespace cf {
CFSoundNote compileSoundNote(const SoundNote &n,const SoundTrack &t,int channel){int value=0;if(n.note){if(channel==3){int noise=(t.noiseShift<<4)|(t.noiseNarrow?8:0)|t.noiseDivider;if(t.drumKit){switch(n.note){case 36:noise=0x63;break;case 38:noise=0x35;break;case 42:noise=0x19;break;case 46:noise=0x29;break;case 45:noise=0x54;break;case 49:noise=0x45;break;}}if(n.noise>=0)noise=n.noise;value=noise+1;}else value=qRound(440*std::pow(2.0,(n.note-69)/12.0));}return {uint16_t(value),uint8_t(n.ticks),uint8_t(n.volume+1)};}
CFSoundTrack compileSoundTrack(const SoundTrack &t){CFSoundTrack out{};out.count=t.notes.size();out.duty=t.duty;out.volume=t.volume;out.pan=t.pan;out.envelope=t.envelopePace|(t.envelopeUp?8:0);out.sweep=(t.sweepPace<<4)|(t.sweepDown?8:0)|t.sweepShift;out.length=t.length;out.dac=t.dac;out.wave_level=t.waveLevel;for(int i=0;i<16;++i)out.wave[i]=(t.wave.value(i*2)<<4)|t.wave.value(i*2+1);return out;}
int soundDuration(const SoundClip &s){int total=0;auto count=[&](const QVector<SoundNote> &notes){int t=0;for(auto n:notes)t+=n.ticks;total=qMax(total,t);};if(s.channels.isEmpty())count(s.notes);else for(auto &t:s.channels)count(t.notes);return total;}
}
