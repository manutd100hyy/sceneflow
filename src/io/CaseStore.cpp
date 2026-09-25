#include "CaseStore.h"

#include "model/Geometry.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace sr {

static QString partyText(const QJsonArray &parties, bool plates)
{
    QStringList parts;
    for (int i = 0; i < parties.size(); ++i) {
        const QJsonObject o = parties.at(i).toObject();
        const QString value = plates ? o.value(QStringLiteral("plate")).toString()
                                      : o.value(QStringLiteral("name")).toString();
        if (!value.trimmed().isEmpty())
            parts << value.trimmed();
    }
    return parts.join(QStringLiteral(" "));
}

CaseStore::CaseStore(QObject *parent)
    : QObject(parent)
    , m_fts(false)
{
}

QString CaseStore::rootDir() const { return m_root; }

QString CaseStore::folderNameFor(const QDateTime &time, const QString &uuid)
{
    const QString stamp = time.toString(QStringLiteral("yyyyMMddTHHmmss"));
    QString compact = uuid;
    compact.remove(QLatin1Char('-'));
    return stamp + QLatin1Char('_') + compact.left(12);
}

QByteArray CaseStore::hashOf(const QByteArray &bytes)
{
    return QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex();
}

QString CaseStore::ftsText(const QString &text)
{
    QString out;
    const QString trimmed = text.simplified();
    for (int i = 0; i < trimmed.size(); ++i) {
        const QChar ch = trimmed.at(i);
        if (ch.isSpace())
            continue;
        if (!out.isEmpty())
            out.append(QLatin1Char(' '));
        out.append(ch);
    }
    return out;
}

bool CaseStore::open(const QString &rootDir, QString *error)
{
    close();
    m_root = rootDir;
    QDir dir(rootDir);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        if (error)
            *error = QStringLiteral("无法创建案例目录");
        return false;
    }
    dir.mkpath(QStringLiteral("cases"));
    dir.mkpath(QStringLiteral("trash"));
    m_connection = QStringLiteral("sr-cases-%1").arg(newUuid());
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connection);
    db.setDatabaseName(dir.filePath(QStringLiteral("index.db")));
    if (!db.open()) {
        if (error)
            *error = db.lastError().text();
        return false;
    }
    return ensureSchema(error);
}

void CaseStore::close()
{
    if (m_connection.isEmpty())
        return;
    {
        QSqlDatabase db = QSqlDatabase::database(m_connection);
        db.close();
    }
    QSqlDatabase::removeDatabase(m_connection);
    m_connection.clear();
}

bool CaseStore::ensureSchema(QString *error)
{
    QSqlQuery q(QSqlDatabase::database(m_connection));
    const char *table =
        "CREATE TABLE IF NOT EXISTS cases ("
        "id TEXT PRIMARY KEY,"
        "folder TEXT NOT NULL,"
        "name TEXT,"
        "edition TEXT,"
        "case_number TEXT,"
        "accident_time TEXT,"
        "location TEXT,"
        "parties TEXT,"
        "plates TEXT,"
        "officer TEXT,"
        "created TEXT,"
        "modified TEXT,"
        "deleted INTEGER DEFAULT 0,"
        "content_hash TEXT,"
        "thumb TEXT)";
    if (!q.exec(QLatin1String(table))) {
        if (error)
            *error = q.lastError().text();
        return false;
    }
    m_fts = q.exec(QStringLiteral(
        "CREATE VIRTUAL TABLE IF NOT EXISTS cases_fts USING fts5("
        "case_id UNINDEXED, name, case_number, accident_time, location, parties, plates, officer, "
        "tokenize='unicode61')"));
    if (!m_fts && error)
        *error = QString();
    return true;
}

QVariantMap CaseStore::metaFromJson(const QByteArray &json, QString *folderId) const
{
    const QJsonObject root = QJsonDocument::fromJson(json).object();
    const QJsonObject meta = root.value(QStringLiteral("meta")).toObject();
    QVariantMap map;
    const QString id = root.value(QStringLiteral("id")).toString();
    map.insert(QStringLiteral("id"), id);
    map.insert(QStringLiteral("name"), meta.value(QStringLiteral("name")).toString());
    map.insert(QStringLiteral("edition"), root.value(QStringLiteral("edition")).toString(QStringLiteral("sketch")));
    map.insert(QStringLiteral("caseNumber"), meta.value(QStringLiteral("caseNumber")).toString());
    map.insert(QStringLiteral("accidentTime"), meta.value(QStringLiteral("accidentTime")).toString());
    map.insert(QStringLiteral("location"), meta.value(QStringLiteral("location")).toString());
    map.insert(QStringLiteral("officer"), meta.value(QStringLiteral("officer")).toString());
    map.insert(QStringLiteral("parties"), partyText(meta.value(QStringLiteral("parties")).toArray(), false));
    map.insert(QStringLiteral("plates"), partyText(meta.value(QStringLiteral("parties")).toArray(), true));
    map.insert(QStringLiteral("created"), root.value(QStringLiteral("created")).toString());
    map.insert(QStringLiteral("modified"), root.value(QStringLiteral("modified")).toString());
    if (folderId)
        *folderId = id;
    return map;
}

void CaseStore::upsertIndex(const QVariantMap &meta, const QString &folder, const QString &hash, const QString &thumb)
{
    QSqlDatabase db = QSqlDatabase::database(m_connection);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO cases(id,folder,name,edition,case_number,accident_time,location,parties,plates,officer,created,modified,deleted,content_hash,thumb)"
        " VALUES(?,?,?,?,?,?,?,?,?,?,?,?,0,?,?)"
        " ON CONFLICT(id) DO UPDATE SET folder=excluded.folder,name=excluded.name,edition=excluded.edition,"
        "case_number=excluded.case_number,accident_time=excluded.accident_time,location=excluded.location,"
        "parties=excluded.parties,plates=excluded.plates,officer=excluded.officer,created=excluded.created,"
        "modified=excluded.modified,deleted=0,content_hash=excluded.content_hash,thumb=excluded.thumb"));
    q.addBindValue(meta.value(QStringLiteral("id")).toString());
    q.addBindValue(folder);
    q.addBindValue(meta.value(QStringLiteral("name")).toString());
    q.addBindValue(meta.value(QStringLiteral("edition")).toString());
    q.addBindValue(meta.value(QStringLiteral("caseNumber")).toString());
    q.addBindValue(meta.value(QStringLiteral("accidentTime")).toString());
    q.addBindValue(meta.value(QStringLiteral("location")).toString());
    q.addBindValue(meta.value(QStringLiteral("parties")).toString());
    q.addBindValue(meta.value(QStringLiteral("plates")).toString());
    q.addBindValue(meta.value(QStringLiteral("officer")).toString());
    q.addBindValue(meta.value(QStringLiteral("created")).toString());
    q.addBindValue(meta.value(QStringLiteral("modified")).toString());
    q.addBindValue(hash);
    q.addBindValue(thumb);
    q.exec();

    if (!m_fts)
        return;
    QSqlQuery del(db);
    del.prepare(QStringLiteral("DELETE FROM cases_fts WHERE case_id=?"));
    del.addBindValue(meta.value(QStringLiteral("id")).toString());
    del.exec();
    QSqlQuery ins(db);
    ins.prepare(QStringLiteral(
        "INSERT INTO cases_fts(case_id,name,case_number,accident_time,location,parties,plates,officer) VALUES(?,?,?,?,?,?,?,?)"));
    ins.addBindValue(meta.value(QStringLiteral("id")).toString());
    ins.addBindValue(ftsText(meta.value(QStringLiteral("name")).toString()));
    ins.addBindValue(ftsText(meta.value(QStringLiteral("caseNumber")).toString()));
    ins.addBindValue(ftsText(meta.value(QStringLiteral("accidentTime")).toString()));
    ins.addBindValue(ftsText(meta.value(QStringLiteral("location")).toString()));
    ins.addBindValue(ftsText(meta.value(QStringLiteral("parties")).toString()));
    ins.addBindValue(ftsText(meta.value(QStringLiteral("plates")).toString()));
    ins.addBindValue(ftsText(meta.value(QStringLiteral("officer")).toString()));
    ins.exec();
}

void CaseStore::removeIndex(const QString &id)
{
    QSqlQuery q(QSqlDatabase::database(m_connection));
    q.prepare(QStringLiteral("UPDATE cases SET deleted=1 WHERE id=?"));
    q.addBindValue(id);
    q.exec();
    if (m_fts) {
        QSqlQuery del(QSqlDatabase::database(m_connection));
        del.prepare(QStringLiteral("DELETE FROM cases_fts WHERE case_id=?"));
        del.addBindValue(id);
        del.exec();
    }
}

QString CaseStore::createCase(const QString &name, const QString &edition, const QByteArray &json)
{
    Q_UNUSED(name);
    Q_UNUSED(edition);
    const QString id = QJsonDocument::fromJson(json).object().value(QStringLiteral("id")).toString();
    if (id.isEmpty())
        return QString();
    const QString folder = folderNameFor(QDateTime::currentDateTime(), id);
    const QString path = QDir(m_root).filePath(QStringLiteral("cases/") + folder);
    QDir().mkpath(path + QStringLiteral("/photos"));
    QDir().mkpath(path + QStringLiteral("/aerial"));
    QSaveFile file(path + QStringLiteral("/case.json"));
    if (!file.open(QIODevice::WriteOnly))
        return QString();
    file.write(json);
    if (!file.commit())
        return QString();
    QSqlDatabase db = QSqlDatabase::database(m_connection);
    db.transaction();
    upsertIndex(metaFromJson(json), folder, QString::fromLatin1(hashOf(json)), QString());
    if (!db.commit())
        db.rollback();
    emit casesChanged();
    return id;
}

bool CaseStore::saveCase(const QString &id, const QByteArray &json, const QImage &thumb)
{
    const QString folder = caseFolder(id);
    if (folder.isEmpty())
        return false;
    const QString dir = QDir(m_root).filePath(QStringLiteral("cases/") + folder);
    QSaveFile file(dir + QStringLiteral("/case.json"));
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(json);
    if (!file.commit())
        return false;
    QString thumbRel;
    if (!thumb.isNull()) {
        thumbRel = QStringLiteral("thumb.png");
        thumb.save(dir + QStringLiteral("/thumb.png"), "PNG");
    }
    QSqlDatabase db = QSqlDatabase::database(m_connection);
    db.transaction();
    upsertIndex(metaFromJson(json), folder, QString::fromLatin1(hashOf(json)), thumbRel);
    const bool ok = db.commit();
    if (!ok)
        db.rollback();
    emit casesChanged();
    return ok;
}

QByteArray CaseStore::loadCase(const QString &id, QString *error) const
{
    const QString folder = caseFolder(id);
    if (folder.isEmpty()) {
        if (error)
            *error = QStringLiteral("案例不存在");
        return QByteArray();
    }
    QFile file(QDir(m_root).filePath(QStringLiteral("cases/") + folder + QStringLiteral("/case.json")));
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QStringLiteral("无法读取案例");
        return QByteArray();
    }
    return file.readAll();
}

QString CaseStore::caseFolder(const QString &id) const
{
    QSqlQuery q(QSqlDatabase::database(m_connection));
    q.prepare(QStringLiteral("SELECT folder FROM cases WHERE id=? AND deleted=0"));
    q.addBindValue(id);
    if (!q.exec() || !q.next())
        return QString();
    return q.value(0).toString();
}

bool CaseStore::duplicateCase(const QString &id, QString *newId, QString *error)
{
    const QByteArray bytes = loadCase(id, error);
    if (bytes.isEmpty())
        return false;
    QJsonObject root = QJsonDocument::fromJson(bytes).object();
    const QString nid = newUuid();
    root.insert(QStringLiteral("id"), nid);
    QJsonObject meta = root.value(QStringLiteral("meta")).toObject();
    meta.insert(QStringLiteral("name"), meta.value(QStringLiteral("name")).toString() + QStringLiteral(" 副本"));
    root.insert(QStringLiteral("meta"), meta);
    root.insert(QStringLiteral("created"), QDateTime::currentDateTime().toString(Qt::ISODate));
    root.insert(QStringLiteral("modified"), root.value(QStringLiteral("created")));
    const QString created = createCase(meta.value(QStringLiteral("name")).toString(),
                                        root.value(QStringLiteral("edition")).toString(),
                                        QJsonDocument(root).toJson(QJsonDocument::Compact));
    if (created.isEmpty()) {
        if (error)
            *error = QStringLiteral("复制失败");
        return false;
    }
    const QString src = QDir(m_root).filePath(QStringLiteral("cases/") + caseFolder(id));
    const QString dst = QDir(m_root).filePath(QStringLiteral("cases/") + caseFolder(created));
    QDir srcDir(src);
    const QStringList entries = srcDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (int i = 0; i < entries.size(); ++i) {
        if (entries.at(i) == QLatin1String("case.json") || entries.at(i) == QLatin1String("thumb.png"))
            continue;
        const QString from = srcDir.filePath(entries.at(i));
        const QString to = QDir(dst).filePath(entries.at(i));
        if (QFileInfo(from).isDir()) {
            QDir().mkpath(to);
            const QStringList files = QDir(from).entryList(QDir::Files);
            for (int k = 0; k < files.size(); ++k)
                QFile::copy(QDir(from).filePath(files.at(k)), QDir(to).filePath(files.at(k)));
        }
    }
    if (newId)
        *newId = created;
    return true;
}

bool CaseStore::deleteCase(const QString &id, QString *error)
{
    const QString folder = caseFolder(id);
    if (folder.isEmpty()) {
        if (error)
            *error = QStringLiteral("案例不存在");
        return false;
    }
    const QString src = QDir(m_root).filePath(QStringLiteral("cases/") + folder);
    const QString dst = QDir(m_root).filePath(QStringLiteral("trash/") + folder);
    QDir().mkpath(QDir(m_root).filePath(QStringLiteral("trash")));
    if (QDir(dst).exists())
        QDir(dst).removeRecursively();
    if (!QDir().rename(src, dst)) {
        if (error)
            *error = QStringLiteral("无法移入回收站");
        return false;
    }
    QSqlDatabase db = QSqlDatabase::database(m_connection);
    db.transaction();
    removeIndex(id);
    if (!db.commit())
        db.rollback();
    emit casesChanged();
    return true;
}

int CaseStore::rebuildIndex(QString *error)
{
    Q_UNUSED(error);
    QSqlQuery q(QSqlDatabase::database(m_connection));
    q.exec(QStringLiteral("DELETE FROM cases"));
    if (m_fts)
        q.exec(QStringLiteral("DELETE FROM cases_fts"));
    int count = 0;
    const QFileInfoList dirs = QDir(QDir(m_root).filePath(QStringLiteral("cases"))).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (int i = 0; i < dirs.size(); ++i) {
        QFile file(dirs.at(i).filePath() + QStringLiteral("/case.json"));
        if (!file.open(QIODevice::ReadOnly))
            continue;
        const QByteArray bytes = file.readAll();
        const QVariantMap meta = metaFromJson(bytes);
        if (meta.value(QStringLiteral("id")).toString().isEmpty())
            continue;
        const QString thumb = QFileInfo::exists(dirs.at(i).filePath() + QStringLiteral("/thumb.png"))
                ? QStringLiteral("thumb.png") : QString();
        upsertIndex(meta, dirs.at(i).fileName(), QString::fromLatin1(hashOf(bytes)), thumb);
        ++count;
    }
    emit casesChanged();
    return count;
}

QVariantList CaseStore::search(const QString &query, const QString &edition) const
{
    QVariantList rows;
    QSqlQuery q(QSqlDatabase::database(m_connection));
    const QString needle = query.trimmed();
    if (!needle.isEmpty() && m_fts) {
        QStringList terms;
        const QString compact = needle.simplified();
        for (int i = 0; i < compact.size(); ++i) {
            if (!compact.at(i).isSpace())
                terms << compact.at(i);
        }
        const QString match = terms.join(QStringLiteral(" AND "));
        QString sql = QStringLiteral(
            "SELECT c.id,c.name,c.edition,c.case_number,c.accident_time,c.location,c.parties,c.plates,c.officer,c.modified,c.folder "
            "FROM cases_fts f JOIN cases c ON c.id=f.case_id WHERE c.deleted=0 AND f MATCH ?");
        if (!edition.isEmpty())
            sql += QStringLiteral(" AND c.edition=?");
        sql += QStringLiteral(" ORDER BY c.modified DESC LIMIT 200");
        q.prepare(sql);
        q.addBindValue(match);
        if (!edition.isEmpty())
            q.addBindValue(edition);
        if (!q.exec()) {
            q.clear();
        } else {
            while (q.next()) {
                QVariantMap row;
                row.insert(QStringLiteral("id"), q.value(0).toString());
                row.insert(QStringLiteral("name"), q.value(1).toString());
                row.insert(QStringLiteral("edition"), q.value(2).toString());
                row.insert(QStringLiteral("caseNumber"), q.value(3).toString());
                row.insert(QStringLiteral("accidentTime"), q.value(4).toString());
                row.insert(QStringLiteral("location"), q.value(5).toString());
                row.insert(QStringLiteral("parties"), q.value(6).toString());
                row.insert(QStringLiteral("plates"), q.value(7).toString());
                row.insert(QStringLiteral("officer"), q.value(8).toString());
                row.insert(QStringLiteral("modified"), q.value(9).toString());
                rows.append(row);
            }
            return rows;
        }
    }
    QString sql = QStringLiteral(
        "SELECT id,name,edition,case_number,accident_time,location,parties,plates,officer,modified FROM cases WHERE deleted=0");
    if (!needle.isEmpty())
        sql += QStringLiteral(" AND (name LIKE ? OR case_number LIKE ? OR location LIKE ? OR parties LIKE ? OR plates LIKE ? OR officer LIKE ? OR accident_time LIKE ?)");
    if (!edition.isEmpty())
        sql += QStringLiteral(" AND edition=?");
    sql += QStringLiteral(" ORDER BY modified DESC LIMIT 200");
    q.prepare(sql);
    if (!needle.isEmpty()) {
        const QString like = QStringLiteral("%") + needle + QStringLiteral("%");
        for (int i = 0; i < 7; ++i)
            q.addBindValue(like);
    }
    if (!edition.isEmpty())
        q.addBindValue(edition);
    q.exec();
    while (q.next()) {
        QVariantMap row;
        row.insert(QStringLiteral("id"), q.value(0).toString());
        row.insert(QStringLiteral("name"), q.value(1).toString());
        row.insert(QStringLiteral("edition"), q.value(2).toString());
        row.insert(QStringLiteral("caseNumber"), q.value(3).toString());
        row.insert(QStringLiteral("accidentTime"), q.value(4).toString());
        row.insert(QStringLiteral("location"), q.value(5).toString());
        row.insert(QStringLiteral("parties"), q.value(6).toString());
        row.insert(QStringLiteral("plates"), q.value(7).toString());
        row.insert(QStringLiteral("officer"), q.value(8).toString());
        row.insert(QStringLiteral("modified"), q.value(9).toString());
        rows.append(row);
    }
    return rows;
}

QVariantMap CaseStore::summary(const QString &id) const
{
    const QVariantList rows = search(QString());
    for (int i = 0; i < rows.size(); ++i) {
        const QVariantMap row = rows.at(i).toMap();
        if (row.value(QStringLiteral("id")).toString() == id)
            return row;
    }
    return QVariantMap();
}

} // namespace sr
