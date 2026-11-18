#pragma once
#include "project.h"
namespace cf {
struct MapBrush {QString kind,area,id;};
struct MapSelection {QString kind,id;bool operator==(const MapSelection&o)const{return kind==o.kind&&id==o.id;}};
World &areaAt(Project &,int);
QString uniqueMapId(QString base,const QStringList &used);
bool placeMapInstance(Project &,int area,const MapBrush &,QPoint tile,MapSelection &,QString &error);
bool moveMapInstance(Project &,int area,const MapSelection &,QPoint tile,QString &error);
bool renameMapInstance(Project &,int area,const MapSelection &,QString id,QString &error);
bool removeMapInstance(Project &,int area,const MapSelection &);
int createMap(Project &,QString name,QString scene,QString parentMap,int sourceArea,int width,int height,QString tileset,QString player,bool contents,QString &error);
}
