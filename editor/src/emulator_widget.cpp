#include "emulator_widget.h"
#include <QtWidgets>
namespace cf {
EmulatorView::EmulatorView(QWidget *parent):QWidget(parent){setFocusPolicy(Qt::StrongFocus);setMinimumSize(480,432);timer=new QTimer(this);timer->setTimerType(Qt::PreciseTimer);connect(timer,&QTimer::timeout,this,[this]{tick();});timer->start(4);clock.start();}
void EmulatorView::tick(){qint64 now=clock.nsecsElapsed();if(paused||!isVisible()||!core.loaded()){last=now;debt=0;return;}debt+=double(now-last)/1e9;last=now;int n=0;while(debt>=1./59.7275&&n<4){core.frame(keys);audio.write(core.takeAudio());debt-=1./59.7275;++n;}if(n==4)debt=0;if(n)update();}
void EmulatorView::reset(){core.reset();releaseKeys();debt=0;last=clock.nsecsElapsed();update();}
void EmulatorView::releaseKeys(){keys=0;}
void EmulatorView::pulse(int bit,bool down){if(down)keys|=1u<<bit;else keys&=~(1u<<bit);}
int EmulatorView::keyBit(int key)const{switch(key){case Qt::Key_Z:return 0;case Qt::Key_X:return 1;case Qt::Key_Backspace:return 2;case Qt::Key_Return:case Qt::Key_Enter:return 3;case Qt::Key_Right:return 4;case Qt::Key_Left:return 5;case Qt::Key_Up:return 6;case Qt::Key_Down:return 7;default:return -1;}}
void EmulatorView::keyPressEvent(QKeyEvent *e){int b=keyBit(e->key());if(b>=0){pulse(b,true);e->accept();}else QWidget::keyPressEvent(e);}
void EmulatorView::keyReleaseEvent(QKeyEvent *e){if(!e->isAutoRepeat()){int b=keyBit(e->key());if(b>=0)pulse(b,false);}e->accept();}
void EmulatorView::focusOutEvent(QFocusEvent *e){releaseKeys();QWidget::focusOutEvent(e);}
void EmulatorView::hideEvent(QHideEvent *e){releaseKeys();QWidget::hideEvent(e);}
void EmulatorView::paintEvent(QPaintEvent *){QPainter p(this);p.fillRect(rect(),QColor("#0b131b"));auto image=core.image();if(image.isNull()){p.setPen(Qt::white);p.drawText(rect(),Qt::AlignCenter,"Build & Play a project or test an area.\nYou can also load an existing GB, GBC or GBA ROM.");return;}int scale=qMax(1,qMin(width()/image.width(),height()/image.height()));QSize size=image.size()*scale;QRect target(QPoint((width()-size.width())/2,(height()-size.height())/2),size);p.drawImage(target,image);setAccessibleName(QString("mGBA emulated ROM, %1 × %2 pixels, %3").arg(image.width()).arg(image.height()).arg(paused?"paused":"running"));}
EmulatorWidget::EmulatorWidget(QWidget *parent):QWidget(parent){auto *root=new QVBoxLayout(this);auto *bar=new QHBoxLayout;auto *open=new QPushButton("Load ROM…");pauseButton=new QPushButton("Pause");auto *reset=new QPushButton("Reset");bar->addWidget(open);bar->addWidget(pauseButton);bar->addWidget(reset);bar->addStretch();bar->addWidget(new QLabel("mGBA 0.10.5 · GB / GBC / GBA · audio enabled"));root->addLayout(bar);view=new EmulatorView;root->addWidget(view,1);auto *controls=new QHBoxLayout;
    for(auto pair:{qMakePair(QString("←"),5),qMakePair(QString("↑"),6),qMakePair(QString("↓"),7),qMakePair(QString("→"),4),qMakePair(QString("A (Z)"),0),qMakePair(QString("B (X)"),1),qMakePair(QString("Select"),2),qMakePair(QString("Start"),3)}){auto *b=new QPushButton(pair.first);controls->addWidget(b);connect(b,&QPushButton::pressed,this,[this,pair]{view->pulse(pair.second,true);});connect(b,&QPushButton::released,this,[this,pair]{QTimer::singleShot(70,view,[this,pair]{view->pulse(pair.second,false);});});}
    root->addLayout(controls);status=new QLabel("Click the screen to focus. Arrows move · Z = A · X = B · Enter = Start · Backspace = Select.");status->setWordWrap(true);root->addWidget(status);
    connect(open,&QPushButton::clicked,this,[this]{auto p=QFileDialog::getOpenFileName(this,"Load ROM",{},"Game Boy / Color / Advance (*.gb *.gbc *.gba)");if(!p.isEmpty())load(p);});
    connect(pauseButton,&QPushButton::clicked,this,[this]{view->paused=!view->paused;view->releaseKeys();pauseButton->setText(view->paused?"Resume":"Pause");view->setFocus();});connect(reset,&QPushButton::clicked,view,[this]{view->reset();view->setFocus();});
}
bool EmulatorWidget::load(const QString &path){QString error;if(!view->core.load(path,error)){status->setText(error);return false;}view->paused=false;pauseButton->setText("Pause");view->setFocus();status->setText("Running "+QFileInfo(path).fileName()+" · actual ROM emulation · arrows, Z / X, Enter / Backspace");view->update();return true;}
void EmulatorWidget::pause(){view->paused=true;view->releaseKeys();pauseButton->setText("Resume");}
}
