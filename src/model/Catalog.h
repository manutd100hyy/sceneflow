#ifndef SR_CATALOG_H
#define SR_CATALOG_H

#include <QHash>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVariantList>
#include <QVector>

namespace sr {

struct Prim {
    int k;
    QVector<QPointF> pts;
    double x, y, w, h, r, a0, a1, cr;
    bool filled;
    bool cw;
    Prim();
};

struct SymbolStyle {
    QString name;
    QVector<Prim> prims;
    QVector<QPointF> skel;
    QVector<QPointF> driving;
    QVector<QString> drivingNames;
};

struct SymbolDef {
    QString name;
    QString uuid;
    int legacyKind;
    int numberType;
    int priority;
    QString fill;
    QString line;
    QVector<SymbolStyle> styles;
};

struct TplLine {
    int style;
    bool curve;
    QVector<QPointF> pts;
    TplLine() : style(1), curve(false) {}
};

struct TplMark {
    QString type;
    QVector<QPointF> pts;
    double width;
    double interval;
    double x, y, w, h, ang;
    int count;
    TplMark();
};

struct TemplateDef {
    QString id;
    QString name;
    QVector<TplLine> lines;
    QVector<TplMark> marks;
    QRectF bounds() const;
};

class Catalog {
public:
    Catalog();
    bool load(const QString &dataDir, QString *error = 0);
    QString dataDir() const;
    const SymbolDef *symbol(const QString &name) const;
    const TemplateDef *templateById(const QString &id) const;
    QString templateIdForName(const QString &name) const;
    QRectF naturalBounds(const QString &name, int styleIndex) const;
    QVector<QPointF> drivingPoints(const QString &name, int styleIndex) const;
    QStringList styleNames(const QString &name) const;
    int symbolCount() const;
    int templateCount() const;
    const QVector<SymbolDef> &symbols() const;
    const QVector<TemplateDef> &templates() const;
    QVariantList templateList() const;
    QVariantList libraryGroups() const;

private:
    const SymbolStyle *styleOf(const QString &name, int styleIndex) const;
    QString m_dir;
    QVector<SymbolDef> m_symbols;
    QHash<QString, int> m_symIndex;
    QVector<TemplateDef> m_templates;
    QHash<QString, int> m_tplIndex;
    QVariantList m_tplList;
    QVariantList m_groups;
};

} // namespace sr

#endif
