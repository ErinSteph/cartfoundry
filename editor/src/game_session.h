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
    void reset();void tick(bool inDialogue);
    void reseed(uint16_t seed){state.random=seed?seed:0xace1;}
private:
    AudioOutput audio;double phase[2]={0,0};
    Project project;QVector<int16_t> initial;QVector<uint8_t> limits;QVector<CFStorageDef> stores;QVector<QVector<uint16_t>> counts;
    QVector<QVector<CFInstruction>> enterCode,choiceCode,timerCode;QVector<CFTimerDef> timerDefs;
};
}
