#ifndef SR_SCENEPAINTER_H
#define SR_SCENEPAINTER_H

#include <QString>

class QPainter;
class QImage;

namespace sr {

class Catalog;
class SceneDocument;

struct PaintOptions {
    bool grid;
    bool forPrint;
    QString selectedId;
    const QImage *aerial;
    PaintOptions();
};

void paintScene(QPainter &painter, const SceneDocument &document, const Catalog &catalog, const PaintOptions &options);

} // namespace sr

#endif
