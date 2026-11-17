#pragma once
#include "project.h"
#include "cf_game.h"
namespace cf {
struct ScreenBitmap {QByteArray tiles,map;QVector<quint16> palette;};
ScreenBitmap screenBitmap(const QImage &,bool dmg,int base=0);
QImage sceneBackdrop(const Project &,const Scene &);
QImage sceneImage(const Project &,const Scene &,const CFGameState *state=nullptr,int selected=-1);
}
