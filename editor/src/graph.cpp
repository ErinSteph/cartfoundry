#include "graph.h"
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QWheelEvent>
#include <cmath>
namespace cf {
static const QColor orange("#f79b67"), ink("#19232f"), muted("#9baab9");
static QPainterPath curve(QPointF a, QPointF b) {
    QPainterPath p(a); qreal d = qMax(80., std::abs(b.x() - a.x()) * .5);
    p.cubicTo(a + QPointF(d, 0), b - QPointF(d, 0), b); return p;
}
class PromptItem : public QGraphicsItem {
public:
    Prompt data; bool entry; GraphView *graph;
    PromptItem(Prompt p, bool start, GraphView *v) : data(p), entry(start), graph(v) {
        setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
        setCursor(Qt::OpenHandCursor); setPos(p.position); setZValue(1);
    }
    QRectF boundingRect() const override { return {-9, -3, 278, qreal(111 + qMax(1, data.choices.size()) * 30)}; }
    QPointF input() const { return mapToScene(QPointF(0, 30)); }
    QPointF output(int c) const { return mapToScene(QPointF(260, 99 + 30 * c)); }
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *) override {
        p->setRenderHint(QPainter::Antialiasing);
        const QRectF box(0, 0, 260, 104 + qMax(1, data.choices.size()) * 30);
        p->setBrush(ink); p->setPen(QPen(isSelected() ? orange : QColor("#435162"), isSelected() ? 2 : 1));
        p->drawRoundedRect(box, 10, 10);
        QFont f = p->font(); f.setPointSize(9); f.setBold(true); p->setFont(f);
        p->setPen(entry ? orange : muted); p->drawText(16, 23, entry ? "STARTING PROMPT" : "TEXT PROMPT");
        f.setPointSize(13); p->setFont(f); p->setPen(QColor("#f5eee1"));
        p->drawText(16, 49, QFontMetrics(f).elidedText(data.title, Qt::ElideRight, 227));
        f.setPointSize(10); f.setBold(false); p->setFont(f); p->setPen(muted);
        p->drawText(16, 72, QFontMetrics(f).elidedText(QString(data.text).replace('\n', ' '), Qt::ElideRight, 227));
        p->setBrush(QColor("#87c9c0")); p->setPen(Qt::NoPen); p->drawEllipse(QPointF(0, 30), 6, 6);
        for (int i = 0; i < data.choices.size(); ++i) {
            const int y = 99 + i * 30;
            p->setPen(QColor("#344151")); p->drawLine(16, y - 17, 244, y - 17);
            p->setPen(QColor("#e1e6eb")); p->drawText(16, y + 4, QFontMetrics(f).elidedText(data.choices[i].text, Qt::ElideRight, 217));
            p->setBrush(data.choices[i].target.isEmpty() ? QColor("#53606f") : orange);
            p->setPen(QPen(orange, 1.5)); p->drawEllipse(QPointF(260, y), 6, 6);
        }
        if (data.choices.isEmpty()) { p->setPen(muted); p->drawText(16, 104, "End of conversation"); }
    }
    QVariant itemChange(GraphicsItemChange c, const QVariant &v) override {
        if (c == ItemPositionHasChanged && scene()) graph->updateWires();
        return QGraphicsItem::itemChange(c, v);
    }
};
GraphView::GraphView(QWidget *p) : QGraphicsView(p) {
    setScene(new QGraphicsScene(this)); setRenderHint(QPainter::Antialiasing);
    setDragMode(RubberBandDrag); setTransformationAnchor(AnchorUnderMouse);
    setViewportUpdateMode(FullViewportUpdate); setFrameShape(QFrame::NoFrame);
    setSceneRect(-1500, -1200, 6000, 4500);
    connect(scene(), &QGraphicsScene::selectionChanged, this, [this] {
        if (rebuilding) return;
        for (auto *item : scene()->selectedItems()) {
            if (auto *n = dynamic_cast<PromptItem *>(item)) { if (onSelect) onSelect(n->data.id); break; }
        }
    });
}
void GraphView::setProject(const Project &p, const QString &selected) {
    rebuilding = true; dragWire = nullptr; wireNode = wireChoice = -1;
    nodes.clear(); wires.clear(); scene()->clear(); project = p;
    for (const auto &n : project.prompts) { auto *item = new PromptItem(n, n.id == p.entry, this); scene()->addItem(item); nodes << item; item->setSelected(n.id == selected); }
    updateWires(); rebuilding = false;
}
void GraphView::selectPrompt(const QString &id) {
    rebuilding = true;
    for (auto *n : nodes) n->setSelected(n->data.id == id);
    rebuilding = false;
}
void GraphView::frameAll() {
    if (nodes.isEmpty()) return;
    QRectF bounds;
    for (auto *n : nodes) bounds |= n->sceneBoundingRect();
    fitInView(bounds.adjusted(-40, -40, 40, 40), Qt::KeepAspectRatio);
    if (transform().m11() > 1.0) { resetTransform(); centerOn(bounds.center()); }
}
void GraphView::updateWires() {
    for (auto *w : wires) { scene()->removeItem(w); delete w; } wires.clear();
    for (int i = 0; i < nodes.size(); ++i) for (int j = 0; j < nodes[i]->data.choices.size(); ++j) {
        int target = project.indexOf(nodes[i]->data.choices[j].target);
        if (target < 0 || target >= nodes.size()) continue;
        auto *w = scene()->addPath(curve(nodes[i]->output(j), nodes[target]->input()), QPen(QColor("#c68462"), 2.5));
        w->setZValue(-1); wires << w;
    }
}
void GraphView::drawBackground(QPainter *p, const QRectF &r) {
    p->fillRect(r, QColor("#111923")); p->setPen(QPen(QColor("#283341"), 1));
    int startX = int(std::floor(r.left() / 24)) * 24, startY = int(std::floor(r.top() / 24)) * 24;
    for (int x = startX; x < r.right(); x += 24) for (int y = startY; y < r.bottom(); y += 24) p->drawPoint(x, y);
}
void GraphView::mousePressEvent(QMouseEvent *e) {
    const auto pos = mapToScene(e->pos());
    if (e->button() == Qt::LeftButton) for (int i = 0; i < nodes.size(); ++i) for (int c = 0; c < nodes[i]->data.choices.size(); ++c) {
        if (QLineF(pos, nodes[i]->output(c)).length() < 13 / transform().m11()) {
            wireNode = i; wireChoice = c;
            dragWire = scene()->addPath(curve(nodes[i]->output(c), pos), QPen(orange, 3));
            dragWire->setZValue(2); e->accept(); return;
        }
    }
    QGraphicsView::mousePressEvent(e);
}
void GraphView::mouseMoveEvent(QMouseEvent *e) {
    if (dragWire) { dragWire->setPath(curve(nodes[wireNode]->output(wireChoice), mapToScene(e->pos()))); return; }
    QGraphicsView::mouseMoveEvent(e);
}
void GraphView::mouseReleaseEvent(QMouseEvent *e) {
    if (dragWire) {
        auto pos = mapToScene(e->pos()); QString target;
        for (auto *n : nodes) if (QLineF(pos, n->input()).length() < 20 / transform().m11()) { target = n->data.id; break; }
        const QString source = nodes[wireNode]->data.id; const int choice = wireChoice;
        scene()->removeItem(dragWire); delete dragWire; dragWire = nullptr; wireNode = wireChoice = -1;
        if (!target.isEmpty() && onConnect) onConnect(source, choice, target);
        return;
    }
    QGraphicsView::mouseReleaseEvent(e);
    QVector<QPointF> positions; bool moved = false;
    for (int i = 0; i < nodes.size(); ++i) { positions << nodes[i]->pos(); moved |= nodes[i]->pos() != project.prompts[i].position; }
    if (moved && onMove) onMove(positions);
}
void GraphView::wheelEvent(QWheelEvent *e) {
    if (e->modifiers() & (Qt::ControlModifier | Qt::MetaModifier)) {
        qreal factor = e->angleDelta().y() > 0 ? 1.15 : 1 / 1.15;
        if (transform().m11() * factor > .25 && transform().m11() * factor < 2.0) scale(factor, factor);
        e->accept();
    } else QGraphicsView::wheelEvent(e);
}
}
