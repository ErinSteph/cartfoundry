#include "rom_test_helpers.h"
#include "emulator_core.h"
#include <QtTest>
#include <QTemporaryDir>
#include <QDataStream>
#include <QFile>
class EmulatorTests:public QObject{
    Q_OBJECT
private slots:
    void fourChannelSong(){auto p=cf::Project::example();p.sounds<<cf::foundryDemoSong();for(auto &scene:p.scenes)scene.onSceneEnter="sound loop foundry_four";p.timers={{"silence","Silence","sound stop",800,false,true,false}};QTemporaryDir dir;
      for(auto target:{QString("gb"),QString("gbc")}){p.target=target;auto built=fastTestRom(p,dir.filePath("song."+target));QVERIFY2(built.success,qPrintable(built.log));cf::EmulatorCore e;QString error;QVERIFY(e.load(built.romPath,error));bool together=false;QSet<int> noise;QByteArray pcm;
       for(int f=0;f<900;++f){e.frame();auto audio=e.takeAudio();if(target=="gbc")pcm+=audio;if((e.readByte(0xff26)&15)==15){together=true;QCOMPARE(e.readByte(0xff25),quint8(0xde));QCOMPARE(e.readByte(0xff1c)&0x60,0x20);QCOMPARE(e.readByte(0xff11)&0xc0,0x80);QCOMPARE(e.readByte(0xff16)&0xc0,0x40);}if(e.readByte(0xff26)&8)noise.insert(e.readByte(0xff22));}
       QVERIFY2(together,"All four hardware sound channels must be active together.");QVERIFY(noise.contains(0x63));QVERIFY(noise.contains(0x35));QVERIFY(noise.contains(0x19));QCOMPARE(e.readByte(0xff26)&15,0);QCOMPARE(e.readByte(0xff30),quint8(0x01));QCOMPARE(e.readByte(0xff37),quint8(0xef));
       if(target=="gbc"){auto base=QCoreApplication::applicationDirPath();QFile file(base+"/foundry-after-hours.wav");QVERIFY(file.open(QIODevice::WriteOnly));QDataStream wav(&file);wav.setByteOrder(QDataStream::LittleEndian);wav.writeRawData("RIFF",4);wav<<quint32(pcm.size()+36);wav.writeRawData("WAVEfmt ",8);wav<<quint32(16)<<quint16(1)<<quint16(2)<<quint32(48000)<<quint32(192000)<<quint16(4)<<quint16(16);wav.writeRawData("data",4);wav<<quint32(pcm.size());wav.writeRawData(pcm.constData(),pcm.size());}
      }
    }

    void customMusic(){auto p=cf::Project::example();cf::SoundClip clip;clip.id="theme";clip.name="Theme";clip.duty=1;clip.volume=6;clip.notes={{69,30},{0,30},{72,30}};p.sounds<<clip;for(auto &scene:p.scenes)scene.onSceneEnter="sound loop theme";QTemporaryDir dir;
      for(auto target:{QString("gb"),QString("gbc")}){p.target=target;auto built=fastTestRom(p,dir.filePath("music."+target));QVERIFY2(built.success,qPrintable(built.log));cf::EmulatorCore e;QString error;QVERIFY(e.load(built.romPath,error));bool audio=false,silence=false,volume=false;for(int f=0;f<220;++f){e.frame();if(f>40){audio|=(e.readByte(0xff26)&1)!=0;silence|=(e.readByte(0xff26)&1)==0;if(e.readByte(0xff26)&1)volume|=(e.readByte(0xff12)>>4)==6;}}QVERIFY(audio);QVERIFY(silence);QCOMPARE(e.readByte(0xff11)&0xc0,0x40);QVERIFY(volume);}
    }

    void soundEvents(){
        auto p=cf::Project::example();for(auto &scene:p.scenes)scene.onSceneEnter="sound loop pulse\nsound play chime\ntimer start silence";p.timers={{"silence","Silence","sound stop",240,false,false,false}};
        QTemporaryDir dir;
        for(auto target:{QString("gb"),QString("gbc")}){p.target=target;auto build=fastTestRom(p,dir.filePath("sound."+target));QVERIFY2(build.success,qPrintable(build.log));
            cf::EmulatorCore e;QString error;QVERIFY2(e.load(build.romPath,error),qPrintable(error));bool audible=false;
            for(int i=0;i<180;++i){e.frame();auto pcm=e.takeAudio();auto *samples=reinterpret_cast<const qint16 *>(pcm.constData());for(int n=0;n<pcm.size()/2;++n)audible|=samples[n]!=0;}
            QVERIFY2(audible,"Scripted sound must produce actual emulated PCM.");QVERIFY(e.readByte(0xff26)&1);QVERIFY(!(e.readByte(0xff26)&2));for(int i=0;i<120;++i)e.frame();QVERIFY(!(e.readByte(0xff26)&3));
        }
    }

    void actualRomExecution(){
        auto p=cf::Project::overworldExample();QTemporaryDir dir;auto build=fastTestRom(p,dir.filePath("world.gbc"));QVERIFY2(build.success,qPrintable(build.log));
        cf::EmulatorCore emu;QString error;QVERIFY2(emu.load(build.romPath,error),qPrintable(error));
        for(int i=0;i<90;++i)emu.frame();auto enemyX=emu.readByte(0xfe11);bool enemyMoved=false;for(int i=0;i<20;++i){emu.frame();enemyMoved|=enemyX!=emu.readByte(0xfe11);}QVERIFY(enemyMoved);auto image=emu.image();QCOMPARE(image.size(),QSize(160,144));QCOMPARE(emu.readByte(0xfe01),quint8(52));
        // The sign immediately above spawn is solid; holding Up must not move the player.
        auto spriteY=emu.readByte(0xfe00);for(int i=0;i<30;++i)emu.frame(1<<6);QCOMPARE(emu.readByte(0xfe00),spriteY);
        emu.frame();emu.frame(1);for(int i=0;i<60;++i)emu.frame();
        QVERIFY(!(emu.readByte(0xff40)&2));QVERIFY(image!=emu.image());QCOMPARE(image.pixel(80,8),emu.image().pixel(80,8));
        emu.frame(2);for(int i=0;i<60;++i)emu.frame();QVERIFY(emu.readByte(0xff40)&2);
        for(int i=0;i<120;++i)emu.frame(1<<4);QVERIFY(emu.readByte(0xff43)>0);QVERIFY(image!=emu.image());
        // Loading a bad file preserves the running core.
        QVERIFY(!emu.load(dir.filePath("missing.gbc"),error));QVERIFY(emu.loaded());
    }
    void animatedSprites(){
        auto p=cf::Project::overworldExample().isolatedArea(0);p.world.actors.clear();p.world.interactions.clear();p.world.playerScript="behavior top_down\nanimation walk\n";QTemporaryDir dir;
        auto pattern=[](cf::EmulatorCore &e){QByteArray bytes;for(int i=0;i<64;++i)bytes.append(char(e.readByte(0x8000+i)));return bytes;};
        for(auto target:{QString("gbc"),QString("gb")}){
            p.target=target;auto build=fastTestRom(p,dir.filePath("animated."+target));QVERIFY2(build.success,qPrintable(build.log));cf::EmulatorCore e;QString error;QVERIFY(e.load(build.romPath,error));for(int i=0;i<120;++i)e.frame();QSet<QByteArray> frames;QVector<int> intervals;QByteArray previous;int transition=-1;auto tileData=p.asset(p.world.player)->tileData(target=="gb");
            for(int i=0;i<90;++i){e.frame();auto now=pattern(e);frames.insert(now);bool whole=false;for(int f=0;f<4;++f)whole|=now==tileData.mid(f*64,64);if(whole&&now!=previous){if(transition>=0)intervals<<i-transition;transition=i;previous=now;}}
            qInfo()<<"Animation frame intervals"<<target<<intervals;QVERIFY2(frames.size()>1,"Looping sprites must update actual emulated VRAM.");
            p.world.playerScript="behavior top_down\nanimation wave\n";build=fastTestRom(p,dir.filePath("once."+target));QVERIFY2(build.success,qPrintable(build.log));QVERIFY(e.load(build.romPath,error));for(int i=0;i<240;++i)e.frame();auto last=pattern(e);auto *sprite=p.asset(p.world.player);QCOMPARE(last,sprite->tileData(target=="gb").mid(3*64,64));for(int i=0;i<90;++i)e.frame();QCOMPARE(pattern(e),last);p.world.playerScript="behavior top_down\nanimation walk\n";
        }
    }
    void gameSystems(){
        auto p=cf::Project::overworldExample().isolatedArea(0);auto &w=p.world;w.actors.clear();w.objects.clear();w.interactions.clear();w.exits.clear();w.collision.fill(0);w.spawnX=4;w.spawnY=4;
        p.variables={{"score",0}};p.scripts.clear();p.npcs.clear();p.items={{"tonic","Tonic","","",5,true,"add score 1\ndialogue quest"}};p.storages={{"inventory","Inventory",4,{}},{"chest","Chest",4,{{"tonic",1}}}};
        w.objects<<cf::WorldObject{"box","Chest","storage","chest","","",4,3,3,true};cf::Actor enemy{"enemy","Enemy",w.player,cf::behaviorTemplate("idle"),4,6};enemy.health=1;enemy.onDefeat="add score 2\ndialogue welcome";w.actors<<enemy;
        p.prompts[0].choices[0].condition="score >= 3";p.prompts[0].choices[1].actions.clear();p.prompts[p.indexOf("quest")].choices[0].condition="score >= 1";
        QTemporaryDir dir;for(auto target:{QString("gb"),QString("gbc")}){p.target=target;auto build=fastTestRom(p,dir.filePath("systems."+target));QVERIFY2(build.success,qPrintable(build.log));cf::EmulatorCore e;QString error;QVERIFY(e.load(build.romPath,error));auto frames=[&](int n){for(int i=0;i<n;++i)e.frame();};auto press=[&](int key){e.frame();e.frame(key);frames(45);};auto glyph=[&](int x,int y){return e.readByte(((e.readByte(0xff40)&8)?0x9c00:0x9800)+y*32+x);};frames(100);
        press(1);QCOMPARE(glyph(1,1),quint8('C'-32)); // ray opens the solid chest
        press(1);QCOMPARE(glyph(1,4),quint8('E'-32)); // transferred its only item
        press(2);press(1<<2);QCOMPARE(glyph(1,1),quint8('I'-32)); // inventory
        press(1);QVERIFY(!(e.readByte(0xff40)&2)); // use script requests dialogue
        int row=cf::textLayout(p.prompts[p.indexOf("quest")]).choiceRow;QCOMPARE(glyph(1,row),quint8('>'-32)); // score condition became true
        press(2);e.frame(1<<7);frames(1);e.frame();frames(20);press(2); // face south and attack enemy
        QVERIFY(!(e.readByte(0xff40)&2));row=cf::textLayout(p.prompts[0]).choiceRow;QCOMPARE(glyph(1,row),quint8('>'-32)); // defeat reward reached score 3
        }
    }
    void remappedControls(){
        auto p=cf::Project::overworldExample().isolatedArea(0);p.controls={32,16,128};QTemporaryDir dir;auto build=fastTestRom(p,dir.filePath("controls.gbc"));QVERIFY2(build.success,qPrintable(build.log));cf::EmulatorCore emu;QString error;QVERIFY(emu.load(build.romPath,error));for(int i=0;i<90;++i)emu.frame();emu.frame(2);for(int i=0;i<60;++i)emu.frame();QVERIFY(!(emu.readByte(0xff40)&2));emu.frame(1<<3);for(int i=0;i<60;++i)emu.frame();QVERIFY(emu.readByte(0xff40)&2);QCOMPARE(emu.readByte(0xfe01),quint8(52));
    }
    void areasAndDmg(){
        auto p=cf::Project::overworldExample();p.world.spawnX=27;p.world.spawnY=8;QTemporaryDir dir;
        auto build=fastTestRom(p,dir.filePath("areas.gbc"));QVERIFY2(build.success,qPrintable(build.log));
        cf::EmulatorCore emu;QString error;QVERIFY(emu.load(build.romPath,error));for(int i=0;i<90;++i)emu.frame();
        emu.frame(1<<4);emu.frame();emu.frame(1);for(int i=0;i<60;++i)emu.frame();
        QCOMPARE(emu.readByte(0xfe01),quint8(36));QCOMPARE(emu.readByte(0xfe00),quint8(40)); // garden spawn, not field spawn
        auto isolated=p.isolatedArea(1);QVERIFY(isolated.areas.isEmpty());QVERIFY(isolated.world.exits.isEmpty());QCOMPARE(isolated.world.id,QString("area_garden"));
        isolated.target="gb";build=fastTestRom(isolated,dir.filePath("garden.gb"));QVERIFY2(build.success,qPrintable(build.log));QVERIFY(emu.load(build.romPath,error));for(int i=0;i<90;++i)emu.frame();
        QCOMPARE(emu.readByte(0xfe01),quint8(36));QCOMPARE(emu.image().size(),QSize(160,144));
    }
};
QTEST_GUILESS_MAIN(EmulatorTests)
#include "emulator_tests.moc"
