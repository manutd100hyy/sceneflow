#ifndef SR_FORMLAYOUT_H
#define SR_FORMLAYOUT_H

#include <QString>
#include <QVariantMap>
#include <QVector>

class QPainter;

namespace sr {

struct FormField {
    QString key;
    QString label;
    QString kind;
    double x, y, w, h;
    int fontSize;
    QStringList options;
};

struct FormPage {
    QString title;
    QString imagePath;
    double width;
    double height;
    QVector<FormField> fields;
};

struct FormDef {
    QString id;
    QString title;
    QVector<FormPage> pages;
};

class FormLayout {
public:
    bool load(const QString &dataDir);
    const QVector<FormDef> &forms() const;
    const FormDef *find(const QString &id) const;
    void paintPage(QPainter &painter, const FormPage &page, const QVariantMap &values) const;

private:
    QVector<FormDef> m_forms;
};

} // namespace sr

#endif
