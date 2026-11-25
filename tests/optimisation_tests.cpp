#include "rom_test_helpers.h"
#include "emulator_core.h"
#include "menu_model.h"
#include <QtTest>
#include <QTemporaryDir>
class OptimisationTests:public QObject {
 Q_OBJECT
 inline static bool color=true;
 static void frames(cf::EmulatorCore &e,int count=80){while(count--)e.frame();}
 static void press(cf::EmulatorCore &e,int key){e.frame();e.frame(key);frames(e);}
 static quint8 glyph(cf::EmulatorCore &e,int x,int y){for(int i=0;color&&i<20&&(e.readByte(0xff4f)&1);++i)e.frame();return e.readByte(((e.readByte(0xff40)&8)?0x9c00:0x9800)+y*32+x);}
private slots:
 void bankedDialogueWithSaveAndButtonEvents(){auto p=cf::Project::overworldExample();QString menu,error;QVERIFY(cf::addMenuPreset(p,"pause",menu,error));p.pauseMenu.clear();p.world.onStart="on_press start\nmenu "+menu+"\nend\non_press select\ndialogue banked_63\nend";auto original=p.prompts;p.prompts.clear();for(int i=0;i<64;++i){cf::Prompt n;n.id=i<original.size()?original[i].id:QString("banked_%1").arg(i);n.title=QString("Dialogue %1").arg(i);n.text="Banked body text.";n.footer="Footer";n.textbox={0,0,20,18};n.choices={{"Continue",i==63?original[0].id:i+1<original.size()?original[i+1].id:QString("banked_%1").arg(i+1)}};n.onEnter="add coins 1";p.prompts<<n;}p.entry=p.prompts[0].id;QTemporaryDir dir;for(auto target:{QString("gb"),QString("gbc")}){p.target=target;color=target=="gbc";QVERIFY2(p.validate().isEmpty(),qPrintable(p.validate().join('\n')));auto built=fastTestRom(p,dir.filePath("dialogue."+target));QVERIFY2(built.success,qPrintable(built.log));cf::EmulatorCore e;QVERIFY2(e.load(built.romPath,error),qPrintable(error));frames(e,120);press(e,8);QCOMPARE(glyph(e,3,4),quint8('R'-32));press(e,2);QVERIFY(e.readByte(0xff40)&2);press(e,4);QString label="Dialogue 63";for(int x=0;x<label.size();++x)QCOMPARE(glyph(e,1+x,1),quint8(label[x].unicode()-32));press(e,1);label="Dialogue 0";for(int x=0;x<label.size();++x)QCOMPARE(glyph(e,1+x,1),quint8(label[x].unicode()-32));QVERIFY(e.readByte(0xff40)&0x80);}}

 void maximumMapsAndBankedMenus(){
  auto p=cf::Project::overworldExample();auto prototype=p.world;p.areas.clear();p.maps.resize(1);p.scenes.resize(1);p.maps[0].scene=p.scenes[0].id;p.defaultScene=p.scenes[0].id;p.scenes[0].startArea=prototype.id;
  for(int i=0;i<8;++i){auto w=prototype;w.id=i?QString("large_%1").arg(i):prototype.id;w.mapId=p.maps[0].id;w.resize(64,64);w.spawnX=w.spawnY=4;w.actors.clear();w.objects.clear();w.interactions.clear();w.exits.clear();w.terrainEvents.clear();w.onStart.clear();w.playerScript="behavior top_down\nspeed 1\nevery 1\n";
   quint32 random=12345+i;for(int y=0;y<64;++y)for(int x=0;x<64;++x){random=random*1664525u+1013904223u;w.tiles[y*64+x]=random>>31;w.collision[y*64+x]=char(x==0||y==0||x==63||y==63);}
   w.collision[4*64+6]=1;if(i)p.areas<<w;else p.world=w;
  }
  p.scenes[0].startArea=p.areas.last().id;
  // Distinct long text and element arrays exceed the old resident menu budget.
  for(int i=0;i<20;++i){cf::Scene s{QString("screen_%1").arg(i),QString("Screen %1").arg(i)};s.onBack="return";
   for(int j=0;j<16;++j){cf::ScreenElement e;e.id=QString("button_%1").arg(j);e.name=e.id;e.type="button";e.rect={1,j+1,18,1};e.text=QString("Menu %1 item %2").arg(i).arg(j);e.actions="return";s.elements<<e;}
   p.scenes<<s;
  }
  p.pauseMenu="screen_19";QTemporaryDir dir;
  for(auto target:{QString("gb"),QString("gbc")}){p.target=target;color=target=="gbc";QVERIFY2(p.validate().isEmpty(),qPrintable(p.validate().join('\n')));auto built=fastTestRom(p,dir.filePath("large."+target));QVERIFY2(built.success,qPrintable(built.log));
   cf::EmulatorCore e;QString error;QVERIFY2(e.load(built.romPath,error),qPrintable(error));frames(e,120);
   auto before=e.readByte(0xfe01);for(int f=0;f<40;++f)e.frame(16);frames(e);QVERIFY(e.readByte(0xfe01)>before);auto stopped=e.readByte(0xfe01);for(int f=0;f<20;++f)e.frame(16);QCOMPARE(e.readByte(0xfe01),stopped);
   press(e,8);QString label="Menu 19 item 0";for(int x=0;x<label.size();++x)QCOMPARE(glyph(e,2+x,1),quint8(label[x].unicode()-32));
   // Cursor movement must update marker cells while keeping the LCD enabled.
   press(e,128);QCOMPARE(glyph(e,1,2),quint8('>'-32));QVERIFY(e.readByte(0xff40)&0x80);press(e,1);QVERIFY(e.readByte(0xff40)&2);QCOMPARE(e.readByte(0xfe01),stopped);
  }
 }
};
QTEST_GUILESS_MAIN(OptimisationTests)
#include "optimisation_tests.moc"
