#include "world_editor.h"
#include <QtTest>
#include "event_tools.h"
#include "importer.h"
#include "animation_editor.h"
#include "sound_editor.h"
#include "piano_roll.h"
#include <QtWidgets>
class MapCanvasTests:public QObject {
 Q_OBJECT
private slots:
 void removeSpriteSheet(){
  cf::AnimationEditor editor;auto p=cf::Project::overworldExample();
  auto asset=p.assets.last();asset.id="unused_sprite";asset.name="Unused sprite";asset.kind="sprite";p.assets<<asset;
  const auto original=p;
  editor.onEdit=[&](const cf::Project &next){p=next;editor.setProject(p);};editor.setProject(p);
  QListWidget *sprites=nullptr;for(auto *list:editor.findChildren<QListWidget *>())for(int i=0;i<list->count();++i)if(list->item(i)->data(Qt::UserRole).toString()==asset.id){sprites=list;list->setCurrentRow(i);break;}
  QVERIFY(sprites);QPushButton *remove=nullptr;for(auto *button:editor.findChildren<QPushButton *>())if(button->text()=="Remove sprite sheet…")remove=button;
  QVERIFY(remove);QVERIFY(remove->isEnabled());
  auto answer=[](QMessageBox::StandardButton choice){QTimer::singleShot(0,[choice]{auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget());if(box)box->button(choice)->click();});};
  answer(QMessageBox::No);remove->click();QVERIFY(p.asset(asset.id));
  answer(QMessageBox::Yes);remove->click();QVERIFY(!p.asset(asset.id));QCOMPARE(p.assets.size(),original.assets.size()-1);QVERIFY(p.validate().isEmpty());
  editor.setProject(original);bool restored=false;for(int i=0;i<sprites->count();++i)if(sprites->item(i)->data(Qt::UserRole).toString()==asset.id)restored=true;QVERIFY(restored);
  editor.setProject(cf::Project::blank());QVERIFY(!remove->isEnabled());
 }
 void removedSpriteClearsPlacement(){
  cf::WorldEditor editor;auto p=cf::Project::overworldExample();
  auto asset=p.assets.last();asset.id="unused_sprite";p.assets<<asset;editor.setProject(p);editor.placeSprite(asset.id);
  cf::MapCanvas *canvas=nullptr;for(auto *widget:editor.findChildren<QWidget *>())if(auto *map=dynamic_cast<cf::MapCanvas *>(widget))canvas=map;QVERIFY(canvas);QCOMPARE(canvas->tool,6);QVERIFY(!canvas->placementImage.isNull());
  p.assets.removeLast();editor.setProject(p);QCOMPARE(canvas->tool,5);QVERIFY(canvas->placementImage.isNull());QVERIFY(!canvas->placementSprite);
 }
 void blockAssetRemovalReferences(){
  for(int use=0;use<8;++use){
   auto p=cf::Project::overworldExample();auto asset=p.assets.last();asset.id="used_asset";p.assets<<asset;
   switch(use){
    case 0:p.world.player=asset.id;break;
    case 1:p.world.tileset=asset.id;break;
    case 2:p.areas[0].actors<<cf::Actor{"used","Used character",asset.id};break;
    case 3:p.npcs<<cf::NpcTemplate{"used","Used NPC",cf::Actor{"used","Used",asset.id}};break;
    case 4:p.prompts[0].portrait=asset.id;break;
    case 5:{cf::Item item;item.icon=asset.id;p.items<<item;break;}
    case 6:{cf::ScreenElement image;image.type="image";image.image=asset.id;p.scenes[0].elements<<image;break;}
    case 7:p.areas[0].player=asset.id;break;
   }
   const auto before=p.toJson();cf::Project result;bool warned=false;
   QTimer::singleShot(0,[&]{auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget());if(box){warned=box->icon()==QMessageBox::Information && box->text().contains("Change or remove these uses") && !box->informativeText().isEmpty();box->accept();}});
   QVERIFY(!cf::removeAsset(nullptr,p,asset.id,result));QVERIFY(warned);QCOMPARE(p.toJson(),before);
  }
 }
 void editAllSongChannels(){cf::SoundEditor editor;auto p=cf::Project::example();editor.onEdit=[&](const cf::Project &next){p=next;editor.setProject(p);};editor.setProject(p);auto click=[&](QString name){for(auto *b:editor.findChildren<QPushButton *>())if(b->text()==name){b->click();return true;}return false;};QVERIFY(click("+ Demo song"));QCOMPARE(p.sounds[0].channels.size(),4);for(auto &t:p.sounds[0].channels)QVERIFY(t.enabled);auto *tabs=editor.findChild<QTabBar *>();QVERIFY(tabs);for(int ch=0;ch<4;++ch){tabs->setCurrentIndex(ch);auto *roll=editor.findChild<cf::PianoRoll *>();QVERIFY(roll);QVERIFY(click("▶ Play"));QVERIFY(click("■ Stop"));}tabs->setCurrentIndex(2);auto *table=editor.findChild<QTableWidget *>();QVERIFY(table);qobject_cast<QSpinBox *>(table->cellWidget(0,0))->setValue(7);QCOMPARE(p.sounds[0].channels[2].wave[0],7);tabs->setCurrentIndex(3);QVERIFY(click("+ Effect"));QVERIFY(p.sounds[1].effect);QVERIFY(p.sounds[1].channels[1].enabled);}

 void pianoRollGestures(){cf::PianoRoll roll;cf::SoundClip clip;clip.notes={{60,15},{0,15},{64,15}};roll.setPattern(clip);QVector<cf::SoundNote> result;roll.onEdit=[&](const QVector<cf::SoundNote> &n){result=n;};auto point=[](int tick,int note){return QPoint(64+tick*4,26+(107-note)*18+8);};
 QTest::mousePress(&roll,Qt::LeftButton,Qt::NoModifier,point(5,60));QTest::mouseMove(&roll,point(20,63));QTest::mouseRelease(&roll,Qt::LeftButton,Qt::NoModifier,point(20,63));QCOMPARE(result[0].note,0);QCOMPARE(result[0].ticks,15);QCOMPARE(result[1].note,63);
 QTest::mousePress(&roll,Qt::LeftButton,Qt::NoModifier,point(30,63)-QPoint(3,0));QTest::mouseMove(&roll,point(45,63));QTest::mouseRelease(&roll,Qt::LeftButton,Qt::NoModifier,point(45,63));QCOMPARE(result[1].ticks,30);QTest::keyClick(&roll,Qt::Key_Delete);for(auto n:result)QCOMPARE(n.note,0);
 }

 void composeMusic(){cf::SoundEditor editor;auto p=cf::Project::example();editor.onEdit=[&](const cf::Project &next){p=next;editor.setProject(p);};editor.setProject(p);editor.show();auto button=[&](QString text){for(auto *b:editor.findChildren<QPushButton *>())if(b->text()==text)return b;return static_cast<QPushButton *>(nullptr);};QVERIFY(button("+ Song"));button("+ Song")->click();QCOMPARE(p.sounds.size(),1);button("+ Note")->click();QCOMPARE(p.sounds[0].notes.size(),5);auto *roll=editor.findChild<cf::PianoRoll *>();QVERIFY(roll);QTest::mouseClick(roll,Qt::LeftButton,Qt::NoModifier,QPoint(65,26+(107-69)*18+8));QCOMPARE(p.sounds[0].notes[0].note,69);button("▶ Play")->click();button("■ Stop")->click();editor.hide();}

 void insertSoundEvents(){
  auto p=cf::Project::example();auto *source=new QPlainTextEdit;std::unique_ptr<QWidget> tools(cf::eventTools(source,[&]{return p;}));tools->show();
  auto *insert=tools->findChildren<QPushButton *>().first();
  for(auto command:{QString("Sound / Play one-shot"),QString("Sound / Start loop"),QString("Sound / Stop all")}){
   QTimer::singleShot(0,[command]{auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());if(!dialog)return;auto *combo=dialog->findChild<QComboBox *>();combo->setCurrentIndex(combo->findText(command));auto *buttons=dialog->findChild<QDialogButtonBox *>();buttons->button(QDialogButtonBox::Ok)->click();});
   insert->click();
  }
  QCOMPARE(source->toPlainText(),QString("sound play blip\nsound loop blip\nsound stop\n"));
 }

 void stampUsesTheTileUnderThePointer(){cf::MapCanvas canvas;canvas.project=cf::Project::overworldExample();canvas.tool=6;QPoint placed(-1,-1);canvas.onPlace=[&](QPoint p){placed=p;};for(int scale:{1,2,3,6}){canvas.zoom=scale;canvas.reload();for(int offset:{0,4,7}){QTest::mouseClick(&canvas,Qt::LeftButton,Qt::NoModifier,QPoint((10*8+offset)*scale,(11*8+offset)*scale));QCOMPARE(placed,QPoint(10,11));}QTest::mouseClick(&canvas,Qt::LeftButton,Qt::NoModifier,QPoint(11*8*scale,12*8*scale));QCOMPARE(placed,QPoint(11,12));}}
 void dragCommitsOnceAtTheCorrectTile(){cf::MapCanvas canvas;canvas.project=cf::Project::overworldExample();canvas.tool=5;canvas.zoom=3;canvas.reload();cf::MapSelection selected,moved;QPoint destination;int commits=0;canvas.onSelect=[&](cf::MapSelection s){selected=s;};canvas.onMove=[&](cf::MapSelection s,QPoint p){moved=s;destination=p;++commits;};auto pixel=[](int x,int y){return QPoint((x*8+4)*3,(y*8+4)*3);};QTest::mousePress(&canvas,Qt::LeftButton,Qt::NoModifier,pixel(9,10));QCOMPARE(selected.kind,QString("object"));QTest::mouseMove(&canvas,pixel(10,10));QCOMPARE(commits,0);QTest::mouseRelease(&canvas,Qt::LeftButton,Qt::NoModifier,pixel(10,10));QCOMPARE(commits,1);QCOMPARE(destination,QPoint(10,10));QCOMPARE(moved.id,selected.id);}
};
QTEST_MAIN(MapCanvasTests)
#include "map_canvas_tests.moc"
