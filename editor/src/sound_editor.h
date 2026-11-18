#pragma once
#include "game_session.h"
#include <QWidget>
#include <QElapsedTimer>
#include <functional>
class QListWidget;class QLineEdit;class QSpinBox;class QComboBox;class QScrollArea;class QLabel;class QTimer;class QCheckBox;class QTabBar;class QTableWidget;
namespace cf {
class PianoRoll;
class SoundEditor:public QWidget {
public:
 explicit SoundEditor(QWidget *parent=nullptr);
 void setProject(const Project &);
 std::function<void(const Project &)> onEdit;
protected:
 void hideEvent(QHideEvent *)override;
private:
 Project project;GameSession audition;QListWidget *library;QLineEdit *id,*name;
 QSpinBox *tempo,*volume,*masterLeft,*masterRight,*envelopePace,*sweepPace,*sweepShift,*length,*noiseShift,*noiseDivider,*noteVolume,*noteNoise,*noteLength;
 QComboBox *duty,*pan,*waveLevel;QCheckBox *enabled,*solo,*dac,*envelopeUp,*sweepDown,*noiseNarrow,*drumKit;
 QTabBar *channels;QTableWidget *wave;QWidget *pulseBox,*sweepBox,*waveBox,*noiseBox,*envelopeBox;
 PianoRoll *notes;QScrollArea *rollScroll;QLabel *status;QWidget *detail;QTimer *clock;QElapsedTimer elapsed;
 double debt=0;qint64 previous=0;int selected=-1,channel=0,viewedChannel=-1;bool filling=false,notifying=false,looping=false;
 void notify();void inspect();void commit();void play(bool loop);void stop();void add(bool effect);void demo();void addNote();void noteInspector();void editNote();
 SoundTrack track()const;void setTrack(const SoundTrack &);void statusText();
};
}
