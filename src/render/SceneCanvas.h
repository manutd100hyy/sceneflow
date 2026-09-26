#ifndef SR_SCANVAS_H
#define SR_SCANVAS_H

#include "model/SceneObject.h"

#include <QImage>
#include <QQuickPaintedItem>
#include <QPointF>
#include <QVector>

namespace sr {

class Catalog;
class SceneDocument;

class SceneCanvas : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(int tool READ tool WRITE setTool NOTIFY toolChanged)
    Q_PROPERTY(QString symbolName READ symbolName WRITE setSymbolName NOTIFY toolChanged)
    Q_PROPERTY(int traceType READ traceType WRITE setTraceType NOTIFY toolChanged)
    Q_PROPERTY(int debrisVariant READ debrisVariant WRITE setDebrisVariant NOTIFY toolChanged)
    Q_PROPERTY(int dimensionStyle READ dimensionStyle WRITE setDimensionStyle NOTIFY toolChanged)
    Q_PROPERTY(int laneCount READ laneCount WRITE setLaneCount NOTIFY toolChanged)
    Q_PROPERTY(double laneWidth READ laneWidth WRITE setLaneWidth NOTIFY toolChanged)
public:
    explicit SceneCanvas(QQuickItem *parent = 0);
    void setDocument(SceneDocument *document);
    void setCatalog(const Catalog *catalog);

    int tool() const;
    void setTool(int tool);
    QString symbolName() const;
    void setSymbolName(const QString &name);
    int traceType() const;
    void setTraceType(int type);
    int debrisVariant() const;
    void setDebrisVariant(int variant);
    int dimensionStyle() const;
    void setDimensionStyle(int style);
    int laneCount() const;
    void setLaneCount(int count);
    double laneWidth() const;
    void setLaneWidth(double metres);

    Q_INVOKABLE void finishDraft();
    Q_INVOKABLE void cancelDraft();

signals:
    void toolChanged();
    void needText(double x, double y);
    void calibrationReady();
    void hintChanged(const QString &text);

protected:
    void paint(QPainter *painter) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
    void touchEvent(QTouchEvent *event) Q_DECL_OVERRIDE;

private:
    enum Handle { HandleNone, HandleMove, HandleRotate, HandleScale };
    QPointF toWorld(const QPointF &screen) const;
    QPointF toScreen(const QPointF &world) const;
    double tolerance() const;
    Handle hitHandle(const QPointF &screen) const;
    void zoomAt(const QPointF &screen, double factor);
    void finishAt(const QVector<QPointF> &pts);
    QRectF selectionBox() const;

    SceneDocument *m_doc;
    const Catalog *m_catalog;
    int m_tool;
    QString m_symbol;
    int m_traceType;
    int m_debris;
    int m_dimStyle;
    int m_lanes;
    double m_laneWidth;
    bool m_drafting;
    bool m_panning;
    bool m_dragging;
    bool m_rotating;
    bool m_scaling;
    QVector<QPointF> m_draft;
    QPointF m_cursor;
    QPointF m_pressScreen;
    QPointF m_pressWorld;
    QPointF m_lastScreen;
    SceneObject m_editBefore;
    bool m_haveEditBefore;
    double m_pinchDist;
    QImage m_aerial;
    QString m_aerialPath;

    void reloadAerial();
};

} // namespace sr

#endif
