#include "ScenePainter.h"

#include "model/Catalog.h"
#include "model/Geometry.h"
#include "model/SceneDocument.h"

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace sr {

PaintOptions::PaintOptions()
    : grid(true)
    , forPrint(false)
    , aerial(0)
{
}

namespace {

QColor lineColor(const QString &name)
{
    if (name == QStringLiteral("红色"))
        return QColor(176, 42, 42);
    if (name == QStringLiteral("白色"))
        return QColor(250, 250, 250);
    return QColor(32, 40, 48);
}

double devicePerMetre(const QPainter &painter)
{
    return qMax(0.01, qAbs(painter.transform().m11()));
}

double stroke(const QPainter &painter, double worldWidth)
{
    return qMax(worldWidth, 1.15 / devicePerMetre(painter));
}

void drawLabel(QPainter &painter, const QPointF &world, const QString &text, const QColor &color)
{
    if (text.isEmpty())
        return;
    const QTransform worldToDevice = painter.transform();
    const QPointF device = worldToDevice.map(world);
    const int px = qBound(10, int(devicePerMetre(painter) * 0.55), 40);
    painter.save();
    painter.resetTransform();
    QFont font(QStringLiteral("WenQuanYi Micro Hei"));
    font.setPixelSize(px);
    painter.setFont(font);
    painter.setPen(color);
    const QFontMetrics metrics(font);
    const int w = metrics.horizontalAdvance(text) + 8;
    const int h = metrics.height() + 2;
    painter.drawText(QRectF(device.x() - w / 2.0, device.y() - h / 2.0, w, h), Qt::AlignCenter, text);
    painter.restore();
}

void strokePath(QPainter &painter, const QVector<QPointF> &pts, const QPen &pen, bool closed)
{
    if (pts.size() < 2)
        return;
    QPainterPath path;
    path.moveTo(pts.first());
    for (int i = 1; i < pts.size(); ++i)
        path.lineTo(pts.at(i));
    if (closed)
        path.closeSubpath();
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
}

void drawRoadLine(QPainter &painter, const SceneObject &obj)
{
    if (obj.lineStyle == 0 || obj.points.size() < 2)
        return;
    const QVector<QPointF> pts = obj.points;
    QColor color(35, 44, 52);
    double width = 0.12;
    Qt::PenStyle pattern = Qt::SolidLine;
    switch (obj.lineStyle) {
    case 2: pattern = Qt::DashLine; break;
    case 4: pattern = Qt::DashLine; break;
    case 5: color = QColor(78, 130, 86); width = 0.7; break;
    case 6: color = QColor(90, 140, 90); width = 0.45; pattern = Qt::DotLine; break;
    case 7: color = QColor(60, 60, 60); break;
    case 8: color = QColor(48, 98, 140); pattern = Qt::DashDotLine; break;
    case 9: color = QColor(120, 140, 150); pattern = Qt::DotLine; break;
    case 10: color = QColor(120, 120, 120); width = 0.35; break;
    case 11: color = QColor(90, 90, 90); pattern = Qt::DashLine; break;
    case 12: color = QColor(70, 90, 110); width = 0.2; break;
    default: break;
    }
    if (obj.lineStyle == 7) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        const double step = 1.2;
        double walked = 0;
        for (int i = 1; i < pts.size(); ++i) {
            const double len = dist(pts.at(i - 1), pts.at(i));
            if (len < 1e-6)
                continue;
            const QPointF dir = (pts.at(i) - pts.at(i - 1)) / len;
            while (walked < len) {
                painter.drawEllipse(pts.at(i - 1) + dir * walked, 0.12, 0.12);
                walked += step;
            }
            walked -= len;
        }
        return;
    }
    QPen pen(color, stroke(painter, width), pattern, Qt::FlatCap, Qt::RoundJoin);
    if (obj.lineStyle == 3 || obj.lineStyle == 4) {
        strokePath(painter, offsetPolyline(pts, 0.1), pen, obj.closed);
        strokePath(painter, offsetPolyline(pts, -0.1), pen, obj.closed);
        return;
    }
    strokePath(painter, pts, pen, obj.closed);
}

void drawPrims(QPainter &painter, const SymbolStyle &style, const QColor &ink)
{
    for (int i = 0; i < style.prims.size(); ++i) {
        const Prim &prim = style.prims.at(i);
        painter.setPen(QPen(ink, stroke(painter, 0.035), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        if (prim.k == 0 && prim.pts.size() >= 2) {
            painter.drawLine(prim.pts.at(0), prim.pts.at(1));
        } else if (prim.k == 1 || prim.k == 3) {
            painter.drawRoundedRect(QRectF(prim.x, prim.y, prim.w, prim.h), prim.cr, prim.cr);
        } else if (prim.k == 2 && !prim.pts.isEmpty()) {
            painter.drawEllipse(prim.pts.first(), prim.r, prim.r);
        } else if (prim.k == 9 && !prim.pts.isEmpty()) {
            painter.setBrush(ink);
            painter.drawEllipse(prim.pts.first(), prim.r, prim.r);
        } else if (prim.k == 4 && prim.pts.size() >= 3) {
            strokePath(painter, sampleQuadratic(prim.pts.at(0), prim.pts.at(1), prim.pts.at(2)), painter.pen(), false);
        } else if (prim.k == 4 && prim.pts.size() == 2) {
            painter.drawLine(prim.pts.at(0), prim.pts.at(1));
        } else if (prim.k == 5 && !prim.pts.isEmpty()) {
            const QVector<QPointF> arc = sampleArc(prim.pts.first(), prim.r, prim.a0, prim.a1, prim.cw);
            if (prim.filled && arc.size() >= 3) {
                QPainterPath path;
                path.moveTo(prim.pts.first());
                for (int k = 0; k < arc.size(); ++k)
                    path.lineTo(arc.at(k));
                path.closeSubpath();
                painter.setBrush(ink);
                painter.drawPath(path);
            } else {
                strokePath(painter, arc, painter.pen(), false);
            }
        } else if ((prim.k == 7 || prim.k == 8) && prim.pts.size() >= 3) {
            QPainterPath path;
            path.moveTo(prim.pts.first());
            for (int k = 1; k < prim.pts.size(); ++k)
                path.lineTo(prim.pts.at(k));
            path.closeSubpath();
            if (prim.filled || prim.k == 8)
                painter.setBrush(QColor(ink.red(), ink.green(), ink.blue(), prim.k == 8 ? 30 : 180));
            painter.drawPath(path);
        }
    }
    if (style.prims.isEmpty() && style.skel.size() >= 2)
        strokePath(painter, style.skel, QPen(ink, stroke(painter, 0.04)), style.skel.size() > 2);
}

void drawSymbol(QPainter &painter, const SceneObject &obj, const Catalog &catalog)
{
    const SymbolDef *def = catalog.symbol(obj.symbolId);
    const QRectF natural = catalog.naturalBounds(obj.symbolId, obj.styleIndex);
    double sx = obj.scaleX;
    double sy = obj.scaleY;
    if (obj.lengthM > 0 && natural.width() > 1e-4)
        sx = obj.lengthM / natural.width();
    if (obj.widthM > 0 && natural.height() > 1e-4)
        sy = obj.widthM / natural.height();
    painter.save();
    painter.translate(obj.x, obj.y);
    painter.rotate(obj.rotation);
    painter.scale(sx, sy);
    if (def && obj.styleIndex >= 0 && obj.styleIndex < def->styles.size())
        drawPrims(painter, def->styles.at(obj.styleIndex), lineColor(def->line));
    else {
        painter.setPen(QPen(QColor(32, 40, 48), stroke(painter, 0.05)));
        painter.drawRect(QRectF(-1.8, -0.8, 3.6, 1.6));
    }
    painter.restore();
    if (!obj.label.isEmpty())
        drawLabel(painter, QPointF(obj.x, obj.y + natural.height() * sy * 0.5 + 0.6), obj.label, QColor(20, 90, 84));
}

void drawTrace(QPainter &painter, const SceneObject &obj)
{
    QColor color(20, 20, 20);
    double width = obj.lineWidth > 0 ? obj.lineWidth : 0.12;
    Qt::PenStyle pattern = Qt::SolidLine;
    switch (obj.subType) {
    case 2: width = qMax(width, 0.22); break;
    case 3: pattern = Qt::DashLine; break;
    case 4: color = QColor(150, 40, 40); pattern = Qt::DashLine; break;
    case 5: pattern = Qt::DotLine; break;
    case 6: color = QColor(50, 50, 50); pattern = Qt::DashDotLine; width = 0.08; break;
    default: break;
    }
    QPen pen(color, stroke(painter, width), pattern, Qt::RoundCap, Qt::RoundJoin);
    if (obj.subType == 2) {
        strokePath(painter, offsetPolyline(obj.points, 0.12), pen, false);
        strokePath(painter, offsetPolyline(obj.points, -0.12), pen, false);
    } else {
        strokePath(painter, obj.points, pen, false);
    }
    if (obj.showMeasure && obj.points.size() >= 2) {
        const double length = polylineLength(obj.points);
        const QPointF mid = obj.points.at(obj.points.size() / 2);
        drawLabel(painter, mid, QString::number(length, 'f', 2) + QStringLiteral(" m"), QColor(90, 30, 30));
    }
}

void drawDebris(QPainter &painter, const SceneObject &obj)
{
    if (obj.subType == 1) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(obj.isBlood ? QColor(160, 30, 30) : QColor(90, 90, 90));
        const double step = 0.45;
        double walked = 0;
        for (int i = 1; i < obj.points.size(); ++i) {
            const double len = dist(obj.points.at(i - 1), obj.points.at(i));
            if (len < 1e-6)
                continue;
            const QPointF dir = (obj.points.at(i) - obj.points.at(i - 1)) / len;
            while (walked < len) {
                painter.drawEllipse(obj.points.at(i - 1) + dir * walked, 0.08, 0.08);
                walked += step;
            }
            walked -= len;
        }
        return;
    }
    if (obj.points.size() < 3)
        return;
    QPainterPath path;
    path.moveTo(obj.points.first());
    for (int i = 1; i < obj.points.size(); ++i)
        path.lineTo(obj.points.at(i));
    path.closeSubpath();
    const QColor fill = obj.isBlood ? QColor(176, 48, 48, 90) : QColor(80, 80, 80, 70);
    painter.setPen(QPen(obj.isBlood ? QColor(140, 30, 30) : QColor(60, 60, 60), stroke(painter, 0.04)));
    painter.setBrush(fill);
    painter.drawPath(path);
    if (obj.showMeasure) {
        const double area = polygonArea(obj.points);
        drawLabel(painter, boundsOf(obj.points).center(), QString::number(area, 'f', 2) + QStringLiteral(" m²"), QColor(80, 40, 40));
    }
}

QString metresText(double value)
{
    return QString::number(value, 'f', value >= 20 ? 1 : 2) + QStringLiteral(" m");
}

void drawDimension(QPainter &painter, const SceneObject &obj)
{
    if (obj.points.size() < 2)
        return;
    const QPointF a = obj.points.at(0);
    const QPointF b = obj.points.at(1);
    const QColor color(17, 110, 100);
    QPen pen(color, stroke(painter, 0.03), Qt::SolidLine, Qt::RoundCap);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    QVector<QPointF> shown;
    if (obj.subType == 1 || obj.subType == 2) {
        const QPointF corner(b.x(), a.y());
        painter.drawLine(a, corner);
        painter.drawLine(corner, b);
        shown << a << corner << b;
    }
    if (obj.subType != 1) {
        painter.drawLine(a, b);
        shown << a << b;
    }
    const double tick = 0.25;
    const QPointF dir = b - a;
    const double len = qMax(0.001, QLineF(a, b).length());
    const QPointF n(-dir.y() / len, dir.x() / len);
    painter.drawLine(a - n * tick, a + n * tick);
    painter.drawLine(b - n * tick, b + n * tick);
    QVector<QPointF> measuredPath;
    measuredPath << a;
    if (obj.subType == 1)
        measuredPath << QPointF(b.x(), a.y());
    measuredPath << b;
    const double shownValue = obj.measured >= 0 ? obj.measured : polylineLength(measuredPath);
    QString text = metresText(shownValue);
    if (obj.subType == 3)
        text = QStringLiteral("皮尺 ") + text;
    if (obj.determined)
        text = text + QStringLiteral(" ●");
    drawLabel(painter, (a + b) * 0.5 + n * 0.45, text, color);
}

void drawCrosswalk(QPainter &painter, const SceneObject &obj)
{
    if (obj.points.size() < 2)
        return;
    const QPointF a = obj.points.at(0);
    const QPointF b = obj.points.at(1);
    const double len = qMax(0.001, dist(a, b));
    const QPointF dir = (b - a) / len;
    const QPointF n(-dir.y(), dir.x());
    const double span = obj.widthM > 0 ? obj.widthM : 3.0;
    const double stripe = 0.4;
    const double gap = obj.lineWidth > 0 ? obj.lineWidth : 0.4;
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(40, 40, 40));
    for (double t = 0; t < len; t += stripe + gap) {
        const QPointF p = a + dir * t;
        const QPointF p2 = a + dir * qMin(len, t + stripe);
        QPainterPath path;
        path.moveTo(p + n * span * 0.5);
        path.lineTo(p2 + n * span * 0.5);
        path.lineTo(p2 - n * span * 0.5);
        path.lineTo(p - n * span * 0.5);
        path.closeSubpath();
        painter.drawPath(path);
    }
}

void drawGuide(QPainter &painter, const SceneObject &obj)
{
    if (obj.points.size() < 2)
        return;
    const QPointF a = obj.points.at(0);
    const QPointF b = obj.points.at(1);
    const double len = qMax(0.001, dist(a, b));
    const QPointF dir = (b - a) / len;
    const QPointF n(-dir.y(), dir.x());
    painter.setPen(QPen(QColor(30, 30, 30), stroke(painter, 0.08), Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(a, b - dir * 0.5);
    QPainterPath head;
    head.moveTo(b);
    head.lineTo(b - dir * 0.7 + n * 0.28);
    head.lineTo(b - dir * 0.7 - n * 0.28);
    head.closeSubpath();
    painter.setBrush(QColor(30, 30, 30));
    painter.drawPath(head);
}

void drawParking(QPainter &painter, const SceneObject &obj)
{
    painter.save();
    painter.translate(obj.x, obj.y);
    painter.rotate(obj.rotation);
    const double length = obj.lengthM > 0 ? obj.lengthM : 10;
    const double width = obj.widthM > 0 ? obj.widthM : 5;
    const int stalls = qMax(1, obj.stallCount);
    painter.setPen(QPen(QColor(40, 40, 40), stroke(painter, 0.06)));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(QRectF(0, 0, length, width));
    const double slot = length / stalls;
    for (int i = 1; i < stalls; ++i)
        painter.drawLine(QPointF(i * slot, 0), QPointF(i * slot, width));
    painter.restore();
}

void drawCompass(QPainter &painter, const SceneObject &obj)
{
    painter.save();
    painter.translate(obj.x, obj.y);
    painter.rotate(obj.rotation);
    painter.setPen(QPen(QColor(20, 20, 20), stroke(painter, 0.04)));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QPointF(0, 0), 1.05, 1.05);
    QPainterPath arrow;
    arrow.moveTo(0, 1.35);
    arrow.lineTo(0.28, 0.15);
    arrow.lineTo(0, 0.35);
    arrow.lineTo(-0.28, 0.15);
    arrow.closeSubpath();
    painter.setBrush(QColor(176, 42, 42));
    painter.drawPath(arrow);
    painter.restore();
    drawLabel(painter, QPointF(obj.x, obj.y + 1.7), QStringLiteral("北"), QColor(140, 30, 30));
}

void drawText(QPainter &painter, const SceneObject &obj)
{
    drawLabel(painter, QPointF(obj.x, obj.y), obj.label, QColor(20, 30, 40));
}

void drawGrid(QPainter &painter, const SceneDocument &document)
{
    const QRectF view = painter.transform().inverted().mapRect(QRectF(painter.viewport()));
    if (!view.isValid())
        return;
    const double step = document.gridMetres();
    if (step <= 0)
        return;
    const QPen pen(QColor(180, 198, 196, 140), stroke(painter, 0.01));
    painter.setPen(pen);
    const double x0 = qFloor(view.left() / step) * step;
    const double x1 = view.right();
    const double y0 = qFloor(view.top() / step) * step;
    const double y1 = view.bottom();
    int guard = 0;
    for (double x = x0; x <= x1 && guard < 400; x += step, ++guard)
        painter.drawLine(QPointF(x, y0), QPointF(x, y1));
    guard = 0;
    for (double y = y0; y <= y1 && guard < 400; y += step, ++guard)
        painter.drawLine(QPointF(x0, y), QPointF(x1, y));
}

void drawAerial(QPainter &painter, const SceneDocument &document, const QImage &image)
{
    const AerialLayer aerial = document.aerial();
    if (image.isNull() || aerial.imageWidth <= 0)
        return;
    const AerialPose pose = aerial.pose();
    const QPointF origin = pose.pixelToWorld(QPointF(0, 0));
    const QPointF ex = pose.pixelToWorld(QPointF(1, 0)) - origin;
    const QPointF ey = pose.pixelToWorld(QPointF(0, 1)) - origin;
    const QTransform pixelToWorld(ex.x(), ex.y(), ey.x(), ey.y(), origin.x(), origin.y());
    painter.save();
    painter.setOpacity(qBound(0.05, aerial.opacity, 1.0));
    painter.setTransform(pixelToWorld, true);
    painter.drawImage(QRectF(0, 0, image.width(), image.height()), image);
    painter.restore();
}

void drawObject(QPainter &painter, const SceneObject &obj, const Catalog &catalog)
{
    if (!obj.visible)
        return;
    if (obj.type == QLatin1String("roadline"))
        drawRoadLine(painter, obj);
    else if (obj.type == QLatin1String("symbol"))
        drawSymbol(painter, obj, catalog);
    else if (obj.type == QLatin1String("trace"))
        drawTrace(painter, obj);
    else if (obj.type == QLatin1String("debris"))
        drawDebris(painter, obj);
    else if (obj.type == QLatin1String("dimension"))
        drawDimension(painter, obj);
    else if (obj.type == QLatin1String("crosswalk"))
        drawCrosswalk(painter, obj);
    else if (obj.type == QLatin1String("guide"))
        drawGuide(painter, obj);
    else if (obj.type == QLatin1String("parking"))
        drawParking(painter, obj);
    else if (obj.type == QLatin1String("text"))
        drawText(painter, obj);
    else if (obj.type == QLatin1String("compass"))
        drawCompass(painter, obj);
}

} // namespace

void paintScene(QPainter &painter, const SceneDocument &document, const Catalog &catalog, const PaintOptions &options)
{
    painter.save();
    if (options.aerial)
        drawAerial(painter, document, *options.aerial);
    if (options.grid && !options.forPrint)
        drawGrid(painter, document);
    const QVector<SceneObject> &objects = document.objects();
    for (int pass = 0; pass < 6; ++pass) {
        for (int i = 0; i < objects.size(); ++i) {
            const QString type = objects.at(i).type;
            const int bucket =
                type == QLatin1String("roadline") || type == QLatin1String("parking") || type == QLatin1String("crosswalk") ? 0 :
                type == QLatin1String("debris") ? 1 :
                type == QLatin1String("trace") || type == QLatin1String("guide") ? 2 :
                type == QLatin1String("symbol") ? 3 :
                type == QLatin1String("dimension") || type == QLatin1String("text") ? 4 : 5;
            if (bucket == pass)
                drawObject(painter, objects.at(i), catalog);
        }
    }
    if (!options.selectedId.isEmpty()) {
        const SceneObject *selected = document.object(options.selectedId);
        if (selected) {
            QRectF box = selected->worldBounds(&catalog).adjusted(-0.25, -0.25, 0.25, 0.25);
            QPen pen(QColor(17, 123, 112));
            pen.setCosmetic(true);
            pen.setWidthF(1.6);
            pen.setStyle(Qt::DashLine);
            painter.setPen(pen);
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(box);
        }
    }
    painter.restore();
}

} // namespace sr
