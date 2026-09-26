#ifndef SR_GEOMETRY_H
#define SR_GEOMETRY_H

#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>

namespace sr {

double dist(const QPointF &a, const QPointF &b);
double polylineLength(const QVector<QPointF> &pts);
double polygonArea(const QVector<QPointF> &pts);
QRectF boundsOf(const QVector<QPointF> &pts);
QRectF united(const QRectF &a, const QRectF &b);

QVector<QPointF> offsetPolyline(const QVector<QPointF> &pts, double offset);
QVector<QPointF> sampleQuadratic(const QPointF &a, const QPointF &ctrl, const QPointF &b, int n = 12);
QVector<QPointF> sampleArc(const QPointF &center, double radius, double startPi, double endPi, bool clockwise);
QVector<QPointF> smoothPolyline(const QVector<QPointF> &pts, int samplesPerSeg = 6);

QPointF rotateAround(const QPointF &p, const QPointF &origin, double degrees);
QVector<QPointF> mapPoints(const QVector<QPointF> &local, const QPointF &origin, double degrees, double sx, double sy);

double medianOf(QVector<double> values);
QPointF snapToGrid(const QPointF &p, double grid);
bool nearestOnSegment(const QPointF &p, const QPointF &a, const QPointF &b, double tol, QPointF *closest);
QPointF projectOnPolyline(const QPointF &p, const QVector<QPointF> &pts, double *distance = 0);
bool pointInPolygon(const QPointF &p, const QVector<QPointF> &poly);

QString newUuid();

struct AerialPose {
    double mpp;
    double originX;
    double originY;
    double rotationDeg;
    AerialPose();
    QPointF pixelToWorld(const QPointF &pixel) const;
    QPointF worldToPixel(const QPointF &world) const;
};

QString locateDataDir();

} // namespace sr

#endif
