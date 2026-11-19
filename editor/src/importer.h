#pragma once
#include "assets.h"
class QWidget;
namespace cf {
struct Project;
bool removeAsset(QWidget *parent, const Project &, const QString &id, Project &result);
bool importAsset(QWidget *parent, const QString &kind, RasterAsset &result, bool placeAfterImport=false); }
