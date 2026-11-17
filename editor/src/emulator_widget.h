#pragma once
#include "emulator_core.h"
#include <QWidget>
#include <QElapsedTimer>
class QLabel;class QPushButton;class QTimer;
namespace cf {
class EmulatorView : public QWidget {
public:
    explicit EmulatorView(QWidget *parent=nullptr);
    EmulatorCore core;quint32 keys=0;bool paused=true;
    void reset();void releaseKeys();void pulse(int bit,bool down);
protected:
    void paintEvent(QPaintEvent *)override;
    void keyPressEvent(QKeyEvent *)override;
    void keyReleaseEvent(QKeyEvent *)override;
    void focusOutEvent(QFocusEvent *)override;
    void hideEvent(QHideEvent *)override;
private:
    QTimer *timer;QElapsedTimer clock;qint64 last=0;double debt=0;
    void tick();int keyBit(int key)const;
};
class EmulatorWidget : public QWidget {
public:
    explicit EmulatorWidget(QWidget *parent=nullptr);
    bool load(const QString &);
    void pause();
private:
    EmulatorView *view;QLabel *status;QPushButton *pauseButton;
};
}
