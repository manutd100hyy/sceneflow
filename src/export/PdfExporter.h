#ifndef SR_PDFEXPORTER_H
#define SR_PDFEXPORTER_H

#include <QString>
#include <QVariantMap>

class QImage;

namespace sr {

class Catalog;
class FormLayout;
class SceneDocument;

struct SceneSheet {
    QString paper;
    bool landscape;
    int scaleDenom;
    QString title;
    QString location;
    QString accidentTime;
    QString weather;
    QString drawer;
    SceneSheet();
};

// 1:denom 时图上每米的毫米数。denom<=0 时返回 0，由导出按内容自适应。
double pdfMillimetresPerMetre(int scaleDenom);

bool exportScenePdf(const QString &path, const SceneDocument &document, const Catalog &catalog,
                    const QImage *aerial, const SceneSheet &sheet, QString *error = 0);
bool exportFormPdf(const QString &path, const FormLayout &layout, const QString &formId,
                   const QVariantMap &values, const QString &paper, QString *error = 0);

} // namespace sr

#endif
