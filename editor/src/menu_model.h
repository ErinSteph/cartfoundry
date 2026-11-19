#pragma once
#include "project.h"
#include "cf_game.h"
namespace cf {
CFSaveDef saveDefinition(const Project &);
bool addMenuPreset(Project &,const QString &preset,QString &selectedScene,QString &error);
void generateSaveScripts(Project &);
void fillWorstText(const Project &,CFGameState &);
}
