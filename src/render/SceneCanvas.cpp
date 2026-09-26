#include "SceneCanvas.h"

#include "ScenePainter.h"
#include "model/Catalog.h"
#include "model/Geometry.h"
#include "model/SceneDocument.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QTouchEvent>
#include <QWheelEvent>
#include <QtMath>

namespace sr {

SceneCanvas::SceneCanvas(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , m_doc(0)
    , m_catalog(0)
    , m_tool(0)
    , m_traceType(1)
    , m_debris(2)
    , m_dimStyle(0)
    , m_lanes(2)
    , m_laneWidth(3.5)
    , m_drafting(false)
    , m_panning(false)
    , m_dragging(false)
    , m_rotating(false)
    , m_scaling(false)
    , m_haveEditBefore(false)
    , m_pinchDist(0)
{
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    setAcceptTouchEvents(true);
    setFlag(ItemAcceptsInputMethod, true);
    setFlag(ItemIsFocusScope, true);
    setAntialiasing(true);
    setRenderTarget(QQuickPaintedItem::Image);
    setOpaquePainting(true);
}

void SceneCanvas::setDocument(SceneDocument *document)
{
    if (m_doc == document)
        return;
    if (m_doc)
        m_doc->disconnect(this);
    m_doc = document;
    if (m_doc) {
        connect(m_doc, &SceneDocument::changed, this, [this]() {
            reloadAerial();
            update();
        });
        connect(m_doc, &SceneDocument::viewChanged, this, [this]() { update(); });
    }
    reloadAerial();
    update();
}

void SceneCanvas::setCatalog(const Catalog *catalog)
{
    m_catalog = catalog;
    update();
}

int SceneCanvas::tool() const { return m_tool; }
void SceneCanvas::setTool(int tool)
{
    if (m_tool == tool)
        return;
    cancelDraft();
    m_tool = tool;
    emit toolChanged();
    QString hint = QStringLiteral("选择对象，拖动移动");
    if (tool == 1) hint = QStringLiteral("拖动画布平移，滚轮或双指缩放");
    else if (tool == 2) hint = QStringLiteral("单击添加道路中线，双击或回车结束");
    else if (tool == 3) hint = QStringLiteral("单击放置图符");
    else if (tool == 4) hint = QStringLiteral("按住拖动绘制痕迹");
    else if (tool == 5) hint = QStringLiteral("按住拖动圈出散落物");
    else if (tool == 6) hint = QStringLiteral("单击两个点完成标注，可在属性中填写实测米数");
    else if (tool == 7) hint = QStringLiteral("单击放置文字");
    else if (tool == 8) hint = QStringLiteral("单击两个点确定人行横道");
    else if (tool == 9) hint = QStringLiteral("单击两个点确定导向箭头");
    else if (tool == 10) hint = QStringLiteral("单击圆心，再单击确定半径");
    else if (tool == 11) hint = QStringLiteral("单击删除对象");
    else if (tool == 12) hint = QStringLiteral("在航拍图上点两个已知距离的点");
    emit hintChanged(hint);
}
QString SceneCanvas::symbolName() const { return m_symbol; }
void SceneCanvas::setSymbolName(const QString &name) { m_symbol = name; emit toolChanged(); }
int SceneCanvas::traceType() const { return m_traceType; }
void SceneCanvas::setTraceType(int type) { m_traceType = type; }
int SceneCanvas::debrisVariant() const { return m_debris; }
void SceneCanvas::setDebrisVariant(int variant) { m_debris = variant; }
int SceneCanvas::dimensionStyle() const { return m_dimStyle; }
void SceneCanvas::setDimensionStyle(int style) { m_dimStyle = style; }
int SceneCanvas::laneCount() const { return m_lanes; }
void SceneCanvas::setLaneCount(int count) { m_lanes = qBound(1, count, 8); }
double SceneCanvas::laneWidth() const { return m_laneWidth; }
void SceneCanvas::setLaneWidth(double metres) { m_laneWidth = qBound(2.0, metres, 6.0); }

QPointF SceneCanvas::toWorld(const QPointF &screen) const
{
    if (!m_doc)
        return QPointF();
    const double z = qMax(0.05, m_doc->zoom());
    return QPointF((screen.x() - m_doc->panX()) / z, (m_doc->panY() - screen.y()) / z);
}

QPointF SceneCanvas::toScreen(const QPointF &world) const
{
    if (!m_doc)
        return world;
    const double z = m_doc->zoom();
    return QPointF(m_doc->panX() + world.x() * z, m_doc->panY() - world.y() * z);
}

double SceneCanvas::tolerance() const
{
    const double z = m_doc ? qMax(0.2, m_doc->zoom()) : 8;
    return 10.0 / z;
}

QRectF SceneCanvas::selectionBox() const
{
    if (!m_doc || !m_catalog || m_doc->selectionId().isEmpty())
        return QRectF();
    const SceneObject *obj = m_doc->object(m_doc->selectionId());
    if (!obj)
        return QRectF();
    return obj->worldBounds(m_catalog);
}

SceneCanvas::Handle SceneCanvas::hitHandle(const QPointF &screen) const
{
    const QRectF box = selectionBox();
    if (!box.isValid())
        return HandleNone;
    const QPointF rotate = toScreen(QPointF(box.center().x(), box.top() + box.height() + 0.8));
    if (QLineF(screen, rotate).length() < 14)
        return HandleRotate;
    const QPointF scale = toScreen(box.bottomRight());
    if (QLineF(screen, scale).length() < 14)
        return HandleScale;
    return HandleNone;
}

void SceneCanvas::zoomAt(const QPointF &screen, double factor)
{
    if (!m_doc)
        return;
    const QPointF world = toWorld(screen);
    const double next = qBound(0.2, m_doc->zoom() * factor, 400.0);
    m_doc->setZoom(next);
    m_doc->setPanX(screen.x() - world.x() * next);
    m_doc->setPanY(screen.y() + world.y() * next);
}

void SceneCanvas::reloadAerial()
{
    if (!m_doc) {
        m_aerial = QImage();
        m_aerialPath.clear();
        return;
    }
    const QString file = m_doc->aerial().file;
    if (file == m_aerialPath)
        return;
    m_aerialPath = file;
    m_aerial = QImage();
    if (!file.isEmpty())
        m_aerial.load(file);
    update();
}

void SceneCanvas::cancelDraft()
{
    m_drafting = false;
    m_draft.clear();
    m_dragging = false;
    m_panning = false;
    m_rotating = false;
    m_scaling = false;
    if (m_doc)
        m_doc->cancelDrag();
    update();
}

void SceneCanvas::finishAt(const QVector<QPointF> &pts)
{
    if (!m_doc)
        return;
    if (m_tool == 2)
        m_doc->addRoad(pts, m_lanes, m_laneWidth);
    else if (m_tool == 10 && pts.size() >= 2)
        m_doc->addCircleRoad(pts.at(0), dist(pts.at(0), pts.at(1)), m_lanes, m_laneWidth);
    else if (m_tool == 4)
        m_doc->addTrace(pts, m_traceType);
    else if (m_tool == 5)
        m_doc->addDebris(pts, m_debris);
    else if (m_tool == 6 && pts.size() >= 2) {
        const QString fromId = m_doc->hitTest(pts.at(0), tolerance() * 1.4, m_catalog);
        const QString toId = m_doc->hitTest(pts.at(1), tolerance() * 1.4, m_catalog);
        m_doc->addDimension(pts.at(0), pts.at(1), m_dimStyle, fromId, toId);
    } else if (m_tool == 8 && pts.size() >= 2)
        m_doc->addCrosswalk(pts.at(0), pts.at(1), m_lanes * m_laneWidth);
    else if (m_tool == 9 && pts.size() >= 2)
        m_doc->addGuide(pts.at(0), pts.at(1));
}

void SceneCanvas::finishDraft()
{
    if (m_draft.size() >= 2)
        finishAt(m_draft);
    m_drafting = false;
    m_draft.clear();
    update();
}

void SceneCanvas::paint(QPainter *painter)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->fillRect(QRectF(0, 0, width(), height()), QColor(243, 246, 245));
    if (!m_doc || !m_catalog)
        return;
    painter->save();
    painter->translate(m_doc->panX(), m_doc->panY());
    painter->scale(m_doc->zoom(), -m_doc->zoom());
    PaintOptions options;
    options.grid = true;
    options.selectedId = m_doc->selectionId();
    options.aerial = m_aerial.isNull() ? 0 : &m_aerial;
    paintScene(*painter, *m_doc, *m_catalog, options);
    if (m_draft.size() >= 1) {
        painter->setPen(QPen(QColor(17, 123, 112), 1.4 / qMax(0.2, m_doc->zoom()), Qt::DashLine, Qt::RoundCap));
        for (int i = 1; i < m_draft.size(); ++i)
            painter->drawLine(m_draft.at(i - 1), m_draft.at(i));
        if (m_drafting)
            painter->drawLine(m_draft.last(), m_cursor);
        painter->setBrush(QColor(17, 123, 112));
        for (int i = 0; i < m_draft.size(); ++i)
            painter->drawEllipse(m_draft.at(i), 4.0 / m_doc->zoom(), 4.0 / m_doc->zoom());
    }
    painter->restore();

    const QRectF box = selectionBox();
    if (box.isValid()) {
        const QPointF rotate = toScreen(QPointF(box.center().x(), box.bottom()));
        const QPointF handle = toScreen(QPointF(box.center().x(), box.bottom() + 1.2));
        painter->setPen(QPen(QColor(17, 123, 112), 1));
        painter->drawLine(rotate, handle);
        painter->setBrush(Qt::white);
        painter->drawEllipse(handle, 7, 7);
        painter->drawRect(QRectF(toScreen(box.bottomRight()) - QPointF(5, 5), QSizeF(10, 10)));
    }
}

void SceneCanvas::mousePressEvent(QMouseEvent *event)
{
    forceActiveFocus();
    if (!m_doc || !m_catalog)
        return;
    m_pressScreen = event->localPos();
    m_lastScreen = m_pressScreen;
    const QPointF world = toWorld(m_pressScreen);
    const QPointF snapped = m_doc->snapPoint(world, m_catalog);
    m_pressWorld = world;
    m_cursor = snapped;
    if (event->button() == Qt::MiddleButton || m_tool == 1) {
        m_panning = true;
        event->accept();
        return;
    }
    if (m_tool == 0 || m_tool == 11) {
        const Handle handle = m_tool == 0 ? hitHandle(m_pressScreen) : HandleNone;
        if (handle == HandleRotate || handle == HandleScale) {
            const SceneObject *obj = m_doc->object(m_doc->selectionId());
            if (obj && !obj->locked) {
                m_editBefore = *obj;
                m_haveEditBefore = true;
                m_rotating = handle == HandleRotate;
                m_scaling = handle == HandleScale;
            }
            event->accept();
            return;
        }
        const QString hit = m_doc->hitTest(world, tolerance(), m_catalog);
        if (m_tool == 11) {
            if (!hit.isEmpty())
                m_doc->removeIds(QStringList() << hit);
            event->accept();
            return;
        }
        m_doc->setSelection(hit);
        if (!hit.isEmpty()) {
            m_dragging = true;
            m_doc->beginDrag(QStringList() << hit);
        }
        event->accept();
        return;
    }
    if (m_tool == 3) {
        if (!m_symbol.isEmpty())
            m_doc->placeSymbol(*m_catalog, m_symbol, snapped);
        event->accept();
        return;
    }
    if (m_tool == 7) {
        emit needText(snapped.x(), snapped.y());
        event->accept();
        return;
    }
    if (m_tool == 12) {
        const AerialPose pose = m_doc->aerial().pose();
        const QPointF pixel = pose.worldToPixel(world);
        AerialLayer aerial = m_doc->aerial();
        if (!m_drafting) {
            aerial.refAx = pixel.x();
            aerial.refAy = pixel.y();
            aerial.hasRefs = false;
            m_drafting = true;
            m_draft = QVector<QPointF>() << world;
        } else {
            aerial.refBx = pixel.x();
            aerial.refBy = pixel.y();
            aerial.hasRefs = true;
            m_drafting = false;
            m_draft.clear();
            m_doc->setAerial(aerial);
            emit calibrationReady();
            event->accept();
            return;
        }
        m_doc->setAerial(aerial);
        event->accept();
        return;
    }
    if (!m_drafting) {
        m_draft.clear();
        m_drafting = true;
    }
    m_draft.append(snapped);
    if ((m_tool == 6 || m_tool == 8 || m_tool == 9 || m_tool == 10) && m_draft.size() >= 2)
        finishDraft();
    else
        update();
    event->accept();
}

void SceneCanvas::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_doc)
        return;
    const QPointF screen = event->localPos();
    const QPointF world = toWorld(screen);
    m_cursor = m_doc->snapPoint(world, m_catalog);
    if (m_panning) {
        const QPointF d = screen - m_lastScreen;
        m_doc->setPanX(m_doc->panX() + d.x());
        m_doc->setPanY(m_doc->panY() + d.y());
        m_lastScreen = screen;
        return;
    }
    if (m_dragging) {
        m_doc->dragTo(world - m_pressWorld);
        return;
    }
    if ((m_rotating || m_scaling) && m_haveEditBefore) {
        SceneObject edited = m_editBefore;
        if (m_rotating) {
            const double ang = qRadiansToDegrees(qAtan2(world.y() - edited.y, world.x() - edited.x));
            edited.rotation = ang - 90.0;
        } else {
            const double before = qMax(0.2, dist(m_pressWorld, QPointF(m_editBefore.x, m_editBefore.y)));
            const double now = qMax(0.2, dist(world, QPointF(m_editBefore.x, m_editBefore.y)));
            const double factor = now / before;
            edited.scaleX = m_editBefore.scaleX * factor;
            edited.scaleY = m_editBefore.scaleY * factor;
            edited.lengthM = 0;
            edited.widthM = 0;
        }
        m_doc->replaceObjectRaw(edited);
        update();
        return;
    }
    if (m_drafting && (m_tool == 4 || m_tool == 5)) {
        if (m_draft.isEmpty() || dist(m_draft.last(), m_cursor) > 0.3)
            m_draft.append(m_cursor);
        update();
        return;
    }
    if (m_drafting)
        update();
}

void SceneCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if (m_panning)
        m_panning = false;
    if (m_dragging) {
        m_doc->endDrag();
        m_dragging = false;
    }
    if ((m_rotating || m_scaling) && m_haveEditBefore) {
        m_doc->commitReplace(m_editBefore);
        m_rotating = false;
        m_scaling = false;
        m_haveEditBefore = false;
    }
    if (m_drafting && (m_tool == 4 || m_tool == 5))
        finishDraft();
}

void SceneCanvas::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if (m_tool == 2 && m_drafting) {
        if (!m_draft.isEmpty())
            m_draft.removeLast();
        finishDraft();
    }
}

void SceneCanvas::wheelEvent(QWheelEvent *event)
{
    const int delta = event->angleDelta().y();
    if (delta == 0)
        return;
    zoomAt(event->pos(), delta > 0 ? 1.12 : 1.0 / 1.12);
    event->accept();
}

void SceneCanvas::keyPressEvent(QKeyEvent *event)
{
    if (!m_doc)
        return;
    if (event->key() == Qt::Key_Escape)
        cancelDraft();
    else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
        finishDraft();
    else if (event->key() == Qt::Key_Backspace && m_drafting && !m_draft.isEmpty()) {
        m_draft.removeLast();
        update();
    } else if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
        m_doc->removeSelection();
    else if (event->matches(QKeySequence::Undo))
        m_doc->undo();
    else if (event->matches(QKeySequence::Redo))
        m_doc->redo();
    else
        QQuickPaintedItem::keyPressEvent(event);
}

void SceneCanvas::touchEvent(QTouchEvent *event)
{
    const QList<QTouchEvent::TouchPoint> points = event->touchPoints();
    if (points.size() >= 2) {
        const QPointF a = points.at(0).pos();
        const QPointF b = points.at(1).pos();
        const QPointF center = (a + b) * 0.5;
        const double distance = qMax(1.0, QLineF(a, b).length());
        if (m_pinchDist < 1)
            m_pinchDist = distance;
        zoomAt(center, distance / m_pinchDist);
        m_pinchDist = distance;
        if (points.at(0).state() == Qt::TouchPointReleased || points.at(1).state() == Qt::TouchPointReleased)
            m_pinchDist = 0;
        event->accept();
        return;
    }
    m_pinchDist = 0;
    if (points.isEmpty())
        return;
    const QTouchEvent::TouchPoint point = points.first();
    QEvent::Type type = QEvent::MouseMove;
    if (point.state() == Qt::TouchPointPressed)
        type = QEvent::MouseButtonPress;
    else if (point.state() == Qt::TouchPointReleased)
        type = QEvent::MouseButtonRelease;
    QMouseEvent mouse(type, point.pos(), Qt::LeftButton,
                      type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton,
                      Qt::NoModifier);
    if (type == QEvent::MouseButtonPress)
        mousePressEvent(&mouse);
    else if (type == QEvent::MouseButtonRelease)
        mouseReleaseEvent(&mouse);
    else
        mouseMoveEvent(&mouse);
    event->accept();
}

} // namespace sr
