#pragma once
#include "project.h"
#include <QWidget>
#include <functional>
namespace cf {
class PianoRoll:public QWidget {
 Q_OBJECT
public:
 explicit PianoRoll(QWidget *parent=nullptr);
 void setPattern(const SoundClip &);
 void setPercussion(bool enabled){percussion=enabled;update();}
 void setPlayhead(int tick){playhead=tick;update();}
 int selectedNote()const{return selected;}
 void eraseSelected();void transposeSelected(int semitones);
 // Horizontal zoom as a percentage of the default (100 = 4 pixels per tick). Keeps the view centred where it was.
 void setZoomPercent(int percent);int zoomPercent()const{return zoom;}
 static QList<int> zoomLevels(){return {25,50,75,100,150,200,300,400};}
 std::function<void(int)> onZoom;
 std::function<void(const QVector<SoundNote> &)> onEdit;
 std::function<void(const QString &)> onStatus;
 std::function<void(int)> onSelect;
protected:
 void paintEvent(QPaintEvent *)override;
 void mousePressEvent(QMouseEvent *)override;
 void mouseMoveEvent(QMouseEvent *)override;
 void mouseReleaseEvent(QMouseEvent *)override;
 void keyPressEvent(QKeyEvent *)override;
 void wheelEvent(QWheelEvent *)override;
private:
 SoundClip clip;int selected=-1,quantum=15,playhead=-1,scale=4,zoom=100;
 bool percussion=false;bool dragging=false,resizing=false;QPoint press;int originalStart=0,originalEnd=0,originalPitch=60,ghostStart=0,ghostEnd=0,ghostPitch=60;
 int total()const;int startOf(int)const;int tickAt(int,bool fine=false)const;int pitchAt(int)const;QRect noteRect(int start,int end,int pitch)const;
 void apply(int start,int end,int pitch,int remove=-1);void send(const QVector<SoundNote> &);
};
}
