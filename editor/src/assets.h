#pragma once
#include <QImage>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include "cf_animation.h"
namespace cf {
struct SpriteAnimation {
    QString name;
    int first=0,last=0,duration=6; // duration of the entire range in ticks
    bool loop=true;bool reverse=false,flipH=false,flipV=false;
};
struct RasterAsset {
    QString id, name, kind; // sprite or tileset
    int width = 8, height = 8;
    QByteArray pixels; // one 0..3 index per pixel; index zero is transparent for sprites
    QVector<quint16> palette; // four RGB555 colours
    int columns=1,rows=1;
    QVector<SpriteAnimation> animations;
    int frameCount() const { return columns*rows; }
    int frameWidth() const { return columns>0?width/columns:0; }
    int frameHeight() const { return rows>0?height/rows:0; }
    int frameTileCount() const { return frameWidth()/8*(frameHeight()/8); }
    QRect frameRect(int index) const;
    QImage frameImage(int frame=0,bool dmg=false) const;
    QVector<CFAnimation> clips() const;
    int animationIndex(const QString &) const;
    QJsonObject toJson() const;
    static bool fromJson(const QJsonObject &, RasterAsset &, QString &error);
    QImage image(bool dmg = false) const;
    QByteArray tileData(bool dmg = false) const;
    QVector<int> colorOrder(bool dmg) const;
    int tileCount() const { return width / 8 * (height / 8); }
};
QJsonObject animationList(const RasterAsset &);
bool applyAnimationList(const QJsonObject &,RasterAsset &,QString &error);
struct ImportOptions {
    int width = 16, height = 16; // output size per frame
    int columns=1,rows=1;
    bool screen=false;
    bool smooth = false;
    enum Fit { Contain, Cover, Stretch, Preserve } fit = Contain;
    bool transparent = true;
    QVector<quint16> palette; // optional user-selected visible colours
    bool removeBackground = false;
    QColor background = Qt::white;
    int tolerance = 24;
};
ImportOptions guessSpriteSheet(const QImage &, const QString &filename = {});
int usedColorCount(const QImage &, bool transparent);
RasterAsset convertImage(const QImage &, const ImportOptions &, int *usedColors = nullptr);
QImage textScreenImage(const QStringList &, const QRect &, bool dmg,const QVector<quint16> &palette={});
QColor rgb555(quint16 value);
quint16 toRgb555(QColor value);
}
