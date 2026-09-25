#include "SceneDocument.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QLineF>
#include <QtMath>
#include <algorithm>

namespace sr {

namespace {

class AddCommand : public Command {
public:
    explicit AddCommand(const SceneObject &obj) : m_obj(obj) {}
    void undo(SceneDocument *doc) { doc->removeObjectRaw(m_obj.id); }
    void redo(SceneDocument *doc) { doc->insertObjectRaw(m_obj); }
private:
    SceneObject m_obj;
};

class RemoveCommand : public Command {
public:
    explicit RemoveCommand(const QVector<SceneObject> &objs) : m_objs(objs) {}
    void undo(SceneDocument *doc)
    {
        for (int i = 0; i < m_objs.size(); ++i)
            doc->insertObjectRaw(m_objs.at(i));
    }
    void redo(SceneDocument *doc)
    {
        for (int i = 0; i < m_objs.size(); ++i)
            doc->removeObjectRaw(m_objs.at(i).id);
    }
private:
    QVector<SceneObject> m_objs;
};

class ChangeCommand : public Command {
public:
    ChangeCommand(const SceneObject &before, const SceneObject &after)
        : m_before(before), m_after(after) {}
    void undo(SceneDocument *doc) { doc->replaceObjectRaw(m_before); }
    void redo(SceneDocument *doc) { doc->replaceObjectRaw(m_after); }
private:
    SceneObject m_before;
    SceneObject m_after;
};

class BatchCommand : public Command {
public:
    explicit BatchCommand(const QVector<Command *> &cmds) : m_cmds(cmds) {}
    ~BatchCommand() { qDeleteAll(m_cmds); }
    void undo(SceneDocument *doc)
    {
        for (int i = m_cmds.size() - 1; i >= 0; --i)
            m_cmds.at(i)->undo(doc);
    }
    void redo(SceneDocument *doc)
    {
        for (int i = 0; i < m_cmds.size(); ++i)
            m_cmds.at(i)->redo(doc);
    }
private:
    QVector<Command *> m_cmds;
};

class SnapshotCommand : public Command {
public:
    SnapshotCommand(const QVector<SceneObject> &before, const AerialLayer &aerialBefore,
                    const QVector<SceneObject> &after, const AerialLayer &aerialAfter)
        : m_before(before), m_aerialBefore(aerialBefore), m_after(after), m_aerialAfter(aerialAfter) {}
    void undo(SceneDocument *doc) { doc->restoreRaw(m_before, m_aerialBefore); }
    void redo(SceneDocument *doc) { doc->restoreRaw(m_after, m_aerialAfter); }
private:
    QVector<SceneObject> m_before;
    AerialLayer m_aerialBefore;
    QVector<SceneObject> m_after;
    AerialLayer m_aerialAfter;
};

int rankOf(const QString &type)
{
    if (type == QLatin1String("text")) return 100;
    if (type == QLatin1String("dimension")) return 90;
    if (type == QLatin1String("symbol")) return 80;
    if (type == QLatin1String("trace")) return 70;
    if (type == QLatin1String("debris")) return 60;
    if (type == QLatin1String("guide")) return 55;
    if (type == QLatin1String("crosswalk")) return 50;
    if (type == QLatin1String("parking")) return 45;
    if (type == QLatin1String("compass")) return 40;
    if (type == QLatin1String("roadline")) return 20;
    return 10;
}

double geometricLength(const SceneObject &o)
{
    if (o.points.size() < 2)
        return 0;
    if (o.subType == 1) {
        const QPointF corner(o.points.at(1).x(), o.points.at(0).y());
        return dist(o.points.at(0), corner) + dist(corner, o.points.at(1));
    }
    return polylineLength(o.points);
}

bool nearlySame(const SceneObject &a, const SceneObject &b)
{
    return a.toJson() == b.toJson();
}

QString traceName(int type)
{
    switch (type) {
    case 0: return QStringLiteral("轮胎滚印");
    case 1: return QStringLiteral("轮胎拖印");
    case 2: return QStringLiteral("双胎拖印");
    case 3: return QStringLiteral("轮胎压印");
    case 4: return QStringLiteral("轮胎侧滑印");
    case 5: return QStringLiteral("挫伤印");
    case 6: return QStringLiteral("自行车压印");
    default: return QStringLiteral("痕迹");
    }
}

} // namespace

AerialLayer::AerialLayer()
    : opacity(0.9)
    , mpp(0.05)
    , originX(0)
    , originY(0)
    , rotation(0)
    , calibrated(false)
    , hasRefs(false)
    , refAx(0), refAy(0), refBx(0), refBy(0)
    , knownMetres(0)
    , imageWidth(0)
    , imageHeight(0)
{
}

AerialPose AerialLayer::pose() const
{
    AerialPose pose;
    pose.mpp = mpp > 1e-9 ? mpp : 0.05;
    pose.originX = originX;
    pose.originY = originY;
    pose.rotationDeg = rotation;
    return pose;
}

QJsonObject AerialLayer::toJson() const
{
    QJsonObject o;
    o.insert(QStringLiteral("file"), file);
    o.insert(QStringLiteral("opacity"), opacity);
    o.insert(QStringLiteral("metresPerPixel"), mpp);
    o.insert(QStringLiteral("originX"), originX);
    o.insert(QStringLiteral("originY"), originY);
    o.insert(QStringLiteral("rotation"), rotation);
    o.insert(QStringLiteral("calibrated"), calibrated);
    o.insert(QStringLiteral("hasRefs"), hasRefs);
    o.insert(QStringLiteral("refAx"), refAx);
    o.insert(QStringLiteral("refAy"), refAy);
    o.insert(QStringLiteral("refBx"), refBx);
    o.insert(QStringLiteral("refBy"), refBy);
    o.insert(QStringLiteral("knownMetres"), knownMetres);
    o.insert(QStringLiteral("imageWidth"), imageWidth);
    o.insert(QStringLiteral("imageHeight"), imageHeight);
    return o;
}

void AerialLayer::fromJson(const QJsonObject &obj)
{
    *this = AerialLayer();
    file = obj.value(QStringLiteral("file")).toString();
    opacity = obj.value(QStringLiteral("opacity")).toDouble(0.9);
    mpp = obj.value(QStringLiteral("metresPerPixel")).toDouble(0.05);
    originX = obj.value(QStringLiteral("originX")).toDouble();
    originY = obj.value(QStringLiteral("originY")).toDouble();
    rotation = obj.value(QStringLiteral("rotation")).toDouble();
    calibrated = obj.value(QStringLiteral("calibrated")).toBool();
    hasRefs = obj.value(QStringLiteral("hasRefs")).toBool();
    refAx = obj.value(QStringLiteral("refAx")).toDouble();
    refAy = obj.value(QStringLiteral("refAy")).toDouble();
    refBx = obj.value(QStringLiteral("refBx")).toDouble();
    refBy = obj.value(QStringLiteral("refBy")).toDouble();
    knownMetres = obj.value(QStringLiteral("knownMetres")).toDouble();
    imageWidth = obj.value(QStringLiteral("imageWidth")).toInt();
    imageHeight = obj.value(QStringLiteral("imageHeight")).toInt();
}

SceneDocument::SceneDocument(QObject *parent)
    : QObject(parent)
    , m_grid(5)
    , m_snap(true)
    , m_zoom(8)
    , m_panX(400)
    , m_panY(300)
    , m_dragGroups(true)
    , m_dragActive(false)
{
}

SceneDocument::~SceneDocument()
{
    qDeleteAll(m_undo);
    qDeleteAll(m_redo);
}

const QVector<SceneObject> &SceneDocument::objects() const { return m_objects; }
int SceneDocument::objectCount() const { return m_objects.size(); }
bool SceneDocument::canUndo() const { return !m_undo.isEmpty(); }
bool SceneDocument::canRedo() const { return !m_redo.isEmpty(); }
QString SceneDocument::selectionId() const { return m_selection; }
double SceneDocument::gridMetres() const { return m_grid; }
bool SceneDocument::snapEnabled() const { return m_snap; }
double SceneDocument::zoom() const { return m_zoom; }
double SceneDocument::panX() const { return m_panX; }
double SceneDocument::panY() const { return m_panY; }
bool SceneDocument::dragGroups() const { return m_dragGroups; }
bool SceneDocument::dragActive() const { return m_dragActive; }
AerialLayer SceneDocument::aerial() const { return m_aerial; }

void SceneDocument::setGridMetres(double metres)
{
    m_grid = qMax(0.1, metres);
    emit viewChanged();
}

void SceneDocument::setSnapEnabled(bool on)
{
    m_snap = on;
    emit viewChanged();
}

void SceneDocument::setZoom(double z)
{
    m_zoom = qBound(0.2, z, 400.0);
    emit viewChanged();
}

void SceneDocument::setPanX(double v)
{
    m_panX = v;
    emit viewChanged();
}

void SceneDocument::setPanY(double v)
{
    m_panY = v;
    emit viewChanged();
}

void SceneDocument::setDragGroups(bool on) { m_dragGroups = on; }

void SceneDocument::setAerial(const AerialLayer &layer)
{
    m_aerial = layer;
    emit changed();
}

void SceneDocument::rebuildIndex()
{
    m_index.clear();
    for (int i = 0; i < m_objects.size(); ++i)
        m_index.insert(m_objects.at(i).id, i);
}

SceneObject *SceneDocument::object(const QString &id)
{
    const QHash<QString, int>::const_iterator it = m_index.constFind(id);
    if (it == m_index.constEnd())
        return 0;
    return &m_objects[it.value()];
}

const SceneObject *SceneDocument::object(const QString &id) const
{
    const QHash<QString, int>::const_iterator it = m_index.constFind(id);
    if (it == m_index.constEnd())
        return 0;
    return &m_objects.at(it.value());
}

void SceneDocument::commitReplace(const SceneObject &before)
{
    const SceneObject *now = object(before.id);
    if (!now || nearlySame(before, *now))
        return;
    pushCommand(new ChangeCommand(before, *now), true);
    emit selectionChanged();
}

void SceneDocument::insertObjectRaw(const SceneObject &obj)
{
    if (obj.id.isEmpty() || m_index.contains(obj.id))
        return;
    m_index.insert(obj.id, m_objects.size());
    m_objects.append(obj);
}

void SceneDocument::removeObjectRaw(const QString &id)
{
    const QHash<QString, int>::const_iterator it = m_index.constFind(id);
    if (it == m_index.constEnd())
        return;
    m_objects.removeAt(it.value());
    rebuildIndex();
    if (m_selection == id)
        m_selection.clear();
}

void SceneDocument::replaceObjectRaw(const SceneObject &obj)
{
    const QHash<QString, int>::const_iterator it = m_index.constFind(obj.id);
    if (it == m_index.constEnd())
        return;
    m_objects[it.value()] = obj;
}

void SceneDocument::restoreRaw(const QVector<SceneObject> &objects, const AerialLayer &aerial)
{
    m_objects = objects;
    m_aerial = aerial;
    rebuildIndex();
    if (!m_selection.isEmpty() && !m_index.contains(m_selection))
        m_selection.clear();
    emit selectionChanged();
    emit changed();
}

void SceneDocument::pushCommand(Command *command, bool alreadyApplied)
{
    if (!alreadyApplied)
        command->redo(this);
    m_undo.append(command);
    qDeleteAll(m_redo);
    m_redo.clear();
    while (m_undo.size() > 50) {
        delete m_undo.first();
        m_undo.removeFirst();
    }
    emit historyChanged();
    emit changed();
}

void SceneDocument::undo()
{
    if (m_undo.isEmpty())
        return;
    Command *command = m_undo.takeLast();
    command->undo(this);
    m_redo.append(command);
    emit historyChanged();
    emit changed();
    emit selectionChanged();
}

void SceneDocument::redo()
{
    if (m_redo.isEmpty())
        return;
    Command *command = m_redo.takeLast();
    command->redo(this);
    m_undo.append(command);
    emit historyChanged();
    emit changed();
    emit selectionChanged();
}

void SceneDocument::clearHistory()
{
    qDeleteAll(m_undo);
    qDeleteAll(m_redo);
    m_undo.clear();
    m_redo.clear();
    emit historyChanged();
}

void SceneDocument::setSelection(const QString &id)
{
    if (m_selection == id)
        return;
    m_selection = id;
    emit selectionChanged();
    emit changed();
}

QString SceneDocument::addObject(const SceneObject &obj)
{
    SceneObject copy = obj;
    if (copy.id.isEmpty())
        copy.id = newUuid();
    AddCommand *command = new AddCommand(copy);
    pushCommand(command, false);
    m_selection = copy.id;
    emit selectionChanged();
    return copy.id;
}

void SceneDocument::addObjects(const QVector<SceneObject> &objs, const QString &label)
{
    Q_UNUSED(label);
    if (objs.isEmpty())
        return;
    QVector<Command *> parts;
    QString lastId;
    for (int i = 0; i < objs.size(); ++i) {
        SceneObject copy = objs.at(i);
        if (copy.id.isEmpty())
            copy.id = newUuid();
        parts.append(new AddCommand(copy));
        lastId = copy.id;
    }
    pushCommand(new BatchCommand(parts), false);
    m_selection = lastId;
    emit selectionChanged();
}

void SceneDocument::removeIds(const QStringList &ids)
{
    QVector<SceneObject> doomed;
    for (int i = 0; i < ids.size(); ++i) {
        const SceneObject *obj = object(ids.at(i));
        if (obj && !obj->locked)
            doomed.append(*obj);
    }
    if (doomed.isEmpty())
        return;
    QVector<Command *> parts;
    parts.append(new RemoveCommand(doomed));
    pushCommand(new BatchCommand(parts), false);
    emit selectionChanged();
}

QString SceneDocument::duplicateSelection()
{
    const SceneObject *obj = object(m_selection);
    if (!obj)
        return QString();
    SceneObject copy = *obj;
    copy.id = newUuid();
    copy.translate(QPointF(1.2, 1.2));
    copy.groupId.clear();
    if (!copy.label.isEmpty())
        copy.label = autoLabel(1);
    const QString id = addObject(copy);
    return id;
}

void SceneDocument::removeSelection()
{
    if (m_selection.isEmpty())
        return;
    removeIds(QStringList() << m_selection);
}

void SceneDocument::beginDrag(const QStringList &seedIds)
{
    cancelDrag();
    QStringList ids;
    for (int i = 0; i < seedIds.size(); ++i) {
        const SceneObject *obj = object(seedIds.at(i));
        if (!obj || obj->locked)
            continue;
        if (!ids.contains(obj->id))
            ids.append(obj->id);
        if (m_dragGroups && !obj->groupId.isEmpty()) {
            for (int k = 0; k < m_objects.size(); ++k) {
                if (m_objects.at(k).groupId == obj->groupId && !m_objects.at(k).locked && !ids.contains(m_objects.at(k).id))
                    ids.append(m_objects.at(k).id);
            }
        }
    }
    for (int i = 0; i < m_objects.size(); ++i) {
        const SceneObject &obj = m_objects.at(i);
        if (obj.type != QLatin1String("dimension"))
            continue;
        if ((ids.contains(obj.fromId) || ids.contains(obj.toId)) && !ids.contains(obj.id))
            ids.append(obj.id);
    }
    if (ids.isEmpty())
        return;
    m_dragIds = ids;
    m_dragBefore.clear();
    for (int i = 0; i < ids.size(); ++i) {
        const SceneObject *obj = object(ids.at(i));
        if (obj)
            m_dragBefore.append(*obj);
    }
    m_dragTotal = QPointF();
    m_dragActive = true;
}

void SceneDocument::dragTo(const QPointF &totalDelta)
{
    if (!m_dragActive)
        return;
    m_dragTotal = totalDelta;
    for (int i = 0; i < m_dragBefore.size(); ++i) {
        SceneObject moved = m_dragBefore.at(i);
        moved.translate(totalDelta);
        replaceObjectRaw(moved);
    }
    emit changed();
}

void SceneDocument::endDrag()
{
    if (!m_dragActive)
        return;
    QVector<Command *> parts;
    for (int i = 0; i < m_dragBefore.size(); ++i) {
        const SceneObject *now = object(m_dragBefore.at(i).id);
        if (!now)
            continue;
        if (!nearlySame(m_dragBefore.at(i), *now))
            parts.append(new ChangeCommand(m_dragBefore.at(i), *now));
    }
    m_dragActive = false;
    m_dragBefore.clear();
    m_dragIds.clear();
    if (parts.isEmpty())
        return;
    pushCommand(new BatchCommand(parts), true);
}

void SceneDocument::cancelDrag()
{
    if (!m_dragActive)
        return;
    for (int i = 0; i < m_dragBefore.size(); ++i)
        replaceObjectRaw(m_dragBefore.at(i));
    m_dragActive = false;
    m_dragBefore.clear();
    m_dragIds.clear();
    emit changed();
}

QString SceneDocument::hitTest(const QPointF &world, double tol, const Catalog *catalog) const
{
    QString best;
    int bestRank = -1;
    int bestIndex = -1;
    for (int i = 0; i < m_objects.size(); ++i) {
        const SceneObject &obj = m_objects.at(i);
        if (!obj.visible)
            continue;
        bool hit = false;
        if (obj.type == QLatin1String("symbol") || obj.type == QLatin1String("text")
                || obj.type == QLatin1String("compass") || obj.type == QLatin1String("parking")) {
            QRectF box = obj.worldBounds(catalog).adjusted(-tol, -tol, tol, tol);
            hit = box.contains(world);
        } else if (obj.type == QLatin1String("debris") && obj.closed && pointInPolygon(world, obj.points)) {
            hit = true;
        } else {
            for (int k = 1; k < obj.points.size(); ++k) {
                if (nearestOnSegment(world, obj.points.at(k - 1), obj.points.at(k), tol, 0)) {
                    hit = true;
                    break;
                }
            }
        }
        if (!hit)
            continue;
        const int rank = rankOf(obj.type);
        if (rank > bestRank || (rank == bestRank && i > bestIndex)) {
            bestRank = rank;
            bestIndex = i;
            best = obj.id;
        }
    }
    return best;
}

QRectF SceneDocument::contentBounds(const Catalog *catalog) const
{
    QRectF r;
    for (int i = 0; i < m_objects.size(); ++i)
        r = united(r, m_objects.at(i).worldBounds(catalog));
    if (!m_aerial.file.isEmpty() && m_aerial.imageWidth > 0 && m_aerial.imageHeight > 0) {
        const AerialPose pose = m_aerial.pose();
        QVector<QPointF> corners;
        corners << pose.pixelToWorld(QPointF(0, 0));
        corners << pose.pixelToWorld(QPointF(m_aerial.imageWidth, 0));
        corners << pose.pixelToWorld(QPointF(m_aerial.imageWidth, m_aerial.imageHeight));
        corners << pose.pixelToWorld(QPointF(0, m_aerial.imageHeight));
        r = united(r, boundsOf(corners));
    }
    return r;
}

QPointF SceneDocument::snapPoint(const QPointF &world, const Catalog *catalog, const QString &ignoreId) const
{
    if (!m_snap)
        return world;
    QPointF best = snapToGrid(world, m_grid);
    double bestD = dist(world, best);
    const double limit = qMax(0.35, 12.0 / qMax(0.2, m_zoom));
    for (int i = 0; i < m_objects.size(); ++i) {
        const SceneObject &obj = m_objects.at(i);
        if (obj.id == ignoreId)
            continue;
        QVector<QPointF> targets = obj.points;
        if (obj.type == QLatin1String("symbol"))
            targets += obj.drivingPointsWorld(catalog);
        else
            targets << QPointF(obj.x, obj.y);
        for (int k = 0; k < targets.size(); ++k) {
            const double d = dist(world, targets.at(k));
            if (d < bestD && d <= limit) {
                bestD = d;
                best = targets.at(k);
            }
        }
        if (obj.points.size() >= 2) {
            double unused = 0;
            const QPointF proj = projectOnPolyline(world, obj.points, &unused);
            if (unused < bestD && unused <= limit) {
                bestD = unused;
                best = proj;
            }
        }
    }
    return best;
}

QString SceneDocument::autoLabel(int numberType) const
{
    QStringList series;
    if (numberType == 1)
        series << QStringLiteral("甲") << QStringLiteral("乙") << QStringLiteral("丙") << QStringLiteral("丁")
                << QStringLiteral("戊") << QStringLiteral("己") << QStringLiteral("庚") << QStringLiteral("辛")
                << QStringLiteral("壬") << QStringLiteral("癸");
    else if (numberType == 2)
        series << QStringLiteral("A") << QStringLiteral("B") << QStringLiteral("C") << QStringLiteral("D")
                << QStringLiteral("E") << QStringLiteral("F");
    else if (numberType == 3)
        series << QStringLiteral("1") << QStringLiteral("2") << QStringLiteral("3") << QStringLiteral("4")
                << QStringLiteral("5") << QStringLiteral("6") << QStringLiteral("7") << QStringLiteral("8");
    else
        return QString();
    for (int i = 0; i < series.size(); ++i) {
        bool used = false;
        for (int k = 0; k < m_objects.size(); ++k) {
            if (m_objects.at(k).label == series.at(i)) {
                used = true;
                break;
            }
        }
        if (!used)
            return series.at(i);
    }
    return series.last();
}

static int filletObjects(QVector<SceneObject> &objects, double radius);

QString SceneDocument::placeTemplate(const TemplateDef &tpl, const QPointF &anchor)
{
    const QRectF box = tpl.bounds();
    const QPointF shift = box.isValid() ? anchor - box.center() : anchor;
    const QString group = newUuid();
    QVector<SceneObject> created;
    for (int i = 0; i < tpl.lines.size(); ++i) {
        const TplLine &line = tpl.lines.at(i);
        QVector<QPointF> pts = line.curve ? smoothPolyline(line.pts, 4) : line.pts;
        for (int k = 0; k < pts.size(); ++k)
            pts[k] += shift;
        SceneObject obj;
        obj.type = QStringLiteral("roadline");
        obj.name = tpl.name;
        obj.groupId = group;
        obj.points = pts;
        obj.lineStyle = line.style;
        created.append(obj);
    }
    for (int i = 0; i < tpl.marks.size(); ++i) {
        const TplMark &mark = tpl.marks.at(i);
        SceneObject obj;
        obj.groupId = group;
        obj.name = tpl.name;
        if (mark.type == QLatin1String("guide") && mark.pts.size() >= 2) {
            obj.type = QStringLiteral("guide");
            obj.points << (mark.pts.at(0) + shift) << (mark.pts.at(1) + shift);
        } else if (mark.type == QLatin1String("crosswalk") && mark.pts.size() >= 2) {
            obj.type = QStringLiteral("crosswalk");
            obj.points << (mark.pts.at(0) + shift) << (mark.pts.at(1) + shift);
            obj.widthM = mark.width > 0 ? mark.width : 3;
            obj.lineWidth = mark.interval > 0 ? mark.interval : 0.45;
        } else if (mark.type == QLatin1String("parking")) {
            obj.type = QStringLiteral("parking");
            obj.x = mark.x + shift.x();
            obj.y = mark.y + shift.y();
            obj.lengthM = mark.w;
            obj.widthM = mark.h;
            obj.rotation = mark.ang;
            obj.stallCount = qMax(1, mark.count);
        } else {
            continue;
        }
        created.append(obj);
    }
    filletObjects(created, 5.0);
    addObjects(created, tpl.name);
    return group;
}

static bool curbStyle(int style)
{
    return style == 1 || style == 5 || style == 6 || style == 8 || style == 9 || style == 10 || style == 11;
}

static QPointF towardGap(const SceneObject &obj, bool atStart)
{
    const QPointF d = atStart ? (obj.points.at(0) - obj.points.at(1))
                              : (obj.points.last() - obj.points.at(obj.points.size() - 2));
    const double len = QLineF(QPointF(0, 0), d).length();
    if (len < 1e-6)
        return QPointF(1, 0);
    return d / len;
}

static bool intersectRays(const QPointF &p1, const QPointF &d1, const QPointF &p2, const QPointF &d2,
                          QPointF *hit, double *t, double *s)
{
    const double cross = d1.x() * d2.y() - d1.y() * d2.x();
    if (qAbs(cross) < 1e-5)
        return false;
    const QPointF w = p2 - p1;
    *t = (w.x() * d2.y() - w.y() * d2.x()) / cross;
    *s = (w.x() * d1.y() - w.y() * d1.x()) / cross;
    *hit = p1 + d1 * (*t);
    return true;
}

static void trimEnd(QVector<QPointF> &pts, bool atStart, const QPointF &tangent)
{
    if (pts.size() < 2)
        return;
    if (atStart) {
        while (pts.size() >= 3) {
            const QPointF ab = pts.at(1) - pts.at(0);
            const double ab2 = QPointF::dotProduct(ab, ab);
            const double u = ab2 < 1e-8 ? 0 : QPointF::dotProduct(tangent - pts.at(0), ab) / ab2;
            if (u <= 1.02)
                break;
            pts.removeFirst();
        }
        pts[0] = tangent;
    } else {
        while (pts.size() >= 3) {
            const QPointF inward = pts.at(pts.size() - 2);
            const QPointF end = pts.at(pts.size() - 1);
            const QPointF ab = end - inward;
            const double ab2 = QPointF::dotProduct(ab, ab);
            const double u = ab2 < 1e-8 ? 1 : QPointF::dotProduct(tangent - inward, ab) / ab2;
            if (u >= -0.02)
                break;
            pts.removeLast();
        }
        pts[pts.size() - 1] = tangent;
    }
}

static QVector<QPointF> filletArc(const QPointF &center, const QPointF &t1, const QPointF &t2, double radius)
{
    const double a0 = qAtan2(t1.y() - center.y(), t1.x() - center.x());
    const double a1 = qAtan2(t2.y() - center.y(), t2.x() - center.x());
    double sweep = a1 - a0;
    const double pi = 3.141592653589793;
    while (sweep > pi)
        sweep -= 2 * pi;
    while (sweep < -pi)
        sweep += 2 * pi;
    const int steps = qBound(6, int(qAbs(sweep) / (pi / 16.0)), 28);
    QVector<QPointF> pts;
    for (int i = 0; i <= steps; ++i) {
        const double a = a0 + sweep * (double(i) / steps);
        pts.append(center + QPointF(qCos(a), qSin(a)) * radius);
    }
    return pts;
}

static int filletObjects(QVector<SceneObject> &objects, double radius)
{
    radius = qBound(1.5, radius, 12.0);
    const int count = objects.size();
    QVector<char> usedStart(count, 0);
    QVector<char> usedEnd(count, 0);
    QVector<SceneObject> arcs;
    int joined = 0;
    for (int i = 0; i < count; ++i) {
        SceneObject &a = objects[i];
        if (a.type != QLatin1String("roadline") || a.points.size() < 2 || !curbStyle(a.lineStyle))
            continue;
        if (a.name.contains(QStringLiteral("停止")))
            continue;
        if (polylineLength(a.points) < 8)
            continue;
        for (int j = i + 1; j < count; ++j) {
            SceneObject &b = objects[j];
            if (b.type != QLatin1String("roadline") || b.points.size() < 2 || b.lineStyle != a.lineStyle)
                continue;
            if (b.name.contains(QStringLiteral("停止")) || polylineLength(b.points) < 8)
                continue;
            for (int ea = 0; ea < 2; ++ea) {
                if ((ea == 0 && usedStart.at(i)) || (ea == 1 && usedEnd.at(i)))
                    continue;
                for (int eb = 0; eb < 2; ++eb) {
                    if ((eb == 0 && usedStart.at(j)) || (eb == 1 && usedEnd.at(j)))
                        continue;
                    const bool aStart = ea == 0;
                    const bool bStart = eb == 0;
                    const QPointF pa = aStart ? a.points.first() : a.points.last();
                    const QPointF pb = bStart ? b.points.first() : b.points.last();
                    const double gap = dist(pa, pb);
                    if (gap < 0.35 || gap > 16)
                        continue;
                    const QPointF d1 = towardGap(a, aStart);
                    const QPointF d2 = towardGap(b, bStart);
                    QPointF hit;
                    double t = 0;
                    double s = 0;
                    if (!intersectRays(pa, d1, pb, d2, &hit, &t, &s))
                        continue;
                    if (t < 0.2 || s < 0.2 || t > 24 || s > 24)
                        continue;
                    const QPointF u1 = -d1;
                    const QPointF u2 = -d2;
                    const double dot = qBound(-1.0, QPointF::dotProduct(u1, u2), 1.0);
                    const double theta = qAcos(dot);
                    if (theta < 0.45 || theta > 2.6)
                        continue;
                    const double halfTan = qTan(theta * 0.5);
                    if (halfTan < 0.15)
                        continue;
                    double useR = radius;
                    double trim = useR / halfTan;
                    const double limit = qMin(polylineLength(a.points), polylineLength(b.points)) * 0.55;
                    if (trim > limit) {
                        useR = limit * halfTan;
                        trim = limit;
                    }
                    if (useR < 1.2)
                        continue;
                    const QPointF t1 = hit + u1 * trim;
                    const QPointF t2 = hit + u2 * trim;
                    const QPointF bis = u1 + u2;
                    const double bisLen = QLineF(QPointF(0, 0), bis).length();
                    if (bisLen < 1e-4)
                        continue;
                    const QPointF center = hit + (bis / bisLen) * (useR / qSin(theta * 0.5));
                    trimEnd(a.points, aStart, t1);
                    trimEnd(b.points, bStart, t2);
                    SceneObject arc;
                    arc.type = QStringLiteral("roadline");
                    arc.name = QStringLiteral("路口圆角");
                    arc.groupId = a.groupId.isEmpty() ? b.groupId : a.groupId;
                    arc.lineStyle = a.lineStyle;
                    arc.points = filletArc(center, t1, t2, useR);
                    arcs.append(arc);
                    if (aStart)
                        usedStart[i] = 1;
                    else
                        usedEnd[i] = 1;
                    if (bStart)
                        usedStart[j] = 1;
                    else
                        usedEnd[j] = 1;
                    ++joined;
                }
            }
        }
    }
    for (int i = 0; i < arcs.size(); ++i)
        objects.append(arcs.at(i));
    return joined;
}

QString SceneDocument::filletJunctions(double radius)
{
    QVector<SceneObject> next = m_objects;
    const int joined = filletObjects(next, radius);
    if (joined <= 0)
        return QStringLiteral("没有可衔接的路口转角");
    const QVector<SceneObject> before = m_objects;
    m_objects = next;
    rebuildIndex();
    pushCommand(new SnapshotCommand(before, m_aerial, m_objects, m_aerial), true);
    return QStringLiteral("已为 %1 个路口转角添加圆角").arg(joined);
}

QString SceneDocument::placeSymbol(const Catalog &catalog, const QString &name, const QPointF &at)
{
    const SymbolDef *def = catalog.symbol(name);
    SceneObject obj;
    obj.type = QStringLiteral("symbol");
    obj.symbolId = name;
    obj.name = name;
    obj.x = at.x();
    obj.y = at.y();
    if (def) {
        obj.label = autoLabel(def->numberType);
        obj.lineWidth = 0.04;
    }
    return addObject(obj);
}

void SceneDocument::addRoad(const QVector<QPointF> &center, int lanes, double laneWidth)
{
    if (center.size() < 2 || lanes < 1)
        return;
    lanes = qBound(1, lanes, 8);
    laneWidth = qBound(2.0, laneWidth, 6.0);
    const QString group = newUuid();
    const double total = lanes * laneWidth;
    QVector<SceneObject> created;
    for (int i = 0; i <= lanes; ++i) {
        const double off = -total / 2.0 + i * laneWidth;
        SceneObject obj;
        obj.type = QStringLiteral("roadline");
        obj.name = QStringLiteral("手绘道路");
        obj.groupId = group;
        obj.points = offsetPolyline(center, off);
        if (i == 0 || i == lanes)
            obj.lineStyle = 1;
        else if (lanes % 2 == 0 && i == lanes / 2)
            obj.lineStyle = 3;
        else
            obj.lineStyle = 2;
        created.append(obj);
    }
    addObjects(created, QStringLiteral("道路"));
}

void SceneDocument::addCircleRoad(const QPointF &center, double radius, int lanes, double laneWidth)
{
    if (radius < 2)
        return;
    lanes = qBound(1, lanes, 4);
    laneWidth = qBound(2.5, laneWidth, 5.0);
    const QString group = newUuid();
    QVector<SceneObject> created;
    const int seg = 48;
    for (int ring = 0; ring <= lanes; ++ring) {
        const double r = radius + ring * laneWidth;
        QVector<QPointF> pts;
        for (int i = 0; i <= seg; ++i) {
            const double ang = 2.0 * 3.141592653589793 * double(i) / seg;
            pts.append(center + QPointF(qCos(ang), qSin(ang)) * r);
        }
        SceneObject obj;
        obj.type = QStringLiteral("roadline");
        obj.name = QStringLiteral("环岛");
        obj.groupId = group;
        obj.points = pts;
        obj.closed = true;
        obj.lineStyle = (ring == 0 || ring == lanes) ? 1 : 2;
        created.append(obj);
    }
    addObjects(created, QStringLiteral("环岛"));
}

void SceneDocument::addTrace(const QVector<QPointF> &pts, int type)
{
    if (pts.size() < 2)
        return;
    SceneObject obj;
    obj.type = QStringLiteral("trace");
    obj.subType = qBound(0, type, 6);
    obj.name = traceName(obj.subType);
    obj.points = pts;
    obj.showMeasure = true;
    obj.lineWidth = (type == 1 || type == 2) ? 0.28 : 0.12;
    addObject(obj);
}

void SceneDocument::addDebris(const QVector<QPointF> &pts, int variant)
{
    if (pts.size() < 2)
        return;
    SceneObject obj;
    obj.type = QStringLiteral("debris");
    obj.subType = variant;
    obj.isBlood = variant == 0;
    obj.closed = variant != 1;
    obj.name = obj.isBlood ? QStringLiteral("血迹") : (variant == 1 ? QStringLiteral("点状散落物") : QStringLiteral("散落物"));
    obj.points = pts;
    obj.showMeasure = true;
    addObject(obj);
}

void SceneDocument::addDimension(const QPointF &a, const QPointF &b, int style, const QString &fromId, const QString &toId)
{
    if (dist(a, b) < 0.05)
        return;
    SceneObject obj;
    obj.type = QStringLiteral("dimension");
    obj.name = QStringLiteral("标注");
    obj.points << a << b;
    obj.subType = qBound(0, style, 3);
    obj.fromId = fromId;
    obj.toId = toId;
    obj.showMeasure = true;
    addObject(obj);
}

void SceneDocument::addText(const QPointF &at, const QString &text)
{
    if (text.trimmed().isEmpty())
        return;
    SceneObject obj;
    obj.type = QStringLiteral("text");
    obj.x = at.x();
    obj.y = at.y();
    obj.label = text.trimmed();
    obj.name = QStringLiteral("文字");
    obj.fontSize = 0.9;
    addObject(obj);
}

void SceneDocument::addCrosswalk(const QPointF &a, const QPointF &b, double width)
{
    SceneObject obj;
    obj.type = QStringLiteral("crosswalk");
    obj.name = QStringLiteral("人行横道");
    obj.points << a << b;
    obj.widthM = width > 0 ? width : 3.0;
    obj.lineWidth = 0.45;
    addObject(obj);
}

void SceneDocument::addGuide(const QPointF &a, const QPointF &b)
{
    SceneObject obj;
    obj.type = QStringLiteral("guide");
    obj.name = QStringLiteral("导向箭头");
    obj.points << a << b;
    addObject(obj);
}

void SceneDocument::addParking(const QPointF &at, double length, double width, int stalls, double rotation)
{
    SceneObject obj;
    obj.type = QStringLiteral("parking");
    obj.name = QStringLiteral("停车位");
    obj.x = at.x();
    obj.y = at.y();
    obj.lengthM = length > 0 ? length : 2.5 * qMax(1, stalls);
    obj.widthM = width > 0 ? width : 5.0;
    obj.stallCount = qMax(1, stalls);
    obj.rotation = rotation;
    addObject(obj);
}

void SceneDocument::ensureCompass()
{
    for (int i = 0; i < m_objects.size(); ++i) {
        if (m_objects.at(i).type == QLatin1String("compass"))
            return;
    }
    SceneObject obj;
    obj.type = QStringLiteral("compass");
    obj.name = QStringLiteral("指北针");
    obj.x = 0;
    obj.y = 0;
    addObject(obj);
}

void SceneDocument::parkCompass(const Catalog *catalog)
{
    int found = -1;
    for (int i = 0; i < m_objects.size(); ++i) {
        if (m_objects.at(i).type == QLatin1String("compass")) {
            found = i;
            break;
        }
    }
    if (found < 0) {
        ensureCompass();
        for (int i = 0; i < m_objects.size(); ++i) {
            if (m_objects.at(i).type == QLatin1String("compass")) {
                found = i;
                break;
            }
        }
    }
    if (found < 0)
        return;
    QRectF box;
    for (int i = 0; i < m_objects.size(); ++i) {
        if (i == found)
            continue;
        box = united(box, m_objects.at(i).worldBounds(catalog));
    }
    if (!box.isValid()) {
        m_objects[found].x = 6;
        m_objects[found].y = 6;
    } else {
        m_objects[found].x = box.right() + 4.0;
        m_objects[found].y = box.bottom() + 4.0;
    }
    emit changed();
}

void SceneDocument::clearDrawings()
{
    const QVector<SceneObject> before = m_objects;
    const AerialLayer aerialBefore = m_aerial;
    QVector<SceneObject> kept;
    for (int i = 0; i < m_objects.size(); ++i) {
        if (m_objects.at(i).type == QLatin1String("compass"))
            kept.append(m_objects.at(i));
    }
    if (kept.isEmpty()) {
        SceneObject compass;
        compass.id = newUuid();
        compass.type = QStringLiteral("compass");
        compass.name = QStringLiteral("指北针");
        kept.append(compass);
    }
    m_objects = kept;
    rebuildIndex();
    m_selection.clear();
    pushCommand(new SnapshotCommand(before, aerialBefore, m_objects, m_aerial), true);
    emit selectionChanged();
}

QString SceneDocument::proportionalize()
{
    int linked = 0;
    for (int i = 0; i < m_objects.size(); ++i) {
        const SceneObject &obj = m_objects.at(i);
        if (obj.type == QLatin1String("dimension") && obj.measured > 0 && !obj.fromId.isEmpty())
            ++linked;
    }
    if (linked > 0) {
        const QVector<SceneObject> before = m_objects;
        QHash<QString, QPointF> deltaSum;
        QHash<QString, int> deltaCount;
        QStringList dimIds;
        for (int i = 0; i < m_objects.size(); ++i) {
            const SceneObject &dim = m_objects.at(i);
            if (dim.type != QLatin1String("dimension") || dim.measured <= 0 || dim.points.size() < 2 || dim.fromId.isEmpty())
                continue;
            const QPointF a = dim.points.at(0);
            const QPointF b = dim.points.at(1);
            const double len = dist(a, b);
            if (len < 0.05)
                continue;
            const QPointF dir = (a - b) / len;
            const QPointF newA = b + dir * dim.measured;
            deltaSum[dim.fromId] += newA - a;
            deltaCount[dim.fromId] += 1;
            dimIds << dim.id;
        }
        if (deltaSum.isEmpty())
            return QStringLiteral("实测距离与端点无效，无法比例化");
        QHash<QString, QPointF>::const_iterator it = deltaSum.constBegin();
        while (it != deltaSum.constEnd()) {
            const int n = qMax(1, deltaCount.value(it.key()));
            const QPointF delta = it.value() / n;
            SceneObject *target = object(it.key());
            if (target && !target->locked)
                target->translate(delta);
            ++it;
        }
        for (int i = 0; i < dimIds.size(); ++i) {
            SceneObject *dim = object(dimIds.at(i));
            if (!dim || dim->points.size() < 2)
                continue;
            const QPointF b = dim->points.at(1);
            const QPointF a = dim->points.at(0);
            const double len = dist(a, b);
            if (len < 1e-6)
                continue;
            dim->points[0] = b + (a - b) / len * dim->measured;
            dim->determined = true;
        }
        pushCommand(new SnapshotCommand(before, m_aerial, m_objects, m_aerial), true);
        return QStringLiteral("已按实测距离移动 %1 个对象").arg(deltaSum.size());
    }
    return scaleToMeasures();
}

QString SceneDocument::scaleToMeasures()
{
    QVector<double> factors;
    QPointF acc;
    int accN = 0;
    for (int i = 0; i < m_objects.size(); ++i) {
        const SceneObject &dim = m_objects.at(i);
        if (dim.type != QLatin1String("dimension") || dim.measured <= 0)
            continue;
        const double geom = geometricLength(dim);
        if (geom < 0.05)
            continue;
        factors.append(dim.measured / geom);
        if (dim.points.size() >= 2) {
            acc += (dim.points.at(0) + dim.points.at(1)) * 0.5;
            ++accN;
        }
    }
    if (factors.isEmpty())
        return QStringLiteral("请先给标注填写实测米数");
    const double factor = medianOf(factors);
    if (qAbs(factor - 1.0) < 0.002)
        return QStringLiteral("图面比例已与实测一致");
    const QPointF origin = accN > 0 ? acc / accN : QPointF(0, 0);
    const QVector<SceneObject> before = m_objects;
    scaleEverything(origin, factor);
    for (int i = 0; i < m_objects.size(); ++i) {
        if (m_objects.at(i).type == QLatin1String("dimension") && m_objects.at(i).measured > 0)
            m_objects[i].determined = true;
    }
    pushCommand(new SnapshotCommand(before, m_aerial, m_objects, m_aerial), true);
    return QStringLiteral("已按实测中位数整体缩放 ×%1").arg(QString::number(factor, 'f', 3));
}

void SceneDocument::scaleEverything(const QPointF &origin, double factor)
{
    for (int i = 0; i < m_objects.size(); ++i)
        m_objects[i].scaleAbout(origin, factor);
}

QString SceneDocument::calibrateAerial(double knownMetres)
{
    if (!m_aerial.hasRefs)
        return QStringLiteral("请在航拍图上点两个参考点");
    if (knownMetres <= 0)
        return QStringLiteral("请输入两点间的地面距离（米）");
    AerialPose unit = m_aerial.pose();
    unit.mpp = 1;
    const double pixelDist = dist(unit.pixelToWorld(QPointF(m_aerial.refAx, m_aerial.refAy)),
                                   unit.pixelToWorld(QPointF(m_aerial.refBx, m_aerial.refBy)));
    if (pixelDist < 1.0)
        return QStringLiteral("两个参考点太近");
    const double newMpp = knownMetres / pixelDist;
    const double oldMpp = m_aerial.mpp > 1e-9 ? m_aerial.mpp : 0.05;
    const double factor = newMpp / oldMpp;
    const QVector<SceneObject> before = m_objects;
    const AerialLayer aerialBefore = m_aerial;
    if (qAbs(factor - 1.0) > 1e-6)
        scaleEverything(QPointF(0, 0), factor);
    m_aerial.mpp = newMpp;
    m_aerial.knownMetres = knownMetres;
    m_aerial.calibrated = true;
    pushCommand(new SnapshotCommand(before, aerialBefore, m_objects, m_aerial), true);
    return QStringLiteral("已标定：1 像素 = %1 米").arg(QString::number(newMpp, 'f', 4));
}

QVariantMap SceneDocument::selectionSummary(const Catalog *catalog) const
{
    QVariantMap map;
    const SceneObject *obj = object(m_selection);
    if (!obj)
        return map;
    map.insert(QStringLiteral("id"), obj->id);
    map.insert(QStringLiteral("type"), obj->type);
    map.insert(QStringLiteral("typeLabel"), obj->typeLabel());
    map.insert(QStringLiteral("name"), obj->name);
    map.insert(QStringLiteral("label"), obj->label);
    map.insert(QStringLiteral("x"), obj->x);
    map.insert(QStringLiteral("y"), obj->y);
    map.insert(QStringLiteral("rotation"), obj->rotation);
    map.insert(QStringLiteral("locked"), obj->locked);
    map.insert(QStringLiteral("showMeasure"), obj->showMeasure);
    map.insert(QStringLiteral("isDatum"), obj->isDatum);
    map.insert(QStringLiteral("lineStyle"), obj->lineStyle);
    map.insert(QStringLiteral("subType"), obj->subType);
    map.insert(QStringLiteral("styleIndex"), obj->styleIndex);
    map.insert(QStringLiteral("fontSize"), obj->fontSize);
    map.insert(QStringLiteral("stallCount"), obj->stallCount);
    map.insert(QStringLiteral("groupId"), obj->groupId);
    map.insert(QStringLiteral("symbolId"), obj->symbolId);
    map.insert(QStringLiteral("isBlood"), obj->isBlood);
    map.insert(QStringLiteral("measured"), obj->measured);
    map.insert(QStringLiteral("determined"), obj->determined);
    const QRectF box = obj->worldBounds(catalog);
    double length = obj->lengthM;
    double width = obj->widthM;
    if (obj->type == QLatin1String("symbol")) {
        const QRectF natural = obj->localSymbolBounds(catalog);
        if (length <= 0)
            length = natural.width() * obj->scaleX;
        if (width <= 0)
            width = natural.height() * obj->scaleY;
        map.insert(QStringLiteral("styles"), catalog ? catalog->styleNames(obj->symbolId) : QStringList());
    } else if (obj->type == QLatin1String("trace") || obj->type == QLatin1String("dimension") || obj->type == QLatin1String("roadline")) {
        length = geometricLength(*obj);
    } else if (obj->type == QLatin1String("debris")) {
        length = polygonArea(obj->points);
    }
    map.insert(QStringLiteral("length"), length);
    map.insert(QStringLiteral("width"), width);
    map.insert(QStringLiteral("boundsWidth"), box.width());
    map.insert(QStringLiteral("boundsHeight"), box.height());
    return map;
}

QVariantList SceneDocument::objectList() const
{
    QVariantList list;
    for (int i = m_objects.size() - 1; i >= 0; --i) {
        const SceneObject &obj = m_objects.at(i);
        QVariantMap row;
        row.insert(QStringLiteral("id"), obj.id);
        row.insert(QStringLiteral("type"), obj.type);
        row.insert(QStringLiteral("title"), obj.label.isEmpty() ? obj.typeLabel() : obj.typeLabel() + QStringLiteral(" ") + obj.label);
        row.insert(QStringLiteral("locked"), obj.locked);
        list.append(row);
    }
    return list;
}

bool SceneDocument::setProperty(const QString &key, const QVariant &value, const Catalog *catalog)
{
    SceneObject *obj = object(m_selection);
    if (!obj || (obj->locked && key != QLatin1String("locked")))
        return false;
    SceneObject before = *obj;
    if (key == QLatin1String("x"))
        obj->x = value.toDouble();
    else if (key == QLatin1String("y"))
        obj->y = value.toDouble();
    else if (key == QLatin1String("rotation"))
        obj->rotation = value.toDouble();
    else if (key == QLatin1String("label"))
        obj->label = value.toString();
    else if (key == QLatin1String("locked"))
        obj->locked = value.toBool();
    else if (key == QLatin1String("showMeasure"))
        obj->showMeasure = value.toBool();
    else if (key == QLatin1String("isDatum"))
        obj->isDatum = value.toBool();
    else if (key == QLatin1String("lineStyle"))
        obj->lineStyle = value.toInt();
    else if (key == QLatin1String("subType"))
        obj->subType = value.toInt();
    else if (key == QLatin1String("styleIndex"))
        obj->styleIndex = value.toInt();
    else if (key == QLatin1String("fontSize"))
        obj->fontSize = qMax(0.2, value.toDouble());
    else if (key == QLatin1String("stallCount"))
        obj->stallCount = qMax(1, value.toInt());
    else if (key == QLatin1String("measured")) {
        const QString text = value.toString().trimmed();
        obj->measured = text.isEmpty() ? -1 : text.toDouble();
        obj->determined = obj->measured >= 0;
    } else if (key == QLatin1String("length")) {
        obj->lengthM = qMax(0.01, value.toDouble());
        if (obj->type == QLatin1String("symbol") && catalog) {
            const QRectF natural = catalog->naturalBounds(obj->symbolId, obj->styleIndex);
            if (natural.width() > 1e-4)
                obj->scaleX = obj->lengthM / natural.width();
        }
    } else if (key == QLatin1String("width")) {
        obj->widthM = qMax(0.01, value.toDouble());
        if (obj->type == QLatin1String("symbol") && catalog) {
            const QRectF natural = catalog->naturalBounds(obj->symbolId, obj->styleIndex);
            if (natural.height() > 1e-4)
                obj->scaleY = obj->widthM / natural.height();
        }
    } else {
        return false;
    }
    if (nearlySame(before, *obj))
        return true;
    pushCommand(new ChangeCommand(before, *obj), true);
    emit selectionChanged();
    return true;
}

void SceneDocument::fitView(double viewWidth, double viewHeight, const Catalog *catalog)
{
    const QRectF box = contentBounds(catalog);
    if (!box.isValid() || box.width() < 0.1 || box.height() < 0.1) {
        m_zoom = 8;
        m_panX = viewWidth * 0.5;
        m_panY = viewHeight * 0.55;
        emit viewChanged();
        return;
    }
    const double margin = 72;
    const double availW = qMax(120.0, viewWidth - margin * 2);
    const double availH = qMax(120.0, viewHeight - margin * 2);
    const double zx = availW / qMax(0.5, box.width());
    const double zy = availH / qMax(0.5, box.height());
    m_zoom = qBound(1.5, qMin(zx, zy), 24.0);
    const QPointF c = box.center();
    m_panX = viewWidth * 0.5 - c.x() * m_zoom;
    m_panY = viewHeight * 0.5 + c.y() * m_zoom;
    emit viewChanged();
}

QJsonObject SceneDocument::toJson() const
{
    QJsonObject o;
    o.insert(QStringLiteral("grid"), m_grid);
    o.insert(QStringLiteral("snap"), m_snap);
    o.insert(QStringLiteral("zoom"), m_zoom);
    o.insert(QStringLiteral("panX"), m_panX);
    o.insert(QStringLiteral("panY"), m_panY);
    o.insert(QStringLiteral("aerial"), m_aerial.toJson());
    QJsonArray arr;
    for (int i = 0; i < m_objects.size(); ++i)
        arr.append(m_objects.at(i).toJson());
    o.insert(QStringLiteral("objects"), arr);
    return o;
}

void SceneDocument::fromJson(const QJsonObject &obj)
{
    m_objects.clear();
    m_selection.clear();
    clearHistory();
    m_grid = obj.value(QStringLiteral("grid")).toDouble(5);
    m_snap = obj.value(QStringLiteral("snap")).toBool(true);
    m_zoom = obj.value(QStringLiteral("zoom")).toDouble(8);
    m_panX = obj.value(QStringLiteral("panX")).toDouble(400);
    m_panY = obj.value(QStringLiteral("panY")).toDouble(300);
    m_aerial.fromJson(obj.value(QStringLiteral("aerial")).toObject());
    const QJsonArray arr = obj.value(QStringLiteral("objects")).toArray();
    for (int i = 0; i < arr.size(); ++i) {
        SceneObject item = SceneObject::fromJson(arr.at(i).toObject());
        if (item.id.isEmpty())
            item.id = newUuid();
        if (item.type.isEmpty())
            continue;
        m_objects.append(item);
    }
    rebuildIndex();
    emit changed();
    emit selectionChanged();
    emit viewChanged();
}

} // namespace sr
