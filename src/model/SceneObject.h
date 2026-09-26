#ifndef SR_SCENEOBJECT_H
#define SR_SCENEOBJECT_H

#include <QJsonArray>
#include <QJsonObject>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>

namespace sr {

class Catalog;

struct SceneObject {
    QString id;
    QString type;
    QString name;
    QString groupId;
    bool locked;
    bool visible;
    double x;
    double y;
    double rotation;
    double scaleX;
    double scaleY;
    QString symbolId;
    int styleIndex;
    QString label;
    double lengthM;
    double widthM;
    QVector<QPointF> points;
    int lineStyle;
    int subType;
    double lineWidth;
    bool closed;
    bool isBlood;
    bool showMeasure;
    bool isDatum;
    double measured;
    bool determined;
    QString fromId;
    QString toId;
    double fontSize;
    int stallCount;

    SceneObject();
    QJsonObject toJson() const;
    static SceneObject fromJson(const QJsonObject &obj);
    void translate(const QPointF &delta);
    void scaleAbout(const QPointF &origin, double factor);
    QRectF localSymbolBounds(const Catalog *catalog) const;
    QRectF worldBounds(const Catalog *catalog) const;
    QVector<QPointF> drivingPointsWorld(const Catalog *catalog) const;
    double measureLength() const;
    QString typeLabel() const;
};

QJsonArray pointsToJson(const QVector<QPointF> &pts);
QVector<QPointF> pointsFromJson(const QJsonArray &arr);

} // namespace sr

#endif
