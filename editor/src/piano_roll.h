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
 std::function<void(const QVector<SoundNote> &)> onEdit;
 std::function<void(const QString &)> onStatus;
 std::function<void(int)> onSelect;
protected:
 void paintEvent(QPaintEvent *)override;
 void mousePressEvent(QMouseEvent *)override;
 void mouseMoveEvent(QMouseEvent *)override;
 void mouseReleaseEvent(QMouseEvent *)override;
 void keyPressEvent(QKeyEvent *)override;
private:
 SoundClip clip;int selected=-1,quantum=15,playhead=-1;
 bool percussion=false;bool dragging=false,resizing=false;QPoint press;int originalStart=0,originalEnd=0,originalPitch=60,ghostStart=0,ghostEnd=0,ghostPitch=60;
 int total()const;int startOf(int)const;int tickAt(int)const;int pitchAt(int)const;QRect noteRect(int start,int end,int pitch)const;
 void apply(int start,int end,int pitch,int remove=-1);void send(const QVector<SoundNote> &);
};
}
