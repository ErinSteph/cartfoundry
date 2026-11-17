#pragma once
#include "builder.h"
inline cf::BuildResult fastTestRom(cf::Project p,const QString &output,const QString &resources={}){p.splashTicks=1;return cf::buildRom(p,output,resources);}
