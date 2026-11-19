#include "builder.h"
#include "menu_model.h"
#include "game_script.h"
#include "emulator_core.h"
#include <QtTest>
#include <QTemporaryDir>
class MenuTests:public QObject {
 Q_OBJECT
 inline static bool color=true;
 static void frames(cf::EmulatorCore &e,int n=65){for(int i=0;i<n;++i)e.frame();}
 static void press(cf::EmulatorCore &e,int key){e.frame();e.frame(key);frames(e);}
 static quint8 glyph(cf::EmulatorCore &e,int x,int y){if(color)for(int n=0;n<20&&(e.readByte(0xff4f)&1);++n)e.frame();return e.readByte(((e.readByte(0xff40)&8)?0x9c00:0x9800)+y*32+x);}
 static void expectText(cf::EmulatorCore &e,int x,int y,QString text){for(int i=0;i<text.size();++i)QCOMPARE(glyph(e,x+i,y),quint8(text[i].unicode()-32));}
private slots:
 void worldPositionSurvivesMenusAndLoad(){
  color=true;QTemporaryDir dir;auto p=cf::Project::overworldExample();p.splashTicks=1;QString scene,error;QVERIFY(cf::addMenuPreset(p,"pause",scene,error));p.world.onStart="add coins 1";cf::ScreenElement counter;counter.id="coins";counter.name="Coins";counter.text="Coins {coins}";counter.rect={1,14,18,1};p.scenes[p.sceneIndex(p.pauseMenu)].elements<<counter;
  auto built=cf::buildRom(p,dir.filePath("world.gbc"));QVERIFY2(built.success,qPrintable(built.log));cf::EmulatorCore e;QVERIFY(e.load(built.romPath,error));frames(e,100);for(int i=0;i<12;++i)e.frame(128);frames(e);const auto y=e.readByte(0xfe00),scroll=e.readByte(0xff42);
  press(e,8);expectText(e,1,14,"Coins 1");press(e,128);press(e,1);press(e,128);press(e,1);press(e,2);QVERIFY2(e.readByte(0xff40)&2,qPrintable(QString("BG title tiles: %1 %2, state %3").arg(glyph(e,2,1)).arg(glyph(e,3,1)).arg(e.readByte(0xff40))));
  for(int i=0;i<20;++i)e.frame(128);frames(e);QVERIFY(e.readByte(0xff42)!=scroll||e.readByte(0xfe00)!=y);
  press(e,8);press(e,128);press(e,128);press(e,1);press(e,1);QVERIFY(e.readByte(0xff40)&2);QCOMPARE(e.readByte(0xff42),scroll);QCOMPARE(e.readByte(0xfe00),y);press(e,8);expectText(e,1,14,"Coins 1");
 }
 void keyboardSaveAndPauseOnHardware(){QTemporaryDir dir;for(auto target:{QString("gb"),QString("gbc")}){
  color=target=="gbc";auto p=cf::Project::blank();p.target=target;p.splashTicks=1;p.variables={{"score",0}};p.textFields={{"name","",4}};p.keyboards={{"entry","Enter name","AB",5,0,6}};p.saveConfig.enabled=true;p.saveConfig.id="rom-menu-test";
  cf::Prompt input;input.id="input";input.title="Name";input.text="Hello {name}.";input.textbox={0,8,20,10};input.onEnter="keyboard entry name 4\nset score 7";input.choices={{"Continue","input","","resume"}};p.prompts<<input;
  auto &s=p.scenes[0];s.elements.clear();cf::ScreenElement label;label.id="name";label.name="Name";label.text="{name}";label.rect={1,1,18,1};s.elements<<label;label.id="score";label.text="{score}";label.rect={1,2,18,1};s.elements<<label;
  auto button=[](QString id,int row,QString action){cf::ScreenElement e;e.id=id;e.name=id;e.type="button";e.text=id;e.actions=action;e.rect={1,row,18,1};return e;};
  s.elements<<button("Name",5,"dialogue input")<<button("Save",7,"save write 1")<<button("Change",9,"set score 99\ntext clear name")<<button("Load",11,"save read 1");
  cf::Scene pause{"pause","Pause"};pause.elements<<button("Resume",4,"return");pause.onBack="return";p.scenes<<pause;p.pauseMenu=pause.id;
  QVERIFY2(p.validate().isEmpty(),qPrintable(p.validate().join('\n')));auto built=cf::buildRom(p,dir.filePath("menus."+target));QVERIFY2(built.success,qPrintable(built.log));QFile rom(built.romPath);QVERIFY(rom.open(QIODevice::ReadOnly));auto bytes=rom.readAll();QCOMPARE(quint8(bytes[0x147]),quint8(0x1b));QCOMPARE(quint8(bytes[0x149]),quint8(2));
  cf::EmulatorCore e;QString error;QVERIFY(e.load(built.romPath,error));frames(e,100);press(e,1);expectText(e,1,1,"Enter name");press(e,1);press(e,16);press(e,1);press(e,16);press(e,16);press(e,1);expectText(e,1,cf::textLayout(p.prompts[0]).bodyRow,"Hello AB.");press(e,2);expectText(e,1,1,"AB");expectText(e,1,2,"7");QVERIFY(e.readByte(0xff40)&0x80);
  press(e,128);press(e,1); // Save
  press(e,128);press(e,1);expectText(e,1,2,"99"); // Change
  press(e,128);press(e,1);expectText(e,1,1,"AB");expectText(e,1,2,"7"); // Load
  press(e,8);expectText(e,2,4,"Resume");press(e,2);expectText(e,1,1,"AB"); // Pause / return
  QVERIFY(e.load(built.romPath,error));frames(e,100);press(e,128);press(e,128);press(e,128);press(e,1);expectText(e,1,1,"AB");expectText(e,1,2,"7");
 }}
};
QTEST_GUILESS_MAIN(MenuTests)
#include "menu_tests.moc"
