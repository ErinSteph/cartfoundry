#pragma once
#include "project.h"
#include "cf_world.h"
#include "cf_flow.h"
#include "game_session.h"
#include <QWidget>
#include <QSet>
#include <functional>
class QLineEdit;class QCheckBox;class QComboBox;class QSpinBox;class QLabel;class QListWidget;class QScrollArea;
namespace cf {
class SheetCanvas;
class MapCanvas : public QWidget {
public:
    explicit MapCanvas(QWidget *parent=nullptr);
    Project project;
    int tool=0,brush=0,zoom=3,brushMode=0;
    std::function<void(int)> onPick,onZoom;
    QString trigger, exitTarget;
    std::function<void(const World &)> onStroke;
    void reload();
protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;void wheelEvent(QWheelEvent *) override;
private:
    bool painting=false,changed=false,erasing=false;QPoint strokeStart,strokeEnd;
    void paintCell(QPoint, bool erase);
};
class WorldPlay : public QWidget {
public:
    explicit WorldPlay(QWidget *parent=nullptr);
    void setProject(const Project &);
    void reset(bool fresh=true,bool enter=true);
    void input(int key);
protected:
    void paintEvent(QPaintEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void keyReleaseEvent(QKeyEvent *) override;
    void focusOutEvent(QFocusEvent *) override;
private:
    CFAnimationController playerAnimation{};CFAnimationBindings playerBindings{};QVector<CFAnimation> playerClips;
    QVector<CFAnimationController> actorAnimations;QVector<CFAnimationBindings> actorBindings;QVector<QVector<CFAnimation>> actorClips;
    bool playerMoving=false;QImage playerSheet;QVector<QImage> actorSheets;
    QVector<CFActorState> actorStates;QVector<CFBehavior> actorBehaviors;CFBehavior playerBehavior{};int movementClock=0,touchCooldown=60;
    Project project;CFWorldState state{};CFState flow{};bool dialog=false;QSet<int> held;
    GameSession session;CFStepState stepState{};QVector<CFStepRule> stepRules;QVector<QVector<CFInstruction>> stepCode;QByteArray solids;QVector<int> health;int menu=-1,menuSide=0,menuChoice=0;bool menuReadOnly=false;
    void tick();void interact(bool combat=false);void requests();void menuInput(int);
};
class WorldEditor : public QWidget {
public:
    explicit WorldEditor(QWidget *parent=nullptr);
    void setProject(const Project &);
    std::function<void(const Project &)> onEdit, onTestArea;
    void selectArea(int index){activeArea=index;setProject(project);}
private:
    Project project;bool filling=false;
    MapCanvas *canvas;WorldPlay *play;SheetCanvas *tilesheet;
    QCheckBox *enabled;
    QComboBox *tileset,*player,*prompt,*areaSelector,*exitArea;
    QLineEdit *areaName,*chapter;
    int activeArea=0;
    World &activeWorld();
    Project viewingProject() const;
    QSpinBox *width,*height;
    QListWidget *tileList,*assetList;
    QLabel *status;
    void editWorld(const std::function<void(World &)> &);
    void doImport(const QString &kind,bool replace=false);
};
}
