#pragma once
#include "project.h"
#include "builder.h"
#include "game_session.h"
#include <QMainWindow>
#include <QFutureWatcher>
#include <functional>
class QLineEdit;
class QPlainTextEdit;
class QComboBox;
class QListWidget;
class QLabel;
class QVBoxLayout;
class QPushButton;
class QAction;
class QSpinBox;
class QTabWidget;
class QCheckBox;
namespace cf {
class Updater;class ScreenBuilder;class ScriptWorkspace;
class GraphView;
class WorldEditor;
class EmulatorWidget;
class ScriptEditor;
class AnimationEditor;
class DataEditor;
class SoundEditor;
class PixelEditor;
class Window : public QMainWindow {
public:
    Window();
    bool openProject(const QString &path);
    void startUpdater(const QString &job={});
protected:
    void closeEvent(QCloseEvent *) override;
private:
    ScreenBuilder *screens;ScriptWorkspace *scriptWorkspace;void projectSettings();
    Updater *updater;
    Project project;
    GameSession session;
    DataEditor *dataEditor;PixelEditor *pixels;QCheckBox *modal;QComboBox *portrait;QLineEdit *promptId;
    QString path, selected;
    QByteArray saved;
    QVector<Project> undo, redo;
    bool filling = false, building = false, runAfterBuild = false;
    EmulatorWidget *emulator;
    int previewScene=-1,previewCursor=-1;void previewChoose(int);
    int previewPrompt = 0, previewChoice = 0, previewMenu=-1, previewSide=0, previewItem=0;bool previewReadOnly=false;
    GraphView *graph;
    QListWidget *promptList;
    QLineEdit *projectTitle, *promptTitle;
    QComboBox *entry, *targetMode;
    QSpinBox *boxX, *boxY, *boxW, *boxH;
    WorldEditor *worldEditor;
    ScriptEditor *scripts;
    AnimationEditor *animations;SoundEditor *soundEditor;
    QTabWidget *workspace;
    QPlainTextEdit *body, *log, *onEnter;
    QLabel *validation, *screen, *previewLabel;
    QWidget *choicesWidget;
    QVBoxLayout *choicesLayout,*textSettingsLayout;
    QPushButton *buildButton, *removeButton;
    QAction *undoAction, *redoAction;
    QFutureWatcher<BuildResult> watcher;
    QByteArray bytes() const;
    bool dirty() const;
    bool mayDiscard();
    bool save(bool saveAs = false);
    void fresh();
    void mutate(const std::function<void()> &, bool refill = false);
    void refresh(bool refill = true);
    void fillInspector();
    void select(const QString &);
    void addPrompt();
    void deletePrompt();
    void exportRom();
    void buildAndPlay(const Project &);
    void resetPreview();
    void previewInput(int input);
    void updatePreview();
    void previewRequests();
};
}
