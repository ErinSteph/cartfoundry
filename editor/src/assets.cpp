#include "assets.h"
#include "cf_font.h"
#include <QJsonArray>
#include <QPainter>
#include <QMap>
#include <QSet>
#include <QRegularExpression>
#include <algorithm>
#include <limits>
namespace cf {
QImage textScreenImage(const QStringList &lines, const QRect &box, bool dmg,const QVector<quint16> &palette) {
    QImage image(160,144,QImage::Format_ARGB32); image.fill(Qt::transparent);
    QColor paper = dmg ? QColor("#e0edc4") : rgb555(29 | (29 << 5) | (25 << 10));
    QColor ink = dmg ? QColor("#223c39") : rgb555(4 | (7 << 5) | (11 << 10));
    if(!dmg&&palette.size()==4){paper=rgb555(palette[0]);ink=rgb555(palette[3]);}
    for (int y=qMax(0,box.y()); y<=qMin(17,box.bottom()); ++y) for (int x=qMax(0,box.x()); x<=qMin(19,box.right()); ++x) {
        int c = y<lines.size() && x<lines[y].size() ? lines[y][x].unicode() : 32;
        if(c<32||c>127)c='?';
        for(int py=0;py<8;++py)for(int px=0;px<8;++px)image.setPixelColor(x*8+px,y*8+py,cf_font[(c-32)*8+py] & (128>>px) ? ink : paper);
    }
    return image;
}
QColor rgb555(quint16 v) { return QColor((v & 31) * 255 / 31, ((v >> 5) & 31) * 255 / 31, ((v >> 10) & 31) * 255 / 31); }
quint16 toRgb555(QColor c) { return ((c.red()*31+127)/255) | (((c.green()*31+127)/255) << 5) | (((c.blue()*31+127)/255) << 10); }
QJsonObject RasterAsset::toJson() const {
    QJsonArray colors; for (auto c : palette) colors.append(int(c));
    QJsonArray clips;for(const auto &c:animations)clips.append(QJsonObject{{"name",c.name},{"first",c.first},{"last",c.last},{"duration",c.duration},{"loop",c.loop},{"reverse",c.reverse},{"flipH",c.flipH},{"flipV",c.flipV}});
    return {{"columns",columns},{"rows",rows},{"animations",clips},{"id", id}, {"name", name}, {"kind", kind}, {"width", width}, {"height", height}, {"palette", colors}, {"pixels", QString::fromLatin1(pixels.toBase64())}};
}
bool RasterAsset::fromJson(const QJsonObject &o, RasterAsset &a, QString &error) {
    auto fail = [&] { error = "Invalid embedded raster asset (8-pixel dimensions, four RGB555 colours and indexed pixels required)."; return false; };
    if (!o["id"].isString() || o["id"].toString().isEmpty() || !o["name"].isString() || !o["kind"].isString()) return fail();
    const auto kind = o["kind"].toString(); if (kind != "sprite" && kind != "tileset" && kind != "screen") return fail();
    int w = o["width"].toInt(-1), h = o["height"].toInt(-1), limit = kind == "screen"?160:kind == "sprite" ? 256 : 128;
    if (o["width"].toDouble() != w || o["height"].toDouble() != h || w < 8 || h < 8 || w > limit || h > (kind=="screen"?144:limit) || w % 8 || h % 8) return fail();
    int columns=o.contains("columns")?o["columns"].toInt(-1):1,rows=o.contains("rows")?o["rows"].toInt(-1):1;
    if(columns<1||rows<1||columns>8||rows>8||columns*rows>64||w%columns||h%rows||w/columns%8||h/rows%8)return fail();
    if(kind=="sprite" && (w/columns>32||h/rows>32))return fail();
    if(kind!="sprite" && (columns!=1||rows!=1))return fail();
    if((o.contains("columns") && o["columns"].toDouble()!=columns)||(o.contains("rows") && o["rows"].toDouble()!=rows))return fail();
    QVector<SpriteAnimation> animations;QSet<QString> names;
    if(o.contains("animations")){
        if(!o["animations"].isArray()||o["animations"].toArray().size()>32)return fail();
        for(auto value:o["animations"].toArray()){
            if(!value.isObject())return fail();auto c=value.toObject();SpriteAnimation a;a.name=c["name"].toString();a.first=c["first"].toInt(-1);a.last=c["last"].toInt(-1);a.duration=c["duration"].toInt(-1);a.loop=c["loop"].toBool();for(auto key:{"reverse","flipH","flipV"})if(c.contains(key)&&!c[key].isBool())return fail();a.reverse=c["reverse"].toBool();a.flipH=c["flipH"].toBool();a.flipV=c["flipV"].toBool();
            if(kind!="sprite"||!QRegularExpression("^[A-Za-z_][A-Za-z0-9_]{0,31}$").match(a.name).hasMatch()||names.contains(a.name)||a.first<0||a.last<a.first||a.last>=columns*rows||a.duration<a.last-a.first+1||a.duration>3600||!c["loop"].isBool()||c["first"].toDouble()!=a.first||c["last"].toDouble()!=a.last||c["duration"].toDouble()!=a.duration)return fail();
            names.insert(a.name);animations<<a;
        }
    }
    auto colors = o["palette"].toArray(); if (colors.size() != 4 || !o["pixels"].isString()) return fail();
    QVector<quint16> pal;
    for (const auto c : colors) { int v = c.toInt(-1); if (v < 0 || v > 32767 || c.toDouble() != v) return fail(); pal << quint16(v); }
    auto decoded = QByteArray::fromBase64Encoding(o["pixels"].toString().toLatin1(), QByteArray::AbortOnBase64DecodingErrors);
    if (!decoded || decoded.decoded.size() != w * h) return fail();
    for (auto v : decoded.decoded) if (quint8(v) > 3) return fail();
    a = {o["id"].toString(), o["name"].toString(), kind, w, h, decoded.decoded, pal};a.columns=columns;a.rows=rows;a.animations=animations;return true;
}
QRect RasterAsset::frameRect(int index) const {if(columns<1||rows<1||index<0||index>=frameCount())return {};return QRect(index%columns*frameWidth(),index/columns*frameHeight(),frameWidth(),frameHeight());}
QImage RasterAsset::frameImage(int frame,bool dmg) const {auto rect=frameRect(frame);return rect.isEmpty()?QImage():image(dmg).copy(rect);}
int RasterAsset::animationIndex(const QString &name) const {for(int i=0;i<animations.size();++i)if(animations[i].name==name)return i;return -1;}
QVector<CFAnimation> RasterAsset::clips() const {QVector<CFAnimation> result;for(const auto &a:animations)result<<CFAnimation{uint8_t(a.first),uint8_t(a.last),uint16_t(a.duration),uint8_t(a.loop),uint8_t((a.reverse?1:0)|(a.flipH?2:0)|(a.flipV?4:0))};if(result.isEmpty())result<<CFAnimation{0,0,1,1};return result;}
QVector<int> RasterAsset::colorOrder(bool dmg) const {
    QVector<int> order{0,1,2,3}, remap{0,1,2,3}; if (!dmg || palette.size() != 4) return remap;
    std::stable_sort(order.begin() + (kind == "sprite" ? 1 : 0), order.end(), [&](int a, int b) { return qGray(rgb555(palette[a]).rgb()) > qGray(rgb555(palette[b]).rgb()); });
    for (int i = 0; i < 4; ++i) remap[order[i]] = i; return remap;
}
QImage RasterAsset::image(bool dmg) const {
    if (width < 1 || height < 1 || pixels.size() != width * height || palette.size() != 4) return {};
    QImage out(width, height, QImage::Format_ARGB32); auto order = colorOrder(dmg);
    const QColor shades[] = {QColor("#e0edc4"),QColor("#a1b875"),QColor("#527451"),QColor("#223c39")};
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
        int index = quint8(pixels[y * width + x]); if (index > 3) return {};
        auto c = dmg ? shades[order[index]] : rgb555(palette[index]); if (!index && kind == "sprite") c.setAlpha(0);
        out.setPixelColor(x, y, c);
    }
    return out;
}
QByteArray RasterAsset::tileData(bool dmg) const {
    QByteArray bytes; auto order = colorOrder(dmg);
    const int frames=kind=="sprite"?frameCount():1;
    for(int frame=0;frame<frames;++frame){QRect rect=kind=="sprite"?frameRect(frame):QRect(0,0,width,height);
    for (int ty = rect.y(); ty < rect.bottom()+1; ty += 8) for (int tx = rect.x(); tx < rect.right()+1; tx += 8)
        for (int y = 0; y < 8; ++y) {
            quint8 lo = 0, hi = 0;
            for (int x = 0; x < 8; ++x) { int v = order[quint8(pixels[(ty + y) * width + tx + x])]; lo |= (v & 1) << (7 - x); hi |= ((v >> 1) & 1) << (7 - x); }
            bytes.append(char(lo)); bytes.append(char(hi));
        }
    }
    return bytes;
}
ImportOptions guessSpriteSheet(const QImage &source,const QString &filename) {
    ImportOptions best;best.fit=ImportOptions::Preserve;
    best.width=qBound(8,((source.width()+7)/8)*8,32);best.height=qBound(8,((source.height()+7)/8)*8,32);
    if(source.isNull())return best;
    auto fits=[&](int w,int h){return w>=8&&w<=32&&h>=8&&h<=32&&w%8==0&&h%8==0&&source.width()%w==0&&source.height()%h==0&&source.width()/w<=8&&source.height()/h<=8;};
    auto use=[&](int w,int h){best.width=w;best.height=h;best.columns=source.width()/w;best.rows=source.height()/h;};
    QRegularExpression hint("(?:^|[^0-9])([0-9]{1,3})[xX]([0-9]{1,3})(?:[^0-9]|$)");auto match=hint.match(filename);
    if(match.hasMatch()){int w=match.captured(1).toInt(),h=match.captured(2).toInt();if(fits(w,h)){use(w,h);return best;}if(w>=1&&w<=8&&h>=1&&h<=8&&source.width()%w==0&&source.height()%h==0&&fits(source.width()/w,source.height()/h)){use(source.width()/w,source.height()/h);return best;}}
    if(source.width()<=32&&source.height()<=32)return best; // Preserve static assets as one frame.
    int score=std::numeric_limits<int>::max();
    for(int w=8;w<=32;w+=8)for(int h=8;h<=32;h+=8)if(fits(w,h)){
        int candidate=qAbs(w-16)+qAbs(h-16)+qAbs(w-h);
        if(candidate<score){score=candidate;use(w,h);}
    }
    return best;
}
int usedColorCount(const QImage &image, bool transparent) {
    QSet<quint16> colors;
    for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x){
        auto c=image.pixelColor(x,y);
        if(!transparent || c.alpha()>=128)colors.insert(toRgb555(c));
    }
    return colors.size();
}
struct Sample { QColor color; int count; };
RasterAsset convertImage(const QImage &source, const ImportOptions &requested, int *usedColors) {
    if(usedColors)*usedColors=0;
    ImportOptions opts=requested;if(opts.screen){opts.transparent=false;opts.columns=opts.rows=1;}
    RasterAsset a; a.columns=opts.columns;a.rows=opts.rows;a.width = opts.width*opts.columns; a.height = opts.height*opts.rows; a.kind = opts.screen?"screen":opts.transparent ? "sprite" : "tileset";
    if (source.isNull() || a.width < 8 || a.height < 8 || a.width > (opts.screen?160:opts.transparent?256:128) || a.height > (opts.screen?144:opts.transparent?256:128) || opts.columns<1 || opts.rows<1 || opts.columns>8 || opts.rows>8 || opts.columns*opts.rows>64 || source.width()%opts.columns || source.height()%opts.rows || (opts.transparent&&(opts.width>32||opts.height>32)) || a.width % 8 || a.height % 8) return a;
    if(opts.fit==ImportOptions::Preserve && (source.width()/opts.columns>opts.width || source.height()/opts.rows>opts.height))return a;
    QImage prepared = source.convertToFormat(QImage::Format_ARGB32);
    if (opts.removeBackground) for (int y = 0; y < prepared.height(); ++y) for (int x = 0; x < prepared.width(); ++x) {
        auto c = prepared.pixelColor(x, y); auto bg = opts.background;
        int d = qMax(qAbs(c.red() - bg.red()), qMax(qAbs(c.green() - bg.green()), qAbs(c.blue() - bg.blue())));
        if (d <= opts.tolerance) prepared.setPixelColor(x, y, Qt::transparent);
    }
    Qt::AspectRatioMode fit = opts.fit == ImportOptions::Stretch ? Qt::IgnoreAspectRatio : opts.fit == ImportOptions::Cover ? Qt::KeepAspectRatioByExpanding : Qt::KeepAspectRatio;
    QImage canvas(a.width,a.height,QImage::Format_ARGB32);canvas.fill(opts.transparent?Qt::transparent:opts.background);
    {QPainter p(&canvas);for(int row=0;row<opts.rows;++row)for(int col=0;col<opts.columns;++col){
        auto cell=prepared.copy(col*source.width()/opts.columns,row*source.height()/opts.rows,source.width()/opts.columns,source.height()/opts.rows);
        auto scaled=opts.fit==ImportOptions::Preserve?cell:cell.scaled(opts.width,opts.height,fit,opts.smooth?Qt::SmoothTransformation:Qt::FastTransformation);
        p.save();p.setClipRect(col*opts.width,row*opts.height,opts.width,opts.height);p.drawImage(col*opts.width+(opts.fit==ImportOptions::Preserve?0:(opts.width-scaled.width())/2),row*opts.height+(opts.fit==ImportOptions::Preserve?0:(opts.height-scaled.height())/2),scaled);p.restore();
    }}
    QMap<quint16, int> histogram;
    for (int y = 0; y < a.height; ++y) for (int x = 0; x < a.width; ++x) {
        auto c = canvas.pixelColor(x, y); if (opts.transparent && c.alpha() < 128) continue;
        histogram[toRgb555(c)]++;
    }
    if(usedColors)*usedColors=histogram.size();
    // Weighted median-cut quantization, in the console's actual RGB555 colour space.
    QVector<QVector<Sample>> boxes(1);
    for (auto it = histogram.begin(); it != histogram.end(); ++it) boxes[0].append({rgb555(it.key()), it.value()});
    const int available = opts.transparent ? 3 : 4;
    while (boxes.size() < available) {
        int chosen = -1, axis = 0, score = -1;
        for (int i = 0; i < boxes.size(); ++i) if (boxes[i].size() > 1) {
            int low[3] = {255,255,255}, high[3] = {0,0,0};
            for (const auto &s : boxes[i]) { int c[] = {s.color.red(),s.color.green(),s.color.blue()}; for (int j = 0; j < 3; ++j) { low[j] = qMin(low[j], c[j]); high[j] = qMax(high[j], c[j]); } }
            for (int j = 0; j < 3; ++j) if (high[j] - low[j] > score) { score = high[j] - low[j]; chosen = i; axis = j; }
        }
        if (chosen < 0) break;
        auto values = boxes.takeAt(chosen);
        auto component = [axis](const Sample &s) { return axis == 0 ? s.color.red() : axis == 1 ? s.color.green() : s.color.blue(); };
        std::stable_sort(values.begin(), values.end(), [&](const Sample &a, const Sample &b) { return component(a) < component(b); });
        int total = 0, weight = 0, split = 1; for (auto &s : values) total += s.count;
        for (int i = 0; i < values.size() - 1; ++i) { weight += values[i].count; split = i + 1; if (weight * 2 >= total) break; }
        boxes << values.mid(0, split) << values.mid(split);
    }
    QVector<quint16> palette;
    for (auto &box : boxes) {
        qint64 r = 0, g = 0, b = 0, count = 0;
        for (auto &s : box) { r += s.color.red() * s.count; g += s.color.green() * s.count; b += s.color.blue() * s.count; count += s.count; }
        palette << toRgb555(count ? QColor(r / count, g / count, b / count) : QColor(Qt::black));
    }
    // Do not average or re-encode an already usable palette.
    if(histogram.size()<=available){palette.clear();for(auto it=histogram.begin();it!=histogram.end();++it)palette<<it.key();}
    if(histogram.size()>available && !opts.palette.isEmpty() && opts.palette.size()<=available)palette=opts.palette;
    if(palette.isEmpty())palette<<0;
    if(opts.palette.isEmpty())std::stable_sort(palette.begin(), palette.end(), [](auto a, auto b) { return qGray(rgb555(a).rgb()) > qGray(rgb555(b).rgb()); });
    if (opts.transparent) palette.prepend(0);
    while (palette.size() < 4) palette << palette.last(); a.palette = palette;
    for (int y = 0; y < a.height; ++y) for (int x = 0; x < a.width; ++x) {
        auto c = rgb555(toRgb555(canvas.pixelColor(x,y))); c.setAlpha(canvas.pixelColor(x,y).alpha()); int best = opts.transparent ? 1 : 0, distance = std::numeric_limits<int>::max();
        if (opts.transparent && c.alpha() < 128) best = 0;
        else for (int i = opts.transparent ? 1 : 0; i < 4; ++i) {
            auto p = rgb555(palette[i]); int r = c.red() - p.red(), g = c.green() - p.green(), b = c.blue() - p.blue(), d = r*r*2 + g*g*4 + b*b;
            if (d < distance) { distance = d; best = i; }
        }
        a.pixels.append(char(best));
    }
    if(opts.transparent&&a.frameCount()>1)a.animations.append({"default",0,a.frameCount()-1,a.frameCount()*6,true});
    return a;
}
}
