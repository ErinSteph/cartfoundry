#pragma once
#include "project.h"
namespace cf {
struct BuildResult { bool success = false; QString log, romPath; };
QString resourceRoot();
QString generateHeader(const Project &);
BuildResult buildRom(const Project &, const QString &output, const QString &resources = {});
}
