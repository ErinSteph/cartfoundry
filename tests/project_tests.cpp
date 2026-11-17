#include "builder.h"
#include <QtTest>
#include <QTemporaryDir>
#include <QJsonArray>
#include <QFile>
#include <QCryptographicHash>
class ProjectTests : public QObject {
    Q_OBJECT
private slots:
    void roundTrip() {
        auto p = cf::Project::example(); QVERIFY2(p.validate().isEmpty(), qPrintable(p.validate().join('\n')));
        QTemporaryDir temp; QString error; const QString file = temp.filePath("with spaces.cartfoundry");
        QVERIFY(p.save(file, error)); cf::Project loaded; QVERIFY(cf::Project::load(file, loaded, error));
        QCOMPARE(loaded.toJson(), p.toJson());
        loaded.prompts[0].choices[0].target = "quest"; loaded.prompts[0].position = {420, -40};
        QVERIFY(loaded.save(file, error)); cf::Project reloaded; QVERIFY(cf::Project::load(file, reloaded, error));
        QCOMPARE(reloaded.prompts[0].choices[0].target, QString("quest")); QCOMPARE(reloaded.prompts[0].position, QPointF(420, -40));
    }
    void malformedAndLimits() {
        auto p = cf::Project::example(); auto json = p.toJson(); QString error; cf::Project loaded;
        json["version"] = 99; QVERIFY(!cf::Project::fromJson(json, loaded, error));
        json = p.toJson(); auto nodes = json["prompts"].toArray(); nodes.append(nodes[0]); json["prompts"] = nodes;
        QVERIFY(!cf::Project::fromJson(json, loaded, error));
        p.prompts[0].choices[0].target.clear(); QVERIFY(!p.validate().isEmpty());
        p = cf::Project::example(); p.prompts[0].text = QString(19, 'x'); QVERIFY(!p.validate().isEmpty());
        p.prompts[0].text = "hello\n\nworld"; QCOMPARE(cf::wrapText(p.prompts[0].text), QStringList({"hello", "", "world"}));
        p.prompts[0].text = QString("line\n").repeated(12); QVERIFY(!p.validate().isEmpty());
        p = cf::Project::example(); p.prompts[0].title = QString::fromUtf8("café"); QVERIFY(!p.validate().isEmpty());
        p = cf::Project::example(); p.prompts[0].choices.clear(); QVERIFY(p.validate().isEmpty());
    }
    void screenAndEscaping() {
        auto p = cf::Project::example(); auto lines = cf::screenLines(p, 0, 1);
        QCOMPARE(lines.size(), 18); for (auto &line : lines) QCOMPARE(line.size(), 20);
        QVERIFY(lines[cf::textLayout(p.prompts[0]).choiceRow + 1].startsWith("|>Follow"));
        p.prompts[0].text = "\"quoted\" \\ path\n100% ready?" "?!";
        QVERIFY(p.validate().isEmpty()); auto h = cf::generateHeader(p);
        QVERIFY(h.contains("\\\"quoted\\\"")); QVERIFY(h.contains("\\\\ path")); QVERIFY(h.contains("\\?\\?!"));
    }
    void scriptsAndControls() {
        for(auto kind:{"top_down","patrol","chase","idle"}){cf::BehaviorSpec b;QString error;QVERIFY(cf::parseBehavior(cf::behaviorTemplate(kind),b,error));}
        cf::BehaviorSpec b;QString error;QVERIFY(!cf::parseBehavior("behavior chase\nspeed 99",b,error));QVERIFY(!cf::parseBehavior("behavior chase\nunknown 1",b,error));QVERIFY(!cf::parseBehavior("behavior idle\nspeed 1\nspeed 2",b,error));
        auto p=cf::Project::overworldExample();p.controls.interact=32;QVERIFY(!p.validate().isEmpty());p.controls.close=16;QVERIFY(p.validate().isEmpty());
        p.world.actors[0].script+="on_touch missing\n";QVERIFY(!p.validate().isEmpty());
    }
    void spriteSheets() {
        cf::RasterAsset a;a.id="sheet";a.name="Sheet";a.kind="sprite";a.width=32;a.height=16;a.columns=2;a.rows=2;a.palette={0,1,2,3};a.pixels=QByteArray(512,0);
        for(int y=0;y<16;++y)for(int x=0;x<32;++x)a.pixels[y*32+x]=char((y/8*2+x/16+1)%4);
        a.animations={{"walk",0,3,24,true},{"fire",1,2,12,false}};
        QCOMPARE(a.frameCount(),4);QCOMPARE(a.frameWidth(),16);QCOMPARE(a.frameHeight(),8);QCOMPARE(a.frameTileCount(),2);auto data=a.tileData();QCOMPARE(data.size(),128);
        QCOMPARE(quint8(data[0]),quint8(255));QCOMPARE(quint8(data[1]),quint8(0));QCOMPARE(quint8(data[32]),quint8(0));QCOMPARE(quint8(data[33]),quint8(255));QCOMPARE(quint8(data[64]),quint8(255));QCOMPARE(quint8(data[65]),quint8(255));
        cf::RasterAsset loaded;QString error;QVERIFY(cf::RasterAsset::fromJson(a.toJson(),loaded,error));QCOMPARE(a.toJson(),loaded.toJson());QCOMPARE(a.frameImage(3).size(),QSize(16,8));
        a.animations[0].last=4;QVERIFY(!cf::RasterAsset::fromJson(a.toJson(),loaded,error));a.animations[0].last=3;a.animations[0].duration=3;QVERIFY(!cf::RasterAsset::fromJson(a.toJson(),loaded,error));a.animations[0].duration=24;a.columns=3;QVERIFY(!cf::RasterAsset::fromJson(a.toJson(),loaded,error));
        QImage source(32,32,QImage::Format_ARGB32);source.fill(Qt::transparent);for(int y=0;y<32;++y)for(int x=0;x<32;++x)if(x<16||y<16)source.setPixelColor(x,y,x<16?(y<16?Qt::red:Qt::blue):Qt::green);
        cf::ImportOptions options;options.width=8;options.height=8;options.columns=2;options.rows=2;auto imported=cf::convertImage(source,options);QCOMPARE(imported.frameCount(),4);QCOMPARE(imported.frameWidth(),8);QCOMPARE(imported.width,16);QCOMPARE(imported.animations[0].last,3);QCOMPARE(imported.frameImage(3).pixelColor(0,0).alpha(),0);
        auto p=cf::Project::overworldExample();p.world.playerScript+="on_move missing_clip\n";QVERIFY(!p.validate().isEmpty());
    }
    void rasterConversion() {
        QImage source(20,10,QImage::Format_ARGB32); source.fill(Qt::transparent);
        for(int y=1;y<9;++y)for(int x=2;x<18;++x)source.setPixelColor(x,y,QColor(x*12,y*22,140));
        cf::ImportOptions options;options.width=16;options.height=16;
        auto a=cf::convertImage(source,options);a.id="test";a.name="Test";
        QCOMPARE(a.pixels.size(),256);QCOMPARE(a.palette.size(),4);QCOMPARE(a.tileData().size(),64);QVERIFY(a.image().pixelColor(0,0).alpha()==0);
        for(auto index:a.pixels)QVERIFY(quint8(index)<=3);
        auto original=a.toJson();auto dmg=a.tileData(true);QCOMPARE(a.toJson(),original);QCOMPARE(dmg.size(),64);
        QString error;cf::RasterAsset decoded;QVERIFY(cf::RasterAsset::fromJson(original,decoded,error));QCOMPARE(decoded.toJson(),original);
        auto corrupt=original;corrupt["pixels"]="!not-base64!";QVERIFY(!cf::RasterAsset::fromJson(corrupt,decoded,error));
        source.fill(Qt::white);options.removeBackground=true;options.background=Qt::white;a=cf::convertImage(source,options);
        for(auto index:a.pixels)QCOMPARE(index,char(0));
        cf::RasterAsset tile;tile.kind="tileset";tile.width=8;tile.height=8;tile.palette={0,1,2,3};tile.pixels=QByteArray(64,0);tile.pixels[0]=3;tile.pixels[1]=1;auto bits=tile.tileData();
        QCOMPARE(quint8(bits[0]),quint8(0xc0));QCOMPARE(quint8(bits[1]),quint8(0x80));
    }
    void worldAndModes() {
        auto p=cf::Project::overworldExample();QString error;cf::Project loaded;
        QVERIFY2(p.validate().isEmpty(),qPrintable(p.validate().join('\n')));
        QVERIFY(cf::Project::fromJson(p.toJson(),loaded,error));QCOMPARE(p.toJson(),loaded.toJson());
        auto old=p.world;p.world.resize(48,40);QCOMPARE(p.world.tiles[7*48+6],old.tiles[7*40+6]);QCOMPARE(p.world.interactions.value(7*48+6),QString("welcome"));
        p.world.resize(40,32);QCOMPARE(p.world.toJson(),old.toJson());
        QTemporaryDir temp;auto color=cf::buildRom(p,temp.filePath("world.gbc"));QVERIFY2(color.success,qPrintable(color.log));
        QFile f(color.romPath);QVERIFY(f.open(QIODevice::ReadOnly));auto c=f.readAll();QCOMPARE(quint8(c[0x143]),quint8(0xc0));
        p.target="gb";auto mono=cf::buildRom(p,temp.filePath("world.gb"));QVERIFY2(mono.success,qPrintable(mono.log));
        QFile g(mono.romPath);QVERIFY(g.open(QIODevice::ReadOnly));auto m=g.readAll();QCOMPARE(quint8(m[0x143]),quint8(0));QVERIFY(c!=m);
        QCOMPARE(p.assets[0].toJson(),loaded.assets[0].toJson()); // mode changes retain source colour assets
        p.world.collision[p.world.spawnY*p.world.width+p.world.spawnX]=1;QVERIFY(!p.validate().isEmpty());
        p=cf::Project::overworldExample();p.world.tiles[0]=999;QVERIFY(!p.validate().isEmpty());
    }
    void textboxLayout() {
        auto p=cf::Project::example();auto &n=p.prompts[0];n.title="SIGN";n.text="Hello there.";n.choices={{"OK","quest"}};n.textbox={2,8,16,10};
        QVERIFY2(p.validate().isEmpty(),qPrintable(p.validate().join('\n')));auto lines=cf::screenLines(p,0,0);QCOMPARE(lines[8][2],QChar('+'));QVERIFY(lines[0].trimmed().isEmpty());
        n.text=QString("A ").repeated(60);QVERIFY(!p.validate().isEmpty());
        auto json=p.toJson();auto nodes=json["prompts"].toArray();auto node=nodes[0].toObject();node["textbox"]=QJsonArray{19,0,20,18};nodes[0]=node;json["prompts"]=nodes;QString error;cf::Project out;QVERIFY(!cf::Project::fromJson(json,out,error));
    }
    void buildAndRewire() {
        QTemporaryDir temp; QVERIFY(temp.isValid()); auto p = cf::Project::example();
        auto first = cf::buildRom(p, temp.filePath("original with spaces.gbc"));
        QVERIFY2(first.success, qPrintable(first.log));
        QFile f(first.romPath); QVERIFY(f.open(QIODevice::ReadOnly)); auto a = f.readAll();
        QCOMPARE(a.size(), 32768); QCOMPARE(quint8(a[0x143]), quint8(0xc0)); QCOMPARE(quint8(a[0x147]), quint8(0));
        quint8 checksum = 0; for (int i = 0x134; i <= 0x14c; ++i) checksum = checksum - quint8(a[i]) - 1;
        QCOMPARE(checksum, quint8(a[0x14d]));
        quint16 global = 0; for (int i = 0; i < a.size(); ++i) if (i != 0x14e && i != 0x14f) global += quint8(a[i]);
        QCOMPARE(global, quint16((quint8(a[0x14e]) << 8) | quint8(a[0x14f])));
        QVERIFY(a.contains("THE WORKSHOP"));
        p.prompts[0].choices[0].target = "quest";
        auto second = cf::buildRom(p, temp.filePath("rewired.gbc")); QVERIFY2(second.success, qPrintable(second.log));
        QFile g(second.romPath); QVERIFY(g.open(QIODevice::ReadOnly)); QVERIFY(a != g.readAll());
        // Failed validation leaves an existing ROM untouched.
        p.prompts[0].choices[0].target = "missing";
        auto rejected = cf::buildRom(p, first.romPath); QVERIFY(!rejected.success); f.seek(0); QCOMPARE(f.readAll(), a);
    }
};
QTEST_GUILESS_MAIN(ProjectTests)
#include "project_tests.moc"
