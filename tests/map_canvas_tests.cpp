#include "world_editor.h"
#include <QtTest>
class MapCanvasTests:public QObject {
 Q_OBJECT
private slots:
 void stampUsesTheTileUnderThePointer(){cf::MapCanvas canvas;canvas.project=cf::Project::overworldExample();canvas.tool=6;QPoint placed(-1,-1);canvas.onPlace=[&](QPoint p){placed=p;};for(int scale:{1,2,3,6}){canvas.zoom=scale;canvas.reload();for(int offset:{0,4,7}){QTest::mouseClick(&canvas,Qt::LeftButton,Qt::NoModifier,QPoint((10*8+offset)*scale,(11*8+offset)*scale));QCOMPARE(placed,QPoint(10,11));}QTest::mouseClick(&canvas,Qt::LeftButton,Qt::NoModifier,QPoint(11*8*scale,12*8*scale));QCOMPARE(placed,QPoint(11,12));}}
 void dragCommitsOnceAtTheCorrectTile(){cf::MapCanvas canvas;canvas.project=cf::Project::overworldExample();canvas.tool=5;canvas.zoom=3;canvas.reload();cf::MapSelection selected,moved;QPoint destination;int commits=0;canvas.onSelect=[&](cf::MapSelection s){selected=s;};canvas.onMove=[&](cf::MapSelection s,QPoint p){moved=s;destination=p;++commits;};auto pixel=[](int x,int y){return QPoint((x*8+4)*3,(y*8+4)*3);};QTest::mousePress(&canvas,Qt::LeftButton,Qt::NoModifier,pixel(9,10));QCOMPARE(selected.kind,QString("object"));QTest::mouseMove(&canvas,pixel(10,10));QCOMPARE(commits,0);QTest::mouseRelease(&canvas,Qt::LeftButton,Qt::NoModifier,pixel(10,10));QCOMPARE(commits,1);QCOMPARE(destination,QPoint(10,10));QCOMPARE(moved.id,selected.id);}
};
QTEST_MAIN(MapCanvasTests)
#include "map_canvas_tests.moc"
