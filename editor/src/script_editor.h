#pragma once
#include "project.h"
#include <QWidget>
#include <functional>
class QComboBox;class QListWidget;class QLineEdit;class QSpinBox;class QPlainTextEdit;class QLabel;
namespace cf {
class ScriptEditor:public QWidget{
public:
    explicit ScriptEditor(QWidget *parent=nullptr);
    void setProject(const Project &);
    std::function<void(const Project &)> onEdit;
private:
    Project project;int areaIndex=0,actorIndex=-1;bool filling=false;
    QComboBox *areas,*sprite,*templates,*interact,*close,*restart,*inventory;
    QListWidget *actors;QLineEdit *name;QSpinBox *x,*y;QPlainTextEdit *script;QLabel *status;
    World &world();void fillActor();void edit(const std::function<void(Project &,World &)> &);
};
}
