#pragma once
#include "project.h"
#include <QGraphicsView>
#include <functional>
class QGraphicsPathItem;
namespace cf {
class PromptItem;
class GraphView : public QGraphicsView {
public:
    explicit GraphView(QWidget *parent = nullptr);
    void setProject(const Project &, const QString &selected);
    void frameAll();
    void selectPrompt(const QString &);
    std::function<void(QString)> onSelect;
    std::function<void(QString, int, QString)> onConnect;
    std::function<void(QVector<QPointF>)> onMove;
    void updateWires();
protected:
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void drawBackground(QPainter *, const QRectF &) override;
private:
    Project project;
    QVector<PromptItem *> nodes;
    QVector<QGraphicsPathItem *> wires;
    QGraphicsPathItem *dragWire = nullptr;
    int wireNode = -1, wireChoice = -1;
    bool rebuilding = false;
};
}
