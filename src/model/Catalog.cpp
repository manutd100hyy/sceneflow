#include "Catalog.h"

#include "Geometry.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QStringList>

namespace sr {

Prim::Prim()
    : k(0), x(0), y(0), w(0), h(0), r(0), a0(0), a1(0), cr(0), filled(false), cw(false)
{
}

TplMark::TplMark()
    : width(3), interval(0.5), x(0), y(0), w(2.5), h(5), ang(0), count(1)
{
}

static QPointF readPt(const QJsonValue &v)
{
    const QJsonArray a = v.toArray();
    if (a.size() < 2)
        return QPointF();
    return QPointF(a.at(0).toDouble(), a.at(1).toDouble());
}

static QVector<QPointF> readPts(const QJsonValue &v)
{
    QVector<QPointF> pts;
    const QJsonArray a = v.toArray();
    for (int i = 0; i < a.size(); ++i)
        pts.append(readPt(a.at(i)));
    return pts;
}

static Prim readPrim(const QJsonObject &o)
{
    Prim p;
    p.k = o.value(QStringLiteral("k")).toInt();
    if (o.contains(QStringLiteral("a")))
        p.pts.append(readPt(o.value(QStringLiteral("a"))));
    if (o.contains(QStringLiteral("b")))
        p.pts.append(readPt(o.value(QStringLiteral("b"))));
    if (o.contains(QStringLiteral("c")))
        p.pts.append(readPt(o.value(QStringLiteral("c"))));
    if (o.contains(QStringLiteral("n")))
        p.pts = readPts(o.value(QStringLiteral("n")));
    if (o.contains(QStringLiteral("p")))
        p.pts = readPts(o.value(QStringLiteral("p")));
    p.x = o.value(QStringLiteral("x")).toDouble();
    p.y = o.value(QStringLiteral("y")).toDouble();
    p.w = o.value(QStringLiteral("w")).toDouble();
    p.h = o.value(QStringLiteral("h")).toDouble();
    p.r = o.value(QStringLiteral("r")).toDouble();
    p.a0 = o.value(QStringLiteral("a0")).toDouble();
    p.a1 = o.value(QStringLiteral("a1")).toDouble();
    p.cr = o.value(QStringLiteral("cr")).toDouble();
    p.filled = o.value(QStringLiteral("f")).toBool();
    p.cw = o.value(QStringLiteral("cw")).toBool();
    return p;
}

static QRectF primBounds(const Prim &p)
{
    if (p.k == 0 && p.pts.size() >= 2)
        return boundsOf(p.pts);
    if (p.k == 1 || p.k == 3)
        return QRectF(p.x, p.y, p.w, p.h);
    if ((p.k == 2 || p.k == 9) && !p.pts.isEmpty())
        return QRectF(p.pts.first().x() - p.r, p.pts.first().y() - p.r, p.r * 2, p.r * 2);
    if (p.k == 4 || p.k == 7 || p.k == 8)
        return boundsOf(p.pts);
    if (p.k == 5 && !p.pts.isEmpty())
        return QRectF(p.pts.first().x() - p.r, p.pts.first().y() - p.r, p.r * 2, p.r * 2);
    return QRectF();
}

QRectF TemplateDef::bounds() const
{
    QRectF r;
    for (int i = 0; i < lines.size(); ++i)
        r = united(r, boundsOf(lines.at(i).pts));
    for (int i = 0; i < marks.size(); ++i)
        r = united(r, boundsOf(marks.at(i).pts));
    return r;
}

static QString iconUrl(const QString &relative)
{
    if (relative.isEmpty())
        return QString();
    QString path = relative;
    path.replace(QLatin1String(" "), QLatin1String("%20"));
    return QStringLiteral("qrc:/menuicons/") + path;
}

static void loadIconMap(const QString &dataDir, QHash<QString, QString> *items, QHash<QString, QString> *templates)
{
    QFile file(dataDir + QStringLiteral("/icon_map.json"));
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    const QJsonObject itemObj = root.value(QStringLiteral("items")).toObject();
    for (QJsonObject::const_iterator it = itemObj.constBegin(); it != itemObj.constEnd(); ++it)
        items->insert(it.key(), iconUrl(it.value().toString()));
    const QJsonObject tplObj = root.value(QStringLiteral("templates")).toObject();
    for (QJsonObject::const_iterator it = tplObj.constBegin(); it != tplObj.constEnd(); ++it)
        templates->insert(it.key(), iconUrl(it.value().toString()));
}

static void prependCommonGroup(QVariantList *groups)
{
    const QStringList names = QStringList()
            << QStringLiteral("小轿车")
            << QStringLiteral("客车")
            << QStringLiteral("货车")
            << QStringLiteral("二轮摩托车")
            << QStringLiteral("电瓶车")
            << QStringLiteral("电动自行车")
            << QStringLiteral("自行车")
            << QStringLiteral("三轮车")
            << QStringLiteral("人体");
    QVariantList items;
    for (int i = 0; i < names.size(); ++i) {
        const QString want = names.at(i);
        bool found = false;
        for (int g = 0; g < groups->size() && !found; ++g) {
            const QVariantList src = groups->at(g).toMap().value(QStringLiteral("items")).toList();
            for (int k = 0; k < src.size(); ++k) {
                const QVariantMap row = src.at(k).toMap();
                if (row.value(QStringLiteral("name")).toString() == want) {
                    items.append(row);
                    found = true;
                    break;
                }
            }
        }
    }
    QVariantMap group;
    group.insert(QStringLiteral("name"), QStringLiteral("常用"));
    group.insert(QStringLiteral("items"), items);
    groups->prepend(group);
}

static bool aliasMatches(const QString &symbolName, const QString &query)
{
    const QString pairs[] = {
        QStringLiteral("小货车"), QStringLiteral("货车"),
        QStringLiteral("轻型货车"), QStringLiteral("货车"),
        QStringLiteral("面包车"), QStringLiteral("客车"),
        QStringLiteral("面包车"), QStringLiteral("小轿车"),
    };
    for (int i = 0; i < 4; ++i) {
        const QString alias = pairs[i * 2];
        const QString symbol = pairs[i * 2 + 1];
        if (symbolName != symbol)
            continue;
        if (alias.contains(query) || query.contains(alias))
            return true;
    }
    return false;
}

Catalog::Catalog()
{
}

bool Catalog::load(const QString &dataDir, QString *error)
{
    m_dir = dataDir;
    m_symbols.clear();
    m_symIndex.clear();
    m_uuidIndex.clear();
    m_templates.clear();
    m_tplIndex.clear();
    m_tplList.clear();
    m_groups.clear();

    QFile symFile(dataDir + QStringLiteral("/symbols.json"));
    if (!symFile.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QStringLiteral("无法读取符号库");
        return false;
    }
    const QJsonArray symbols = QJsonDocument::fromJson(symFile.readAll()).object().value(QStringLiteral("symbols")).toArray();
    for (int i = 0; i < symbols.size(); ++i) {
        const QJsonObject o = symbols.at(i).toObject();
        SymbolDef def;
        def.name = o.value(QStringLiteral("name")).toString();
        def.uuid = o.value(QStringLiteral("uuid")).toString();
        def.legacyKind = o.value(QStringLiteral("legacyKind")).toInt();
        def.numberType = o.value(QStringLiteral("numberType")).toInt();
        def.priority = o.value(QStringLiteral("priority")).toInt(1);
        def.fill = o.value(QStringLiteral("fill")).toString();
        def.line = o.value(QStringLiteral("line")).toString();
        const QJsonArray styles = o.value(QStringLiteral("styles")).toArray();
        for (int s = 0; s < styles.size(); ++s) {
            const QJsonObject st = styles.at(s).toObject();
            SymbolStyle style;
            style.name = st.value(QStringLiteral("name")).toString();
            const QJsonArray prims = st.value(QStringLiteral("prims")).toArray();
            for (int p = 0; p < prims.size(); ++p)
                style.prims.append(readPrim(prims.at(p).toObject()));
            style.skel = readPts(st.value(QStringLiteral("skel")));
            const QJsonArray dps = st.value(QStringLiteral("points")).toArray();
            for (int d = 0; d < dps.size(); ++d) {
                const QJsonObject dp = dps.at(d).toObject();
                style.driving.append(QPointF(dp.value(QStringLiteral("x")).toDouble(), dp.value(QStringLiteral("y")).toDouble()));
                style.drivingNames.append(dp.value(QStringLiteral("name")).toString());
            }
            def.styles.append(style);
        }
        if (def.name.isEmpty() || def.styles.isEmpty())
            continue;
        const QString uuidKey = def.uuid.toUpper();
        if (!uuidKey.isEmpty() && !m_uuidIndex.contains(uuidKey))
            m_uuidIndex.insert(uuidKey, m_symbols.size());
        m_symIndex.insert(def.name, m_symbols.size());
        m_symbols.append(def);
    }

    QFile tplFile(dataDir + QStringLiteral("/templates.json"));
    if (!tplFile.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QStringLiteral("无法读取道路模板");
        return false;
    }
    const QJsonArray tpls = QJsonDocument::fromJson(tplFile.readAll()).object().value(QStringLiteral("templates")).toArray();
    for (int i = 0; i < tpls.size(); ++i) {
        const QJsonObject o = tpls.at(i).toObject();
        TemplateDef def;
        def.id = o.value(QStringLiteral("id")).toString();
        def.name = o.value(QStringLiteral("name")).toString();
        const QJsonArray lines = o.value(QStringLiteral("lines")).toArray();
        for (int k = 0; k < lines.size(); ++k) {
            const QJsonObject ln = lines.at(k).toObject();
            TplLine line;
            line.style = ln.value(QStringLiteral("style")).toInt(1);
            line.curve = ln.value(QStringLiteral("curve")).toBool();
            line.pts = readPts(ln.value(QStringLiteral("pts")));
            if (line.pts.size() >= 2)
                def.lines.append(line);
        }
        const QJsonArray marks = o.value(QStringLiteral("marks")).toArray();
        for (int k = 0; k < marks.size(); ++k) {
            const QJsonObject mk = marks.at(k).toObject();
            TplMark mark;
            mark.type = mk.value(QStringLiteral("type")).toString();
            mark.pts = readPts(mk.value(QStringLiteral("pts")));
            mark.width = mk.value(QStringLiteral("width")).toDouble(mark.width);
            mark.interval = mk.value(QStringLiteral("interval")).toDouble(mark.interval);
            mark.x = mk.value(QStringLiteral("x")).toDouble();
            mark.y = mk.value(QStringLiteral("y")).toDouble();
            mark.w = mk.value(QStringLiteral("w")).toDouble(mark.w);
            mark.h = mk.value(QStringLiteral("h")).toDouble(mark.h);
            mark.ang = mk.value(QStringLiteral("ang")).toDouble();
            mark.count = mk.value(QStringLiteral("count")).toInt(mark.count);
            def.marks.append(mark);
        }
        m_tplIndex.insert(def.id, m_templates.size());
        m_templates.append(def);
    }

    QHash<QString, QString> itemIcons;
    QHash<QString, QString> templateIcons;
    loadIconMap(dataDir, &itemIcons, &templateIcons);

    QFile menuFile(dataDir + QStringLiteral("/menu.json"));
    if (menuFile.open(QIODevice::ReadOnly)) {
        const QJsonObject menu = QJsonDocument::fromJson(menuFile.readAll()).object();
        const QJsonArray templates = menu.value(QStringLiteral("templates")).toArray();
        for (int i = 0; i < templates.size(); ++i) {
            const QJsonObject o = templates.at(i).toObject();
            QVariantMap row;
            const QString templateName = o.value(QStringLiteral("name")).toString();
            row.insert(QStringLiteral("name"), templateName);
            row.insert(QStringLiteral("id"), o.value(QStringLiteral("id")).toString());
            row.insert(QStringLiteral("icon"), templateIcons.value(templateName));
            m_tplList.append(row);
        }
        const QJsonArray groups = menu.value(QStringLiteral("groups")).toArray();
        for (int i = 0; i < groups.size(); ++i) {
            const QJsonObject g = groups.at(i).toObject();
            QVariantList items;
            const QJsonArray arr = g.value(QStringLiteral("items")).toArray();
            const QString groupName = g.value(QStringLiteral("name")).toString();
            for (int k = 0; k < arr.size(); ++k) {
                const QJsonObject it = arr.at(k).toObject();
                const QString itemName = it.value(QStringLiteral("name")).toString();
                QVariantMap row;
                row.insert(QStringLiteral("name"), itemName);
                row.insert(QStringLiteral("notification"), it.value(QStringLiteral("notification")).toString());
                row.insert(QStringLiteral("flag"), it.value(QStringLiteral("flag")).toInt());
                row.insert(QStringLiteral("uuid"), it.value(QStringLiteral("uuid")).toString());
                row.insert(QStringLiteral("icon"), itemIcons.value(groupName + QLatin1Char('/') + itemName));
                items.append(row);
            }
            QVariantMap group;
            group.insert(QStringLiteral("name"), groupName);
            group.insert(QStringLiteral("items"), items);
            m_groups.append(group);
        }
        prependCommonGroup(&m_groups);
    }
    if (m_symbols.isEmpty()) {
        if (error)
            *error = QStringLiteral("符号库为空");
        return false;
    }
    return true;
}

QString Catalog::dataDir() const { return m_dir; }
int Catalog::symbolCount() const { return m_symbols.size(); }
int Catalog::templateCount() const { return m_templates.size(); }
const QVector<SymbolDef> &Catalog::symbols() const { return m_symbols; }
const QVector<TemplateDef> &Catalog::templates() const { return m_templates; }
QVariantList Catalog::templateList() const { return m_tplList; }
QVariantList Catalog::libraryGroups() const { return m_groups; }

const SymbolDef *Catalog::symbol(const QString &name) const
{
    const QHash<QString, int>::const_iterator it = m_symIndex.constFind(name);
    if (it == m_symIndex.constEnd())
        return 0;
    return &m_symbols.at(it.value());
}

const SymbolDef *Catalog::symbolByUuid(const QString &uuid) const
{
    if (uuid.isEmpty())
        return 0;
    const QHash<QString, int>::const_iterator it = m_uuidIndex.constFind(uuid.toUpper());
    if (it == m_uuidIndex.constEnd())
        return 0;
    return &m_symbols.at(it.value());
}

QString Catalog::resolveSymbolName(const QString &uuid, const QString &name) const
{
    if (const SymbolDef *byId = symbolByUuid(uuid))
        return byId->name;
    if (symbol(name))
        return name;
    return QString();
}

QVariantList Catalog::searchLibrary(const QString &query, int groupIndex) const
{
    const QString q = query.trimmed();
    QVariantList out;
    QSet<QString> seen;
    for (int g = 0; g < m_groups.size(); ++g) {
        if (q.isEmpty() && g != groupIndex)
            continue;
        const QVariantList items = m_groups.at(g).toMap().value(QStringLiteral("items")).toList();
        for (int i = 0; i < items.size(); ++i) {
            const QVariantMap row = items.at(i).toMap();
            const QString name = row.value(QStringLiteral("name")).toString();
            const QString uuid = row.value(QStringLiteral("uuid")).toString();
            const QString notification = row.value(QStringLiteral("notification")).toString();
            const QString resolved = resolveSymbolName(uuid, name);
            bool hit = q.isEmpty() || name.contains(q) || (!resolved.isEmpty() && resolved != name && resolved.contains(q));
            if (!hit)
                hit = aliasMatches(name, q) || (!resolved.isEmpty() && aliasMatches(resolved, q));
            if (!hit)
                continue;
            const QString key = name + QLatin1Char('\n') + uuid + QLatin1Char('\n') + notification;
            if (seen.contains(key))
                continue;
            seen.insert(key);
            out.append(row);
        }
    }
    return out;
}

const TemplateDef *Catalog::templateById(const QString &id) const
{
    const QHash<QString, int>::const_iterator it = m_tplIndex.constFind(id);
    if (it == m_tplIndex.constEnd())
        return 0;
    return &m_templates.at(it.value());
}

QString Catalog::templateIdForName(const QString &name) const
{
    QString partial;
    for (int i = 0; i < m_tplList.size(); ++i) {
        const QVariantMap row = m_tplList.at(i).toMap();
        const QString n = row.value(QStringLiteral("name")).toString();
        if (n == name)
            return row.value(QStringLiteral("id")).toString();
        if (partial.isEmpty() && (n.contains(name) || name.contains(n)))
            partial = row.value(QStringLiteral("id")).toString();
    }
    if (!partial.isEmpty())
        return partial;
    for (int i = 0; i < m_templates.size(); ++i) {
        if (m_templates.at(i).name.contains(name))
            return m_templates.at(i).id;
    }
    return QString();
}

const SymbolStyle *Catalog::styleOf(const QString &name, int styleIndex) const
{
    const SymbolDef *def = symbol(name);
    if (!def || def->styles.isEmpty())
        return 0;
    if (styleIndex < 0 || styleIndex >= def->styles.size())
        styleIndex = 0;
    return &def->styles.at(styleIndex);
}

QRectF Catalog::naturalBounds(const QString &name, int styleIndex) const
{
    const SymbolStyle *st = styleOf(name, styleIndex);
    if (!st)
        return QRectF(-1, -0.5, 2, 1);
    QRectF r;
    for (int i = 0; i < st->prims.size(); ++i)
        r = united(r, primBounds(st->prims.at(i)));
    if (!r.isValid())
        r = boundsOf(st->skel);
    if (!r.isValid())
        r = QRectF(-1, -0.5, 2, 1);
    return r.normalized();
}

QVector<QPointF> Catalog::drivingPoints(const QString &name, int styleIndex) const
{
    const SymbolStyle *st = styleOf(name, styleIndex);
    if (!st || st->driving.isEmpty())
        return QVector<QPointF>() << QPointF(0, 0);
    return st->driving;
}

QStringList Catalog::styleNames(const QString &name) const
{
    QStringList names;
    const SymbolDef *def = symbol(name);
    if (!def)
        return names;
    for (int i = 0; i < def->styles.size(); ++i)
        names << def->styles.at(i).name;
    return names;
}

} // namespace sr
