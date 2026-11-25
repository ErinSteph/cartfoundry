#include "world_editor.h"
#include <QtTest>
#include "event_tools.h"
#include "importer.h"
#include "animation_editor.h"
#include "screen_builder.h"
#include "screen_model.h"
#include "menu_model.h"
#include "menu_tools.h"
#include "pixel_editor.h"
#include "sound_editor.h"
#include "midi.h"
#include "piano_roll.h"
#include "script_workspace.h"
#include <QtWidgets>
class MapCanvasTests:public QObject {
 Q_OBJECT
private slots:
 void midiImportReviewAndExport(){QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QTemporaryDir dir;auto input=dir.filePath("import.mid"),output=dir.filePath("export.mid");QByteArray bytes;QStringList warnings;QString error;QVERIFY(cf::exportMidi(cf::foundryDemoSong(),bytes,warnings,error));QFile file(input);QVERIFY(file.open(QIODevice::WriteOnly));QCOMPARE(file.write(bytes),bytes.size());file.close();auto p=cf::Project::blank();cf::SoundEditor editor;editor.setProject(p);int changes=0;editor.onEdit=[&](const cf::Project &next){p=next;++changes;editor.setProject(p);};auto *import=editor.findChild<QPushButton *>("importMidi"),*exportButton=editor.findChild<QPushButton *>("exportMidi");QVERIFY(import&&exportButton);QVERIFY(!exportButton->isEnabled());QTimer watchdog;watchdog.setSingleShot(true);connect(&watchdog,&QTimer::timeout,&editor,[]{if(auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()))dialog->reject();});
 auto select=[&](bool accept){watchdog.start(10000);QTimer::singleShot(0,&editor,[&,accept]{auto *picker=qobject_cast<QFileDialog *>(QApplication::activeModalWidget());if(!picker)return;picker->selectFile(input);QTimer::singleShot(0,&editor,[&,accept]{auto *review=qobject_cast<QDialog *>(QApplication::activeModalWidget());if(!review||review->objectName()!="midiImportReview")return;if(accept)review->accept();else review->reject();});static_cast<QDialog *>(picker)->accept();});import->click();watchdog.stop();};select(false);QCOMPARE(changes,0);QCOMPARE(p.sounds.size(),0);select(true);QCOMPARE(changes,1);QCOMPARE(p.sounds.size(),1);auto waveOnly=cf::foundryDemoSong();for(int c=0;c<4;++c)waveOnly.channels[c].enabled=c==2;QVERIFY(cf::exportMidi(waveOnly,bytes,warnings,error));QVERIFY(file.open(QIODevice::WriteOnly|QIODevice::Truncate));QCOMPARE(file.write(bytes),bytes.size());file.close();QVERIFY(exportButton->isEnabled());select(true);QCOMPARE(changes,2);QCOMPARE(p.sounds.size(),2);QCOMPARE(editor.findChild<QTabBar *>()->currentIndex(),2);QVERIFY(p.sounds[0].id!=p.sounds[1].id);auto before=p.toJson();watchdog.start(10000);QTimer::singleShot(0,&editor,[&]{auto *picker=qobject_cast<QFileDialog *>(QApplication::activeModalWidget());if(!picker)return;picker->selectFile(output);static_cast<QDialog *>(picker)->accept();});exportButton->click();watchdog.stop();QCOMPARE(changes,2);QCOMPARE(p.toJson(),before);QFile saved(output);QVERIFY(saved.open(QIODevice::ReadOnly));cf::MidiImport result;QVERIFY2(cf::importMidi(saved.readAll(),result,error),qPrintable(error));}

 void buttonPressPausesWorldPreview(){auto p=cf::Project::overworldExample();QString menu,error;QVERIFY(cf::addMenuPreset(p,"pause",menu,error));p.pauseMenu.clear();p.world.onStart="on_press start\nif coins >= 0\nmenu "+menu+"\nend\nend";for(auto &scene:p.scenes)if(scene.kind=="world")scene.onSceneEnter.clear();cf::WorldPlay view;view.setProject(p);view.show();view.activateWindow();view.setFocus();QTest::qWait(20);auto original=view.grab().toImage();for(int n=0;n<2;++n){QTest::keyClick(&view,Qt::Key_Return);QTest::qWait(10);auto paused=view.grab().toImage();QVERIFY(paused!=original);auto expected=cf::sceneImage(p,p.scenes[p.sceneIndex(menu)],nullptr,2).scaled(view.size());QCOMPARE(paused.copy(48,96,384,24).convertToFormat(QImage::Format_ARGB32),expected.copy(48,96,384,24).convertToFormat(QImage::Format_ARGB32));QKeyEvent repeated(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier,QString(),true);QApplication::sendEvent(&view,&repeated);QCOMPARE(view.grab().toImage(),paused);QTest::keyClick(&view,Qt::Key_X);QTest::qWait(10);QVERIFY(view.grab().toImage()!=paused);}}

 void insertButtonPressHandler(){auto p=cf::Project::overworldExample();auto *source=new QPlainTextEdit;std::unique_ptr<QWidget> tools(cf::eventTools(source,[&]{return p;}));tools->show();bool found=false;QTimer::singleShot(0,[&]{auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());if(!dialog)return;auto *commands=dialog->findChild<QComboBox *>();int index=commands->findText("Input / On button press");found=index>=0;if(!found){dialog->reject();return;}commands->setCurrentIndex(index);auto combos=dialog->findChildren<QComboBox *>();for(auto *combo:combos)if(combo!=commands)combo->setCurrentIndex(combo->findText("start"));dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();});tools->findChildren<QPushButton *>().first()->click();QVERIFY(found);QVERIFY(source->toPlainText().contains("on_press start\n"));QVector<CFInstruction> code;QString error;QVERIFY2(cf::GameCompiler(p).program(source->toPlainText(),code,error),qPrintable(error));}

 void deleteSceneFromScenesTab(){
  auto p=cf::Project::scenesExample();const int original=p.scenes.size();cf::ScreenBuilder editor;editor.onEdit=[&](const cf::Project &next){p=next;editor.setProject(p);};editor.setProject(p);
  QPushButton *remove=nullptr;for(auto *button:editor.findChildren<QPushButton*>())if(button->text()=="Delete scene…")remove=button;QVERIFY(remove);QVERIFY(remove->isEnabled());
  QComboBox *scenes=nullptr;for(auto *box:editor.findChildren<QComboBox*>())if(!scenes&&box->count()==original&&box->itemText(0).contains(" ["))scenes=box;QVERIFY(scenes);
  auto answer=[](QMessageBox::StandardButton choice){QTimer::singleShot(0,[choice]{auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget());if(box)box->button(choice)->click();});};
  scenes->setCurrentIndex(p.sceneIndex("scene_about"));
  answer(QMessageBox::No);remove->click();QCOMPARE(p.scenes.size(),original);QVERIFY(p.sceneIndex("scene_about")>=0);
  answer(QMessageBox::Yes);remove->click();QCOMPARE(p.scenes.size(),original-1);QCOMPARE(p.sceneIndex("scene_about"),-1);QVERIFY(p.sceneIndex(p.defaultScene)>=0);
  QCOMPARE(scenes->count(),original-1);QVERIFY(scenes->currentIndex()>=0);
  // A World scene can be deleted while another exists; its maps move to that one.
  scenes->setCurrentIndex(p.sceneIndex("scene_garden"));answer(QMessageBox::Yes);remove->click();QCOMPARE(p.sceneIndex("scene_garden"),-1);for(const auto &m:p.maps)QCOMPARE(m.scene,QString("scene_main"));
  // Refused: the last World scene holds the maps, and the user is told why instead of nothing happening.
  scenes->setCurrentIndex(p.sceneIndex("scene_main"));bool warned=false;QTimer::singleShot(0,[&]{auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget());if(box){warned=box->text().contains("World");box->button(QMessageBox::Ok)->click();}});remove->click();QVERIFY(warned);QVERIFY(p.sceneIndex("scene_main")>=0);
  // Down to one scene the button disables itself.
  auto single=cf::Project::blank();editor.setProject(single);QVERIFY(!remove->isEnabled());
 }
 void menuAndKeyboardBuilders(){
  auto p=cf::Project::blank();cf::ScreenBuilder editor;editor.onEdit=[&](const cf::Project &next){p=next;editor.setProject(p);};editor.setProject(p);
  QPushButton *preset=nullptr;for(auto *button:editor.findChildren<QPushButton*>())if(button->text()=="+ Menu preset…")preset=button;QVERIFY(preset);
  QTimer::singleShot(0,[]{if(auto *dialog=qobject_cast<QInputDialog*>(QApplication::activeModalWidget()))dialog->accept();});preset->click();QCOMPARE(p.pauseMenu,QString("pause_menu"));QVERIFY(p.saveConfig.enabled);QVERIFY2(p.validate().isEmpty(),qPrintable(p.validate().join('\n')));
  bool changed=false;QTimer::singleShot(0,[&]{auto *dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!dialog)return;auto *tabs=dialog->findChild<QTabWidget*>();tabs->setCurrentIndex(1);auto *keys=dialog->findChild<QTableWidget*>("keyboardTable");keys->item(0,2)->setText("ABC123");auto *cols=qobject_cast<QSpinBox*>(keys->cellWidget(0,3));cols->setValue(3);tabs->setCurrentIndex(2);auto *detected=dialog->findChild<QListWidget*>("detectedSaveFields");changed=detected&&detected->count()>=3;dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();});QVERIFY(cf::menuBuilders(nullptr,p));QVERIFY(changed);QCOMPARE(p.keyboards[0].characters,QString("ABC123"));QCOMPARE(p.keyboards[0].columns,3);
  cf::PixelEditor pixels;bool sprite=false,tiles=false;for(auto *button:pixels.findChildren<QPushButton*>()){sprite|=button->text()=="Import sprite / sheet…";tiles|=button->text()=="Import tileset…";}QVERIFY(sprite);QVERIFY(tiles);
 }
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

 void pianoRollShiftAndZoom(){
  cf::PianoRoll roll;cf::SoundClip clip;clip.notes={{60,15},{0,15},{64,15}};roll.setPattern(clip);QVector<cf::SoundNote> result;roll.onEdit=[&](const QVector<cf::SoundNote> &n){result=n;};QCOMPARE(roll.zoomPercent(),100);
  auto point=[](int tick,int note,int scale=4){return QPoint(64+tick*scale,26+(107-note)*18+8);};
  auto move=[&](QPoint to,Qt::KeyboardModifiers modifiers){QMouseEvent e(QEvent::MouseMove,to,roll.mapToGlobal(to),Qt::NoButton,Qt::LeftButton,modifiers);QApplication::sendEvent(&roll,&e);};
  // Resizing snaps to the grid: dragging the right edge to tick 9 rounds back to the full 15-tick note.
  QTest::mousePress(&roll,Qt::LeftButton,Qt::NoModifier,point(14,60));move(point(9,60),Qt::NoModifier);QTest::mouseRelease(&roll,Qt::LeftButton,Qt::NoModifier,point(9,60));QCOMPARE(result[0].ticks,15);
  // Holding Shift ignores the grid: the same drag now lands exactly on tick 9 and the rest closes up behind it.
  QTest::mousePress(&roll,Qt::LeftButton,Qt::NoModifier,point(14,60));move(point(9,60),Qt::ShiftModifier);QTest::mouseRelease(&roll,Qt::LeftButton,Qt::NoModifier,point(9,60));
  QCOMPARE(result[0].note,60);QCOMPARE(result[0].ticks,9);QCOMPARE(result[1].note,0);QCOMPARE(result[1].ticks,21);QCOMPARE(result[2].note,64);QCOMPARE(result[2].ticks,15);
  // Shift also moves a note in single ticks: 7 ticks right gives a 7-tick rest in front instead of snapping back to 0 or 15.
  QTest::mousePress(&roll,Qt::LeftButton,Qt::NoModifier,point(3,60));move(point(10,60),Qt::ShiftModifier);QTest::mouseRelease(&roll,Qt::LeftButton,Qt::NoModifier,point(10,60));
  QCOMPARE(result[0].note,0);QCOMPARE(result[0].ticks,7);QCOMPARE(result[1].note,60);QCOMPARE(result[1].ticks,9);
  // ...and a click with Shift draws a new note at an off-grid start.
  QTest::mouseClick(&roll,Qt::LeftButton,Qt::ShiftModifier,point(50,72));int tick=0,found=-1;for(auto n:result){if(n.note==72)found=tick;tick+=n.ticks;}QCOMPARE(found,50);
 }
 void pianoRollZoom(){
  cf::PianoRoll roll;cf::SoundClip clip;clip.notes={{0,60}};roll.setPattern(clip);QVector<cf::SoundNote> result;roll.onEdit=[&](const QVector<cf::SoundNote> &n){result=n;};
  int reported=0;roll.onZoom=[&](int percent){reported=percent;};const int base=roll.width()-64;
  roll.setZoomPercent(200);QCOMPARE(roll.zoomPercent(),200);QCOMPARE(reported,200);QCOMPARE(roll.width()-64,base*2);
  QTest::mouseClick(&roll,Qt::LeftButton,Qt::NoModifier,QPoint(64+30*8,26+(107-67)*18+8));QCOMPARE(result[0].ticks,30);QCOMPARE(result[1].note,67);QCOMPARE(result[1].ticks,15);
  roll.setZoomPercent(50);QCOMPARE(roll.width()-64,base/2);
  QTest::mouseClick(&roll,Qt::LeftButton,Qt::NoModifier,QPoint(64+45*2,26+(107-70)*18+8));int tick=0,found=-1;for(auto n:result){if(n.note==70)found=tick;tick+=n.ticks;}QCOMPARE(found,45);
  // Ctrl (Command on a Mac) + wheel steps through the zoom levels; a plain wheel does not zoom.
  auto wheel=[&](int delta,Qt::KeyboardModifiers modifiers){QWheelEvent e(QPointF(100,50),roll.mapToGlobal(QPointF(100,50)),QPoint(),QPoint(0,delta),Qt::NoButton,modifiers,Qt::NoScrollPhase,false);QApplication::sendEvent(&roll,&e);};
  wheel(120,Qt::NoModifier);QCOMPARE(roll.zoomPercent(),50);wheel(120,Qt::ControlModifier);QCOMPARE(roll.zoomPercent(),75);QCOMPARE(reported,75);wheel(-120,Qt::ControlModifier);wheel(-120,Qt::ControlModifier);QCOMPARE(roll.zoomPercent(),25);wheel(-120,Qt::ControlModifier);QCOMPARE(roll.zoomPercent(),25);
  roll.setZoomPercent(9999);QCOMPARE(roll.zoomPercent(),400);
  // Zooming keeps the view centred on the same moment in the song.
  cf::SoundClip longClip;longClip.notes={{60,255},{62,255},{64,255}};QScrollArea area;auto *wide=new cf::PianoRoll;wide->setPattern(longClip);area.setWidget(wide);area.resize(500,300);area.show();QTest::qWait(30);auto *bar=area.horizontalScrollBar();bar->setValue(600);
  const double before=double(bar->value()+bar->pageStep()/2-64)/4;wide->setZoomPercent(200);QTest::qWait(30);const double after=double(bar->value()+bar->pageStep()/2-64)/8;QVERIFY2(qAbs(before-after)<3,qPrintable(QString("centre moved from tick %1 to %2").arg(before).arg(after)));
 }
 void soundEditorZoomBox(){
  cf::SoundEditor editor;auto p=cf::Project::example();editor.onEdit=[&](const cf::Project &next){p=next;editor.setProject(p);};editor.setProject(p);editor.show();
  for(auto *b:editor.findChildren<QPushButton *>())if(b->text()=="+ Song")b->click();QCOMPARE(p.sounds.size(),1);
  auto *roll=editor.findChild<cf::PianoRoll *>();QVERIFY(roll);QVERIFY(roll->isEnabled());QComboBox *zoom=nullptr;for(auto *box:editor.findChildren<QComboBox *>())if(box->findText("100%")>=0&&box->findText("400%")>=0)zoom=box;QVERIFY(zoom);QCOMPARE(zoom->currentText(),QString("100%"));
  const int base=roll->width();zoom->setCurrentText("200%");QCOMPARE(roll->zoomPercent(),200);QVERIFY(roll->width()>base);
  QWheelEvent wheel(QPointF(100,50),roll->mapToGlobal(QPointF(100,50)),QPoint(),QPoint(0,120),Qt::NoButton,Qt::ControlModifier,Qt::NoScrollPhase,false);QApplication::sendEvent(roll,&wheel);QCOMPARE(roll->zoomPercent(),300);QCOMPARE(zoom->currentText(),QString("300%"));
  editor.hide();
 }
 void deleteScriptFromScriptsTab(){
  auto p=cf::Project::scenesExample();p.scripts.clear();p.scripts<<cf::GameScript{"script_a","Alpha","# a\n"}<<cf::GameScript{"script_b","Beta","call script_a\n"}<<cf::GameScript{"script_c","Gamma","# c\n"};
  cf::ScriptWorkspace tab;tab.onEdit=[&](const cf::Project &next){p=next;};tab.setProject(p);
  QComboBox *types=nullptr;for(auto *box:tab.findChildren<QComboBox *>())if(box->findText("Scripts")>=0&&box->findText("Variables")<0&&box->findText("Timers")>=0)types=box;QVERIFY(types);
  QPushButton *remove=nullptr;for(auto *button:tab.findChildren<QPushButton *>())if(button->text()=="Delete script…")remove=button;QVERIFY(remove);
  types->setCurrentText("Scenes");QVERIFY(remove->isHidden());types->setCurrentText("Scripts");QVERIFY(!remove->isHidden());
  QListWidget *elements=nullptr;for(auto *list:tab.findChildren<QListWidget *>())for(int i=0;i<list->count();++i)if(list->item(i)->data(Qt::UserRole).toString()=="script_a")elements=list;QVERIFY(elements);
  auto choose=[&](const QString &id){for(int i=0;i<elements->count();++i)if(elements->item(i)->data(Qt::UserRole).toString()==id)elements->setCurrentRow(i);};
  QString asked;auto answer=[&](QMessageBox::StandardButton choice){QTimer::singleShot(0,[&asked,choice]{auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget());if(box){asked=box->text();box->button(choice)->click();}});};
  choose("script_a");answer(QMessageBox::No);remove->click();QCOMPARE(p.scripts.size(),3);
  // Script B still calls A, so the confirmation warns about it; the delete itself goes ahead on Yes.
  answer(QMessageBox::Yes);remove->click();QVERIFY2(asked.contains("call script_a"),qPrintable(asked));QCOMPARE(p.scripts.size(),2);QCOMPARE(p.scripts[0].id,QString("script_b"));QVERIFY(p.idMentions("script_a")>=1);
  // A script nothing refers to is deleted without any warning about leftovers.
  choose("script_c");asked.clear();answer(QMessageBox::Yes);remove->click();QCOMPARE(p.scripts.size(),1);QVERIFY(!asked.contains("Also:"));QCOMPARE(p.idMentions("script_c"),0);
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
