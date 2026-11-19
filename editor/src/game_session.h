#pragma once
#include "project.h"
#include "game_script.h"
#include "audio_output.h"
namespace cf {
class GameSession {
public:
    CFGameState state{};CFGameDef definition{};QVector<CFPrompt> prompts;QVector<CFChoiceRule> rules;QVector<CFProgram> enter;
    void configure(const Project &,bool reset=true);
    void enterPrompt(CFState &,int index);
    bool step(CFState &,int key);
    bool enabled(int prompt,int choice)const;
    bool run(const QString &);
    const Project &data()const{return project;}
    bool processSave();
    QString previewSavePath()const;
    QString saveError;
    QStringList keyboardLines()const;
    void reset();void tick(bool inDialogue);QByteArray renderAudio(int frames=800);
    void reseed(uint16_t seed){state.random=seed?seed:0xace1;}
private:
    QVector<QByteArray> textInitial,keyboardTitles,keyboardChars;
    QVector<CFTextDef> textDefs;QVector<CFKeyboardDef> keyboardDefs;
    QVector<QVector<CFInstruction>> runPrograms;
    AudioOutput audio;double phase[4]={0,0,0,0};uint16_t noiseLfsr=0x7fff,lastNoiseTrigger=0;
    Project project;QVector<int16_t> initial;QVector<uint8_t> limits;QVector<CFStorageDef> stores;QVector<QVector<uint16_t>> counts;
    QVector<QVector<CFSoundNote>> soundNotes;QVector<QVector<CFSoundTrack>> soundTracks;QVector<CFSoundClip> soundClips;
    QVector<QVector<CFInstruction>> enterCode,choiceCode,timerCode;QVector<CFTimerDef> timerDefs;
};
}
