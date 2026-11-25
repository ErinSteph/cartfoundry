#pragma once
#include <QString>
#include <QStringList>
#include "cf_behavior.h"
#include "cf_animation.h"
namespace cf {
struct BehaviorSpec {CFBehavior code{CF_BEHAVIOR_IDLE,1,1,0,32,80,255};QString prompt,animation,moveAnimation,idleAnimation,actionAnimation;bool directional=false;QString moveDir[4],idleDir[4];};
/* Direction order matches CF_DIR_*: up, down, left, right. */
bool parseBehavior(const QString &,BehaviorSpec &,QString &error);
struct RasterAsset;
CFAnimationBindings animationBindings(const BehaviorSpec &,const RasterAsset &);
QString behaviorTemplate(const QString &kind);
}
