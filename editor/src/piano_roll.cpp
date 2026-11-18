#include "piano_roll.h"
#include <QtWidgets>
#include <algorithm>
namespace cf {
namespace {constexpr int keys=64,header=26,row=18,scale=4,low=36,high=107;
QString drumName(int n){switch(n){case 36:return "Kick";case 38:return "Snare";case 42:return "CHat";case 45:return "Tom";case 46:return "OHat";case 49:return "Crash";}return "";}
QString pitchName(int n){static const QStringList names{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};return names[n%12]+QString::number(n/12-1);}
bool black(int n){return n%12==1||n%12==3||n%12==6||n%12==8||n%12==10;}}
PianoRoll::PianoRoll(QWidget *parent):QWidget(parent){setFocusPolicy(Qt::StrongFocus);setMouseTracking(true);setPattern(clip);setAccessibleName("Piano roll. Click to draw, drag to move, drag right edge to resize; right-click or Delete to erase.");}
int PianoRoll::total()const{int ticks=0;for(auto n:clip.notes)ticks+=n.ticks;return ticks;}
int PianoRoll::startOf(int index)const{int ticks=0;for(int i=0;i<index;++i)ticks+=clip.notes[i].ticks;return ticks;}
int PianoRoll::tickAt(int x)const{return qMax(0,qRound(double(x-keys)/scale/quantum)*quantum);}
int PianoRoll::pitchAt(int y)const{return qBound(low,high-(y-header)/row,high);}
QRect PianoRoll::noteRect(int start,int end,int pitch)const{return {keys+start*scale,header+(high-pitch)*row+1,qMax(3,(end-start)*scale-1),row-2};}
void PianoRoll::setPattern(const SoundClip &p){clip=p;quantum=qBound(1,qRound(1800.0/clip.bpm),255);if(selected>=clip.notes.size())selected=-1;setFixedSize(keys+qMax(quantum*32,total()+quantum*4)*scale,header+(high-low+1)*row);update();}
void PianoRoll::paintEvent(QPaintEvent *){
 QPainter p(this);p.fillRect(rect(),QColor("#15212d"));int right=width();auto visible=visibleRegion().boundingRect();int top=visible.isEmpty()?0:visible.top(),left=visible.isEmpty()?0:visible.left();
 for(int n=low;n<=high;++n){int y=header+(high-n)*row;p.fillRect(keys,y,right-keys,row,QColor(black(n)?"#14202c":"#1c2a38"));p.fillRect(0,y,keys,row,QColor(black(n)?"#263442":"#ccd2cc"));p.setPen(black(n)?QColor("#cbd5db"):QColor("#25343e"));p.drawText(QRect(2,y,keys-7,row),Qt::AlignRight|Qt::AlignVCenter,(percussion&&!drumName(n).isEmpty()?drumName(n):pitchName(n)));p.setPen(QColor("#344252"));p.drawLine(0,y,right,y);}
 for(int t=0;t*scale+keys<right;t+=quantum){int x=keys+t*scale;bool bar=t%(quantum*8)==0,beat=t%(quantum*2)==0;p.setPen(QColor(bar?"#8092a0":beat?"#526474":"#2e4050"));p.drawLine(x,header,x,height());}
 int tick=0;for(int i=0;i<clip.notes.size();++i){auto n=clip.notes[i];if(n.note&&!(dragging&&i==selected)){auto box=noteRect(tick,tick+n.ticks,n.note);p.setPen(i==selected?QColor("#ffe3b3"):QColor("#8eb7ad"));p.setBrush(i==selected?QColor("#c67e54"):QColor("#36766b"));p.drawRoundedRect(box,3,3);p.setPen(QColor("#f1f3e7"));p.drawText(box.adjusted(5,0,-3,0),Qt::AlignVCenter,(percussion&&!drumName(n.note).isEmpty()?drumName(n.note):pitchName(n.note)));p.drawLine(box.right()-4,box.top()+4,box.right()-4,box.bottom()-4);}tick+=n.ticks;}
 if(dragging){p.setPen(QColor("#ffe3b3"));p.setBrush(QColor(238,149,99,170));p.drawRoundedRect(noteRect(ghostStart,ghostEnd,ghostPitch),3,3);}
 if(playhead>=0){p.setPen(QPen(QColor("#ffd184"),2));int x=keys+playhead*scale;p.drawLine(x,header,x,height());}
 for(int n=low;n<=high;++n){int y=header+(high-n)*row;p.fillRect(left,y,keys,row,QColor(black(n)?"#263442":"#ccd2cc"));p.setPen(black(n)?QColor("#cbd5db"):QColor("#25343e"));p.drawText(QRect(left+2,y,keys-7,row),Qt::AlignRight|Qt::AlignVCenter,(percussion&&!drumName(n).isEmpty()?drumName(n):pitchName(n)));}
 p.fillRect(left,top,visible.width(),header,QColor("#243443"));p.setPen(QColor("#d2d9df"));for(int t=0;t*scale+keys<right;t+=quantum*2){int x=keys+t*scale;if(x<left+keys)continue;p.drawText(QRect(x+4,top,quantum*2*scale-4,header),Qt::AlignVCenter,QString::number(t/(quantum*8)+1)+"."+QString::number((t/(quantum*2))%4+1));}p.fillRect(left,top,keys,header,QColor("#243443"));p.setPen(Qt::white);p.drawText(QRect(left,top,keys,header),Qt::AlignCenter,"Pitch");
}
void PianoRoll::send(const QVector<SoundNote> &notes){if(notes.isEmpty()||notes.size()>64){if(onStatus)onStatus("This edit would exceed the 64-row pattern limit. Remove notes or shorten the gap.");return;}clip.notes=notes;setPattern(clip);if(onEdit)onEdit(notes);if(onSelect)onSelect(selected);}
void PianoRoll::apply(int start,int end,int pitch,int remove){
 struct Interval{int a,b;SoundNote n;};QVector<Interval> intervals;int t=0;
 for(int i=0;i<clip.notes.size();++i){auto n=clip.notes[i];int next=t+n.ticks;if(n.note&&i!=remove){if(next<=start||t>=end)intervals<<Interval{t,next,n};else{if(t<start)intervals<<Interval{t,start,n};if(next>end)intervals<<Interval{end,next,n};}}t=next;}
 SoundNote inserted{pitch,end-start};if(remove>=0&&remove<clip.notes.size()){inserted=clip.notes[remove];inserted.note=pitch;}intervals<<Interval{start,end,inserted};std::sort(intervals.begin(),intervals.end(),[](auto a,auto b){return a.a<b.a;});QVector<SoundNote> out;int cursor=0;auto append=[&](SoundNote n,int ticks){while(ticks>0){int length=qMin(ticks,255);n.ticks=length;out<<n;ticks-=length;}};
 for(auto i:intervals){if(i.a>cursor)append(SoundNote{0,1},i.a-cursor);append(i.n,i.b-i.a);cursor=i.b;}if(cursor<total())append(SoundNote{0,1},total()-cursor);selected=-1;int position=0;for(int i=0;i<out.size();++i){if(position==start&&out[i].note==pitch){selected=i;break;}position+=out[i].ticks;}send(out);
}
void PianoRoll::mousePressEvent(QMouseEvent *e){
 auto visible=visibleRegion().boundingRect();if(e->position().x()<visible.left()+keys||e->position().y()<visible.top()+header)return;setFocus();QPoint point=e->position().toPoint();selected=-1;int t=0;for(int i=0;i<clip.notes.size();++i){auto n=clip.notes[i];if(n.note&&noteRect(t,t+n.ticks,n.note).contains(point)){selected=i;break;}t+=n.ticks;}
 if(e->button()==Qt::RightButton){eraseSelected();return;}if(e->button()!=Qt::LeftButton)return;
 if(selected<0){int start=tickAt(point.x());apply(start,start+quantum,pitchAt(point.y()));return;}
 if(onSelect)onSelect(selected);originalStart=startOf(selected);originalEnd=originalStart+clip.notes[selected].ticks;originalPitch=clip.notes[selected].note;ghostStart=originalStart;ghostEnd=originalEnd;ghostPitch=originalPitch;press=point;resizing=point.x()>=noteRect(originalStart,originalEnd,originalPitch).right()-7;dragging=true;update();
}
void PianoRoll::mouseMoveEvent(QMouseEvent *e){auto point=e->position().toPoint();if(!dragging){setCursor(point.x()>=keys?Qt::CrossCursor:Qt::ArrowCursor);return;}if(resizing){ghostEnd=qBound(originalStart+1,tickAt(point.x()),originalStart+255);}else{ghostStart=qMax(0,originalStart+qRound(double(point.x()-press.x())/scale/quantum)*quantum);ghostEnd=ghostStart+originalEnd-originalStart;ghostPitch=qBound(low,originalPitch+qRound(double(press.y()-point.y())/row),high);}if(onStatus)onStatus(QString("%1 · %2 ticks").arg(pitchName(ghostPitch)).arg(ghostEnd-ghostStart));update();}
void PianoRoll::mouseReleaseEvent(QMouseEvent *){if(!dragging)return;dragging=false;int index=selected;apply(ghostStart,ghostEnd,ghostPitch,index);update();}
void PianoRoll::eraseSelected(){if(selected<0||selected>=clip.notes.size())return;auto out=clip.notes;out[selected].note=0;selected=-1;send(out);}
void PianoRoll::transposeSelected(int semitones){if(selected<0||selected>=clip.notes.size())return;auto out=clip.notes;out[selected].note=qBound(low,out[selected].note+semitones,high);send(out);}
void PianoRoll::keyPressEvent(QKeyEvent *e){if(e->key()==Qt::Key_Delete||e->key()==Qt::Key_Backspace)eraseSelected();else if(e->key()==Qt::Key_Up)transposeSelected(1);else if(e->key()==Qt::Key_Down)transposeSelected(-1);else QWidget::keyPressEvent(e);}
}
