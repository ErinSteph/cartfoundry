#pragma once
#include "project.h"
#include <QWidget>
#include <functional>
class QComboBox;class QListWidget;class QPlainTextEdit;class QLabel;
namespace cf {
class ScriptWorkspace:public QWidget {
public:
    explicit ScriptWorkspace(QWidget *parent=nullptr);
    void setProject(const Project &);
    std::function<void(const Project &)> onEdit;
private:
    struct Binding {QString type,name,id,event;std::function<QString(const Project&)> read;std::function<void(Project&,QString)> write;};
    Project project;QVector<Binding> bindings;QComboBox *types,*events,*referenceTypes;QListWidget *elements,*references;QPlainTextEdit *source,*json;QLabel *status;bool filling=false,editing=false;
    void list();void select();void refs();void populateBindings();
};
}
