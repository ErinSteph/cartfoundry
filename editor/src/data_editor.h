#pragma once
#include "project.h"
#include <QWidget>
#include <functional>
class QTreeWidget;class QTreeWidgetItem;class QFormLayout;class QLabel;
namespace cf {
class DataEditor:public QWidget{
public:
 explicit DataEditor(QWidget *parent=nullptr);
 void setProject(const Project &);
 std::function<void(const Project &)> onEdit;
 std::function<void(int)> onOpenArea;
private:
 Project project;QTreeWidget *tree;QWidget *properties;QFormLayout *form;QLabel *errors;QString selected;bool editing=false;
 void populate();void inspect();void edit(const std::function<void(Project &)> &);
 void add(const QString &);void remove();
};
}
