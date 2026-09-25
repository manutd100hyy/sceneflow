#include "SceneObject.h"

#include "Catalog.h"
#include "Geometry.h"

#include <QJsonArray>

namespace sr {

static QJsonArray xy(const QPointF &p)
{
    QJsonArray a;
    a.append(p.x());
    a.append(p.y());
    return a;
}

QJsonArray pointsToJson(const QVector<QPointF> &pts)
{
    QJsonArray a;
    for (int i = 0; i < pts.size(); ++i)
        a.append(xy(pts.at(i)));
    return a;
}

QVector<QPointF> pointsFromJson(const QJsonArray &arr)
{
    QVector<QPointF> pts;
    for (int i = 0; i < arr.size(); ++i) {
        const QJsonArray xyv = arr.at(i).toArray();
        if (xyv.size() >= 2)
            pts.append(QPointF(xyv.at(0).toDouble(), xyv.at(1).toDouble()));
    }
    return pts;
}

SceneObject::SceneObject()
    : locked(false)
    , visible(true)
    , x(0)
    , y(0)
    , rotation(0)
    , scaleX(1)
    , scaleY(1)
    , styleIndex(0)
    , lengthM(0)
    , widthM(0)
    , lineStyle(1)
    , subType(0)
    , lineWidth(0.15)
    , closed(false)
    , isBlood(false)
    , showMeasure(true)
    , isDatum(false)
    , measured(-1)
    , determined(false)
    , fontSize(0.8)
    , stallCount(4)
{
}

QJsonObject SceneObject::toJson() const
{
    QJsonObject o;
    o.insert(QStringLiteral("id"), id);
    o.insert(QStringLiteral("type"), type);
    o.insert(QStringLiteral("name"), name);
    if (!groupId.isEmpty())
        o.insert(QStringLiteral("groupId"), groupId);
    o.insert(QStringLiteral("locked"), locked);
    o.insert(QStringLiteral("visible"), visible);
    o.insert(QStringLiteral("x"), x);
    o.insert(QStringLiteral("y"), y);
    o.insert(QStringLiteral("rotation"), rotation);
    o.insert(QStringLiteral("scaleX"), scaleX);
    o.insert(QStringLiteral("scaleY"), scaleY);
    if (!symbolId.isEmpty())
        o.insert(QStringLiteral("symbolId"), symbolId);
    o.insert(QStringLiteral("styleIndex"), styleIndex);
    if (!label.isEmpty())
        o.insert(QStringLiteral("label"), label);
    o.insert(QStringLiteral("lengthM"), lengthM);
    o.insert(QStringLiteral("widthM"), widthM);
    if (!points.isEmpty())
        o.insert(QStringLiteral("points"), pointsToJson(points));
    o.insert(QStringLiteral("lineStyle"), lineStyle);
    o.insert(QStringLiteral("subType"), subType);
    o.insert(QStringLiteral("lineWidth"), lineWidth);
    o.insert(QStringLiteral("closed"), closed);
    o.insert(QStringLiteral("isBlood"), isBlood);
    o.insert(QStringLiteral("showMeasure"), showMeasure);
    o.insert(QStringLiteral("isDatum"), isDatum);
    if (measured >= 0)
        o.insert(QStringLiteral("measured"), measured);
    o.insert(QStringLiteral("determined"), determined);
    if (!fromId.isEmpty())
        o.insert(QStringLiteral("fromId"), fromId);
    if (!toId.isEmpty())
        o.insert(QStringLiteral("toId"), toId);
    o.insert(QStringLiteral("fontSize"), fontSize);
    o.insert(QStringLiteral("stallCount"), stallCount);
    return o;
}

SceneObject SceneObject::fromJson(const QJsonObject &obj)
{
    SceneObject o;
    o.id = obj.value(QStringLiteral("id")).toString();
    o.type = obj.value(QStringLiteral("type")).toString();
    o.name = obj.value(QStringLiteral("name")).toString();
    o.groupId = obj.value(QStringLiteral("groupId")).toString();
    o.locked = obj.value(QStringLiteral("locked")).toBool(false);
    o.visible = obj.value(QStringLiteral("visible")).toBool(true);
    o.x = obj.value(QStringLiteral("x")).toDouble();
    o.y = obj.value(QStringLiteral("y")).toDouble();
    o.rotation = obj.value(QStringLiteral("rotation")).toDouble();
    o.scaleX = obj.value(QStringLiteral("scaleX")).toDouble(1);
    o.scaleY = obj.value(QStringLiteral("scaleY")).toDouble(1);
    o.symbolId = obj.value(QStringLiteral("symbolId")).toString();
    o.styleIndex = obj.value(QStringLiteral("styleIndex")).toInt();
    o.label = obj.value(QStringLiteral("label")).toString();
    o.lengthM = obj.value(QStringLiteral("lengthM")).toDouble();
    o.widthM = obj.value(QStringLiteral("widthM")).toDouble();
    o.points = pointsFromJson(obj.value(QStringLiteral("points")).toArray());
    o.lineStyle = obj.value(QStringLiteral("lineStyle")).toInt(1);
    o.subType = obj.value(QStringLiteral("subType")).toInt();
    o.lineWidth = obj.value(QStringLiteral("lineWidth")).toDouble(0.15);
    o.closed = obj.value(QStringLiteral("closed")).toBool();
    o.isBlood = obj.value(QStringLiteral("isBlood")).toBool();
    o.showMeasure = obj.value(QStringLiteral("showMeasure")).toBool(true);
    o.isDatum = obj.value(QStringLiteral("isDatum")).toBool();
    o.measured = obj.contains(QStringLiteral("measured")) ? obj.value(QStringLiteral("measured")).toDouble() : -1;
    o.determined = obj.value(QStringLiteral("determined")).toBool();
    o.fromId = obj.value(QStringLiteral("fromId")).toString();
    o.toId = obj.value(QStringLiteral("toId")).toString();
    o.fontSize = obj.value(QStringLiteral("fontSize")).toDouble(0.8);
    o.stallCount = obj.value(QStringLiteral("stallCount")).toInt(4);
    if (o.scaleX == 0)
        o.scaleX = 1;
    if (o.scaleY == 0)
        o.scaleY = 1;
    return o;
}

void SceneObject::translate(const QPointF &delta)
{
    x += delta.x();
    y += delta.y();
    for (int i = 0; i < points.size(); ++i)
        points[i] += delta;
}

void SceneObject::scaleAbout(const QPointF &origin, double factor)
{
    x = origin.x() + (x - origin.x()) * factor;
    y = origin.y() + (y - origin.y()) * factor;
    for (int i = 0; i < points.size(); ++i)
        points[i] = origin + (points.at(i) - origin) * factor;
    if (lengthM > 0)
        lengthM *= factor;
    if (widthM > 0)
        widthM *= factor;
    if (type == QLatin1String("symbol")) {
        scaleX *= factor;
        scaleY *= factor;
    }
    if (fontSize > 0)
        fontSize *= factor;
    lineWidth *= factor;
}

QRectF SceneObject::localSymbolBounds(const Catalog *catalog) const
{
    if (!catalog || symbolId.isEmpty())
        return QRectF(-1, -0.5, 2, 1);
    return catalog->naturalBounds(symbolId, styleIndex);
}

QRectF SceneObject::worldBounds(const Catalog *catalog) const
{
    if (type == QLatin1String("symbol")) {
        const QRectF local = localSymbolBounds(catalog);
        QVector<QPointF> corners;
        corners << local.topLeft() << local.topRight() << local.bottomLeft() << local.bottomRight();
        double sx = scaleX;
        double sy = scaleY;
        if (lengthM > 0 && local.width() > 1e-4)
            sx *= lengthM / local.width();
        if (widthM > 0 && local.height() > 1e-4)
            sy *= widthM / local.height();
        return boundsOf(mapPoints(corners, QPointF(x, y), rotation, sx, sy));
    }
    if (type == QLatin1String("text")) {
        const double w = qMax(1.0, label.size() * fontSize * 0.9);
        return QRectF(x, y - fontSize, w, fontSize * 1.4);
    }
    if (type == QLatin1String("compass"))
        return QRectF(x - 1.2, y - 1.2, 2.4, 2.4);
    if (type == QLatin1String("parking")) {
        QVector<QPointF> corners;
        corners << QPointF(0, 0) << QPointF(lengthM, 0) << QPointF(lengthM, widthM) << QPointF(0, widthM);
        return boundsOf(mapPoints(corners, QPointF(x, y), rotation, 1, 1));
    }
    QRectF r = boundsOf(points);
    if (type == QLatin1String("crosswalk") || type == QLatin1String("guide"))
        r = r.adjusted(-widthM, -widthM, widthM, widthM);
    if (!r.isValid())
        r = QRectF(x - 0.5, y - 0.5, 1, 1);
    return r;
}

QVector<QPointF> SceneObject::drivingPointsWorld(const Catalog *catalog) const
{
    QVector<QPointF> local;
    if (catalog && type == QLatin1String("symbol"))
        local = catalog->drivingPoints(symbolId, styleIndex);
    if (local.isEmpty())
        local.append(QPointF(0, 0));
    const QRectF nb = localSymbolBounds(catalog);
    double sx = scaleX;
    double sy = scaleY;
    if (lengthM > 0 && nb.width() > 1e-4)
        sx *= lengthM / nb.width();
    if (widthM > 0 && nb.height() > 1e-4)
        sy *= widthM / nb.height();
    return mapPoints(local, QPointF(x, y), rotation, sx, sy);
}

double SceneObject::measureLength() const
{
    if (measured >= 0)
        return measured;
    if (subType == 1 && points.size() >= 2) {
        const QPointF corner(points.at(1).x(), points.at(0).y());
        return dist(points.at(0), corner) + dist(corner, points.at(1));
    }
    return polylineLength(points);
}

QString SceneObject::typeLabel() const
{
    if (type == QLatin1String("roadline"))
        return QStringLiteral("分道线");
    if (type == QLatin1String("symbol"))
        return name.isEmpty() ? symbolId : name;
    if (type == QLatin1String("trace"))
        return QStringLiteral("痕迹");
    if (type == QLatin1String("debris"))
        return isBlood ? QStringLiteral("血迹") : QStringLiteral("散落物");
    if (type == QLatin1String("dimension"))
        return QStringLiteral("标注");
    if (type == QLatin1String("text"))
        return QStringLiteral("文字");
    if (type == QLatin1String("crosswalk"))
        return QStringLiteral("人行横道");
    if (type == QLatin1String("guide"))
        return QStringLiteral("导向箭头");
    if (type == QLatin1String("parking"))
        return QStringLiteral("停车位");
    if (type == QLatin1String("compass"))
        return QStringLiteral("指北针");
    return type;
}

} // namespace sr
