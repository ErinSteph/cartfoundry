#pragma once
#include "project.h"
#include "cf_sound.h"
namespace cf {
CFSoundNote compileSoundNote(const SoundNote &,const SoundTrack &,int channel);
CFSoundTrack compileSoundTrack(const SoundTrack &);
int soundDuration(const SoundClip &);
}
