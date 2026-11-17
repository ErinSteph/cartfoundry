#pragma once
#include "project.h"
#include <QWidget>
#include <functional>
class QListWidget;class QLineEdit;class QSpinBox;class QPushButton;
namespace cf{
class PixelCanvas:public QWidget{
public:RasterAsset asset;int zoom=16,brush=1;bool grid=true;std::function<void(const QByteArray &)> onPaint;explicit PixelCanvas(QWidget *p=nullptr);void reload();QPoint cursor{0,0};void paintCursor();
protected:void paintEvent(QPaintEvent *)override;void mousePressEvent(QMouseEvent *)override;void mouseMoveEvent(QMouseEvent *)override;void mouseReleaseEvent(QMouseEvent *)override;void keyPressEvent(QKeyEvent *)override;
private:bool drawing=false,changed=false;void paint(QPoint,bool);
};
class PixelEditor:public QWidget{
public:explicit PixelEditor(QWidget *p=nullptr);void setProject(const Project &);std::function<void(const Project &)> onEdit;
private:Project project;QString selected;bool filling=false;QListWidget *list;PixelCanvas *canvas;QLineEdit *name;QPushButton *colors[4];QSpinBox *zoom;void fill();void edit(const std::function<void(RasterAsset &)> &);void create(QString);
};
}
