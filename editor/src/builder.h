#pragma once
#include "project.h"
namespace cf {
struct EncodedResource {QByteArray data;int codec=0;};
EncodedResource encodeResource(const QByteArray &);
struct BuildResult { bool success = false; QString log, romPath; };
QString resourceRoot();
QString generateHeader(const Project &);
BuildResult buildRom(const Project &, const QString &output, const QString &resources = {});
}
