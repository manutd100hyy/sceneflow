#include "Geometry.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLineF>
#include <QUuid>
#include <QtMath>
#include <algorithm>

namespace sr {

double dist(const QPointF &a, const QPointF &b)
{
    return QLineF(a, b).length();
}

double polylineLength(const QVector<QPointF> &pts)
{
    double sum = 0;
    for (int i = 1; i < pts.size(); ++i)
        sum += dist(pts.at(i - 1), pts.at(i));
    return sum;
}

double polygonArea(const QVector<QPointF> &pts)
{
    if (pts.size() < 3)
        return 0;
    double sum = 0;
    for (int i = 0; i < pts.size(); ++i) {
        const QPointF &a = pts.at(i);
        const QPointF &b = pts.at((i + 1) % pts.size());
        sum += a.x() * b.y() - b.x() * a.y();
    }
    return qAbs(sum) * 0.5;
}

QRectF boundsOf(const QVector<QPointF> &pts)
{
    if (pts.isEmpty())
        return QRectF();
    QRectF r(pts.first(), QSizeF(0, 0));
    for (int i = 1; i < pts.size(); ++i)
        r |= QRectF(pts.at(i), QSizeF(0, 0));
    return r;
}

QRectF united(const QRectF &a, const QRectF &b)
{
    if (!a.isValid())
        return b;
    if (!b.isValid())
        return a;
    return a.united(b);
}

static QPointF unitNormal(const QPointF &a, const QPointF &b)
{
    const QPointF d = b - a;
    const double len = QLineF(a, b).length();
    if (len < 1e-9)
        return QPointF(0, 1);
    return QPointF(-d.y() / len, d.x() / len);
}

QVector<QPointF> offsetPolyline(const QVector<QPointF> &pts, double offset)
{
    QVector<QPointF> out;
    if (pts.size() < 2 || qAbs(offset) < 1e-12) {
        out = pts;
        return out;
    }
    out.reserve(pts.size());
    for (int i = 0; i < pts.size(); ++i) {
        QPointF n;
        if (i == 0) {
            n = unitNormal(pts.at(0), pts.at(1));
        } else if (i == pts.size() - 1) {
            n = unitNormal(pts.at(i - 1), pts.at(i));
        } else {
            const QPointF n1 = unitNormal(pts.at(i - 1), pts.at(i));
            const QPointF n2 = unitNormal(pts.at(i), pts.at(i + 1));
            QPointF m = n1 + n2;
            const double len = qSqrt(m.x() * m.x() + m.y() * m.y());
            if (len < 1e-6) {
                n = n1;
            } else {
                m /= len;
                const double denom = QPointF::dotProduct(m, n1);
                const double scale = qBound(0.25, qAbs(denom) < 1e-4 ? 1.0 : 1.0 / denom, 4.0);
                n = m * scale;
            }
        }
        out.append(pts.at(i) + n * offset);
    }
    return out;
}

QVector<QPointF> sampleQuadratic(const QPointF &a, const QPointF &ctrl, const QPointF &b, int n)
{
    QVector<QPointF> out;
    n = qMax(2, n);
    for (int i = 0; i <= n; ++i) {
        const double t = double(i) / n;
        const double u = 1.0 - t;
        out.append(u * u * a + 2 * u * t * ctrl + t * t * b);
    }
    return out;
}

QVector<QPointF> sampleArc(const QPointF &center, double radius, double startPi, double endPi, bool clockwise)
{
    double d0 = 180.0 * startPi;
    double d1 = 180.0 * endPi;
    if (clockwise)
        qSwap(d0, d1);
    if (d1 < d0)
        d1 += 360.0;
    const int steps = qBound(8, int(qAbs(d1 - d0) / 6.0), 180);
    QVector<QPointF> out;
    for (int i = 0; i <= steps; ++i) {
        const double deg = d0 + (d1 - d0) * (double(i) / steps);
        const double rad = qDegreesToRadians(deg);
        out.append(center + QPointF(qCos(rad), qSin(rad)) * radius);
    }
    return out;
}

QVector<QPointF> smoothPolyline(const QVector<QPointF> &pts, int samplesPerSeg)
{
    if (pts.size() < 3)
        return pts;
    QVector<QPointF> out;
    const int n = pts.size();
    for (int i = 0; i < n - 1; ++i) {
        const QPointF &p0 = pts.at(qMax(0, i - 1));
        const QPointF &p1 = pts.at(i);
        const QPointF &p2 = pts.at(i + 1);
        const QPointF &p3 = pts.at(qMin(n - 1, i + 2));
        for (int s = 0; s < samplesPerSeg; ++s) {
            const double t = double(s) / samplesPerSeg;
            const double t2 = t * t;
            const double t3 = t2 * t;
            const QPointF point = 0.5 * ((2 * p1) + (-p0 + p2) * t
                                          + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t2
                                          + (-p0 + 3 * p1 - 3 * p2 + p3) * t3);
            out.append(point);
        }
    }
    out.append(pts.last());
    return out;
}

QPointF rotateAround(const QPointF &p, const QPointF &origin, double degrees)
{
    const double rad = qDegreesToRadians(degrees);
    const double c = qCos(rad);
    const double s = qSin(rad);
    const QPointF d = p - origin;
    return origin + QPointF(c * d.x() - s * d.y(), s * d.x() + c * d.y());
}

QVector<QPointF> mapPoints(const QVector<QPointF> &local, const QPointF &origin, double degrees, double sx, double sy)
{
    QVector<QPointF> out;
    out.reserve(local.size());
    for (int i = 0; i < local.size(); ++i) {
        const QPointF scaled(local.at(i).x() * sx, local.at(i).y() * sy);
        out.append(rotateAround(scaled, QPointF(0, 0), degrees) + origin);
    }
    return out;
}

double medianOf(QVector<double> values)
{
    if (values.isEmpty())
        return 1;
    std::sort(values.begin(), values.end());
    const int n = values.size();
    if (n % 2)
        return values.at(n / 2);
    return 0.5 * (values.at(n / 2 - 1) + values.at(n / 2));
}

QPointF snapToGrid(const QPointF &p, double grid)
{
    if (grid <= 1e-9)
        return p;
    return QPointF(qRound(p.x() / grid) * grid, qRound(p.y() / grid) * grid);
}

bool nearestOnSegment(const QPointF &p, const QPointF &a, const QPointF &b, double tol, QPointF *closest)
{
    const QPointF ab = b - a;
    const double len2 = QPointF::dotProduct(ab, ab);
    double t = 0;
    if (len2 > 1e-12)
        t = qBound(0.0, QPointF::dotProduct(p - a, ab) / len2, 1.0);
    const QPointF c = a + ab * t;
    if (closest)
        *closest = c;
    return dist(p, c) <= tol;
}

QPointF projectOnPolyline(const QPointF &p, const QVector<QPointF> &pts, double *distance)
{
    QPointF best = pts.isEmpty() ? p : pts.first();
    double bestD = 1e100;
    for (int i = 1; i < pts.size(); ++i) {
        QPointF c;
        nearestOnSegment(p, pts.at(i - 1), pts.at(i), 1e100, &c);
        const double d = dist(p, c);
        if (d < bestD) {
            bestD = d;
            best = c;
        }
    }
    if (distance)
        *distance = pts.size() < 2 ? dist(p, best) : bestD;
    return best;
}

bool pointInPolygon(const QPointF &p, const QVector<QPointF> &poly)
{
    if (poly.size() < 3)
        return false;
    bool inside = false;
    for (int i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
        const QPointF &a = poly.at(i);
        const QPointF &b = poly.at(j);
        const bool intersect = ((a.y() > p.y()) != (b.y() > p.y()))
                && (p.x() < (b.x() - a.x()) * (p.y() - a.y()) / ((b.y() - a.y()) + 1e-12) + a.x());
        if (intersect)
            inside = !inside;
    }
    return inside;
}

QString newUuid()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

AerialPose::AerialPose()
    : mpp(0.05)
    , originX(0)
    , originY(0)
    , rotationDeg(0)
{
}

QPointF AerialPose::pixelToWorld(const QPointF &pixel) const
{
    const double dx = pixel.x() - originX;
    const double dy = pixel.y() - originY;
    const QPointF yup(dx, -dy);
    const double rad = qDegreesToRadians(rotationDeg);
    const double c = qCos(rad);
    const double s = qSin(rad);
    const QPointF rotated(c * yup.x() - s * yup.y(), s * yup.x() + c * yup.y());
    return rotated * mpp;
}

QPointF AerialPose::worldToPixel(const QPointF &world) const
{
    const double m = qAbs(mpp) < 1e-12 ? 1.0 : mpp;
    const QPointF v = world / m;
    const double rad = qDegreesToRadians(-rotationDeg);
    const double c = qCos(rad);
    const double s = qSin(rad);
    const QPointF yup(c * v.x() - s * v.y(), s * v.x() + c * v.y());
    return QPointF(originX + yup.x(), originY - yup.y());
}

QString locateDataDir()
{
    QStringList candidates;
    const QByteArray env = qgetenv("SKETCHROAD_DATA");
    if (!env.isEmpty())
        candidates << QString::fromLocal8Bit(env);
    if (QCoreApplication::instance()) {
        const QString bin = QCoreApplication::applicationDirPath();
        candidates << QDir(bin).filePath("data");
        candidates << QDir(bin).filePath("../data");
        candidates << QDir(bin).filePath("../../data");
    }
#ifdef SR_DATA_DIR
    candidates << QStringLiteral(SR_DATA_DIR);
#endif
    for (int i = 0; i < candidates.size(); ++i) {
        if (QFileInfo::exists(QDir(candidates.at(i)).filePath("symbols.json")))
            return QDir(candidates.at(i)).absolutePath();
    }
    return candidates.isEmpty() ? QString() : candidates.first();
}

} // namespace sr
