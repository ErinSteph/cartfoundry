#include "world_editor.h"
#include <QtTest>
#include "event_tools.h"
#include "sound_editor.h"
#include "piano_roll.h"
#include <QtWidgets>
class MapCanvasTests:public QObject {
 Q_OBJECT
private slots:
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
