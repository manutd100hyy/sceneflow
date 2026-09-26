#ifndef SR_CASESTORE_H
#define SR_CASESTORE_H

#include <QByteArray>
#include <QDateTime>
#include <QImage>
#include <QObject>
#include <QSqlDatabase>
#include <QVariantList>
#include <QVariantMap>

namespace sr {

class CaseStore : public QObject {
    Q_OBJECT
public:
    explicit CaseStore(QObject *parent = 0);
    bool open(const QString &rootDir, QString *error = 0);
    QString rootDir() const;
    void close();

    QString createCase(const QString &name, const QString &edition, const QByteArray &json);
    bool saveCase(const QString &id, const QByteArray &json, const QImage &thumb);
    QByteArray loadCase(const QString &id, QString *error = 0) const;
    QString caseFolder(const QString &id) const;
    bool duplicateCase(const QString &id, QString *newId, QString *error = 0);
    bool deleteCase(const QString &id, QString *error = 0);
    int rebuildIndex(QString *error = 0);
    QVariantList search(const QString &query, const QString &edition = QString()) const;
    QVariantMap summary(const QString &id) const;

    static QString folderNameFor(const QDateTime &time, const QString &uuid);
    static QByteArray hashOf(const QByteArray &bytes);

signals:
    void casesChanged();

private:
    bool ensureSchema(QString *error);
    void upsertIndex(const QVariantMap &meta, const QString &folder, const QString &hash, const QString &thumb);
    void removeIndex(const QString &id);
    QVariantMap metaFromJson(const QByteArray &json, QString *folderId = 0) const;
    static QString ftsText(const QString &text);

    QString m_root;
    QString m_connection;
    bool m_fts;
};

} // namespace sr

#endif
