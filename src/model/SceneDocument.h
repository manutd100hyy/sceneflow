#ifndef SR_SCENEDOCUMENT_H
#define SR_SCENEDOCUMENT_H

#include "Catalog.h"
#include "Geometry.h"
#include "SceneObject.h"

#include <QHash>
#include <QObject>
#include <QStringList>
#include <QVariantMap>
#include <QVector>

namespace sr {

class Catalog;

class Command {
public:
    virtual ~Command() {}
    virtual void undo(class SceneDocument *doc) = 0;
    virtual void redo(class SceneDocument *doc) = 0;
};

struct AerialLayer {
    QString file;
    double opacity;
    double mpp;
    double originX;
    double originY;
    double rotation;
    bool calibrated;
    bool hasRefs;
    double refAx, refAy, refBx, refBy;
    double knownMetres;
    int imageWidth;
    int imageHeight;
    AerialLayer();
    AerialPose pose() const;
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &obj);
};

class SceneDocument : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged)
    Q_PROPERTY(int objectCount READ objectCount NOTIFY changed)
    Q_PROPERTY(QString selectionId READ selectionId NOTIFY selectionChanged)
    Q_PROPERTY(double gridMetres READ gridMetres WRITE setGridMetres NOTIFY viewChanged)
    Q_PROPERTY(bool snapEnabled READ snapEnabled WRITE setSnapEnabled NOTIFY viewChanged)
    Q_PROPERTY(bool gridVisible READ gridVisible WRITE setGridVisible NOTIFY viewChanged)
    Q_PROPERTY(double zoom READ zoom WRITE setZoom NOTIFY viewChanged)
    Q_PROPERTY(double pixelsPerMeter READ pixelsPerMeter NOTIFY viewChanged)
    Q_PROPERTY(double paperWidthM READ paperWidthM WRITE setPaperWidthM NOTIFY viewChanged)
    Q_PROPERTY(double paperHeightM READ paperHeightM WRITE setPaperHeightM NOTIFY viewChanged)
    Q_PROPERTY(double panX READ panX WRITE setPanX NOTIFY viewChanged)
    Q_PROPERTY(double panY READ panY WRITE setPanY NOTIFY viewChanged)
public:
    explicit SceneDocument(QObject *parent = 0);
    ~SceneDocument();

    const QVector<SceneObject> &objects() const;
    int objectCount() const;
    SceneObject *object(const QString &id);
    const SceneObject *object(const QString &id) const;

    bool canUndo() const;
    bool canRedo() const;
    void undo();
    void redo();
    void clearHistory();

    QString selectionId() const;
    void setSelection(const QString &id);
    QVariantMap selectionSummary(const Catalog *catalog) const;
    QVariantList objectList() const;
    bool setProperty(const QString &key, const QVariant &value, const Catalog *catalog);

    QString addObject(const SceneObject &obj);
    void addObjects(const QVector<SceneObject> &objs, const QString &label);
    void removeIds(const QStringList &ids);
    QString duplicateSelection();
    void removeSelection();

    void beginDrag(const QStringList &seedIds);
    void dragTo(const QPointF &totalDelta);
    void endDrag();
    void cancelDrag();
    bool dragActive() const;

    QString hitTest(const QPointF &world, double tol, const Catalog *catalog) const;
    QRectF contentBounds(const Catalog *catalog) const;
    QPointF snapPoint(const QPointF &world, const Catalog *catalog, const QString &ignoreId = QString()) const;

    QString placeTemplate(const TemplateDef &tpl, const QPointF &anchor);
    QString filletJunctions(double radius);
    QString placeSymbol(const Catalog &catalog, const QString &name, const QPointF &at);
    void addRoad(const QVector<QPointF> &center, int lanes, double laneWidth);
    void addCircleRoad(const QPointF &center, double radius, int lanes, double laneWidth);
    void addTrace(const QVector<QPointF> &pts, int type);
    void addDebris(const QVector<QPointF> &pts, int variant);
    void addDimension(const QPointF &a, const QPointF &b, int style, const QString &fromId, const QString &toId);
    void addText(const QPointF &at, const QString &text);
    void addCrosswalk(const QPointF &a, const QPointF &b, double width);
    void addGuide(const QPointF &a, const QPointF &b);
    void addParking(const QPointF &at, double length, double width, int stalls, double rotation);
    void ensureCompass();
    void parkCompass(const Catalog *catalog);
    void clearDrawings();

    QString proportionalize();
    QString scaleToMeasures();

    double gridMetres() const;
    void setGridMetres(double metres);
    bool snapEnabled() const;
    void setSnapEnabled(bool on);
    bool gridVisible() const;
    void setGridVisible(bool on);
    // zoom 是相对 20 像素/米的倍率：1 表示 100%。
    double zoom() const;
    void setZoom(double z);
    double pixelsPerMeter() const;
    double paperWidthM() const;
    void setPaperWidthM(double metres);
    double paperHeightM() const;
    void setPaperHeightM(double metres);
    QRectF paperWorldRect() const;
    double panX() const;
    void setPanX(double v);
    double panY() const;
    void setPanY(double v);
    bool dragGroups() const;
    void setDragGroups(bool on);
    void fitView(double viewWidth, double viewHeight, const Catalog *catalog);
    void clampPan(double viewWidth, double viewHeight);

    AerialLayer aerial() const;
    void setAerial(const AerialLayer &layer);
    QString calibrateAerial(double knownMetres);

    QJsonObject toJson() const;
    void fromJson(const QJsonObject &obj);

    void commitReplace(const SceneObject &before);
    void insertObjectRaw(const SceneObject &obj);
    void removeObjectRaw(const QString &id);
    void replaceObjectRaw(const SceneObject &obj);
    void restoreRaw(const QVector<SceneObject> &objects, const AerialLayer &aerial);

signals:
    void changed();
    void selectionChanged();
    void historyChanged();
    void viewChanged();
    void messageRaised(const QString &text);

private:
    void pushCommand(Command *command, bool alreadyApplied);
    void rebuildIndex();
    QString autoLabel(int numberType) const;
    QVector<SceneObject> captureObjects() const;
    void scaleEverything(const QPointF &origin, double factor);

    QVector<SceneObject> m_objects;
    QHash<QString, int> m_index;
    QString m_selection;
    QVector<Command *> m_undo;
    QVector<Command *> m_redo;
    double m_grid;
    bool m_snap;
    bool m_gridVisible;
    double m_zoom;
    double m_paperWidthM;
    double m_paperHeightM;
    double m_panX;
    double m_panY;
    bool m_dragGroups;
    bool m_dragActive;
    QPointF m_dragTotal;
    QVector<SceneObject> m_dragBefore;
    QStringList m_dragIds;
    AerialLayer m_aerial;
};

// 标尺刻度：在期望的像素间距上取 1、2、5、10 乘以 10 的幂。
double niceMeterStep(double pixelsPerMeter, double desiredPixels);
inline double basePixelsPerMeter() { return 20.0; }

} // namespace sr

#endif
