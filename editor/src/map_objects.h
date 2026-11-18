#pragma once
#include "map_authoring.h"
#include <QWidget>
#include <functional>
class QListWidget;class QLineEdit;class QLabel;class QFormLayout;class QTabWidget;
namespace cf {
class MapObjects:public QWidget {
public:
 explicit MapObjects(QWidget *parent=nullptr);
 void setProject(const Project &,int area,int tile);
 void setSelection(MapSelection);
 void chooseBrush(MapBrush);
 void duplicate();void remove();
 std::function<void(const Project&)> onEdit;
 std::function<void(MapSelection)> onSelection;
 std::function<void(MapBrush)> onBrush;
private:
 Project project;int area=0,tile=0;MapSelection selected;bool filling=false,editing=false;
 QTabWidget *tabs;QListWidget *palette,*instances;QLineEdit *search;QFormLayout *form;QLabel *message;
 void inspect();void edit(const std::function<void(Project&)> &,bool refill=false);void populate();
};
}
