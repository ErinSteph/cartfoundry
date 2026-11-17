#pragma once
#include "project.h"
#include <QWidget>
#include <functional>
class QListWidget;class QSpinBox;class QDoubleSpinBox;class QLineEdit;class QCheckBox;class QLabel;
namespace cf {
class SheetCanvas:public QWidget {
public:
    explicit SheetCanvas(QWidget *parent=nullptr);
    QImage sheetImage;RasterAsset asset;double zoom=0;int first=0,last=0,current=0;bool dmg=false;
    std::function<void(int,bool)> onFrame;std::function<void(double)> onZoom;
    void setZoom(double);void refreshSize();
protected:
    void paintEvent(QPaintEvent *)override;
    void mousePressEvent(QMouseEvent *)override;
    void wheelEvent(QWheelEvent *)override;
private:
    QRect sheetRect()const;
};
QWidget *sheetViewer(SheetCanvas *,QWidget *parent=nullptr);
class AnimationPreview:public QWidget {
public:
    explicit AnimationPreview(QWidget *parent=nullptr);
    QImage sheetImage;RasterAsset asset;bool dmg=false;int clip=0;CFAnimationState state{};
    void play();void stop();
    std::function<void(QString)> onStatus;
protected:
    void paintEvent(QPaintEvent *)override;
};
class AnimationEditor:public QWidget {
public:
    explicit AnimationEditor(QWidget *parent=nullptr);
    void setProject(const Project &);
    std::function<void(const Project &)> onEdit;
private:
    Project project;QString assetId;int selectedClip=0;bool filling=false;
    QListWidget *sprites,*clips;QSpinBox *columns,*rows,*first,*last;QDoubleSpinBox *duration;
    QLineEdit *name;QCheckBox *loop,*reverse,*flipH,*flipV;QLabel *info,*playStatus;SheetCanvas *sheet;AnimationPreview *preview;
    int assetIndex()const;void selectAsset();void fill();void mutate(const std::function<void(RasterAsset &)> &);
};
}
