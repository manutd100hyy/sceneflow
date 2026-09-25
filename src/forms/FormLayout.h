#ifndef SR_FORMLAYOUT_H
#define SR_FORMLAYOUT_H

#include <QHash>
#include <QString>
#include <QVariantMap>
#include <QVector>

class QPainter;
class QSvgRenderer;

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
    FormLayout();
    ~FormLayout();

    bool load(const QString &dataDir);
    const QVector<FormDef> &forms() const;
    const FormDef *find(const QString &id) const;
    void paintPage(QPainter &painter, const FormPage &page, const QVariantMap &values) const;

private:
    QSvgRenderer *svgRenderer(const QString &path) const;

    QVector<FormDef> m_forms;
    mutable QHash<QString, QSvgRenderer *> m_svg;
};

} // namespace sr

#endif
