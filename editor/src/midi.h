#pragma once
#include "project.h"
namespace cf {
struct MidiImport {SoundClip clip;QStringList warnings;int sourceNotes=0,importedNotes=0;};
// Standard MIDI Files; conversion is transactional and uses no external tools.
bool importMidi(const QByteArray &,MidiImport &,QString &error);
bool exportMidi(const SoundClip &,QByteArray &,QStringList &warnings,QString &error);
}
