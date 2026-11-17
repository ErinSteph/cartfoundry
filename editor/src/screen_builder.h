#pragma once
#include "project.h"
#include <QWidget>
#include <functional>
class QComboBox;class QListWidget;class QFormLayout;class QLabel;
namespace cf {
class ScreenCanvas:public QWidget {
public:
    Project project;int scene=0,selected=-1,zoom=3;bool grid=true;
    explicit ScreenCanvas(QWidget *parent=nullptr);
    std::function<void(int)> onSelect;std::function<void(QRect)> onRect;
protected:
    void paintEvent(QPaintEvent*)override;void mousePressEvent(QMouseEvent*)override;void mouseMoveEvent(QMouseEvent*)override;void mouseReleaseEvent(QMouseEvent*)override;void wheelEvent(QWheelEvent*)override;
private:QPoint anchor;QRect original;bool dragging=false,resizing=false;
};
class ScreenBuilder:public QWidget {
public:
    explicit ScreenBuilder(QWidget *parent=nullptr);
    void setProject(const Project &);void selectScene(int);
    std::function<void(const Project&)> onEdit,onTest;
private:
    Project project;int active=0,selected=-1;bool filling=false,editing=false;
    QComboBox *scenes;QListWidget *elements;QFormLayout *form;ScreenCanvas *canvas;QLabel *info;
    void list();void inspect();void edit(const std::function<void(Project&)> &,bool refill=false);void add(QString type);
};
}
