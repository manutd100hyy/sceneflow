#ifndef SR_APPCONTROLLER_H
#define SR_APPCONTROLLER_H

#include "forms/FormLayout.h"
#include "io/CaseStore.h"
#include "model/Catalog.h"
#include "model/SceneDocument.h"

#include <QHash>
#include <QImage>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

class QQuickWindow;

namespace sr {

class SceneCanvas;

class AppController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString screen READ screen NOTIFY screenChanged)
    Q_PROPERTY(QString edition READ edition WRITE setEdition NOTIFY updated)
    Q_PROPERTY(QString caseName READ caseName NOTIFY updated)
    Q_PROPERTY(QString caseId READ caseId NOTIFY updated)
    Q_PROPERTY(bool dirty READ dirty NOTIFY updated)
    Q_PROPERTY(QString statusText READ statusText NOTIFY updated)
    Q_PROPERTY(int activeTab READ activeTab WRITE setActiveTab NOTIFY updated)
    Q_PROPERTY(int tool READ tool WRITE setTool NOTIFY updated)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY updated)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY updated)
    Q_PROPERTY(QString zoomText READ zoomText NOTIFY updated)
    Q_PROPERTY(QString hint READ hint NOTIFY updated)
    Q_PROPERTY(QVariantMap selection READ selection NOTIFY selectionChanged)
    Q_PROPERTY(QVariantList objects READ objects NOTIFY updated)
    Q_PROPERTY(QVariantList templates READ templates CONSTANT)
    Q_PROPERTY(QVariantList groups READ groups CONSTANT)
    Q_PROPERTY(QVariantList cases READ cases NOTIFY updated)
    Q_PROPERTY(bool showCases READ showCases WRITE setShowCases NOTIFY updated)
    Q_PROPERTY(int laneCount READ laneCount WRITE setLaneCount NOTIFY updated)
    Q_PROPERTY(double laneWidth READ laneWidth WRITE setLaneWidth NOTIFY updated)
    Q_PROPERTY(double gridMetres READ gridMetres WRITE setGridMetres NOTIFY updated)
    Q_PROPERTY(bool snapOn READ snapOn WRITE setSnapOn NOTIFY updated)
    Q_PROPERTY(QString message READ message NOTIFY updated)
    Q_PROPERTY(QString formId READ formId WRITE setFormId NOTIFY updated)
    Q_PROPERTY(int formPage READ formPage WRITE setFormPage NOTIFY updated)
    Q_PROPERTY(int formRevision READ formRevision NOTIFY updated)
    Q_PROPERTY(QVariantMap meta READ meta NOTIFY updated)
    Q_PROPERTY(QVariantList parties READ parties NOTIFY updated)
    Q_PROPERTY(QVariantList photos READ photos NOTIFY updated)
    Q_PROPERTY(QVariantList records READ records NOTIFY updated)
    Q_PROPERTY(QString dataPath READ dataPath NOTIFY updated)
    Q_PROPERTY(bool hasAerial READ hasAerial NOTIFY updated)
    Q_PROPERTY(QString aerialStatus READ aerialStatus NOTIFY updated)
public:
    explicit AppController(QObject *parent = 0);
    bool start(QString *error = 0);

    QString screen() const;
    QString edition() const;
    void setEdition(const QString &edition);
    QString caseName() const;
    QString caseId() const;
    bool dirty() const;
    QString statusText() const;
    int activeTab() const;
    void setActiveTab(int tab);
    int tool() const;
    void setTool(int tool);
    bool canUndo() const;
    bool canRedo() const;
    QString zoomText() const;
    QString hint() const;
    QVariantMap selection() const;
    QVariantList objects() const;
    QVariantList templates() const;
    QVariantList groups() const;
    QVariantList cases() const;
    bool showCases() const;
    void setShowCases(bool on);
    int laneCount() const;
    void setLaneCount(int count);
    double laneWidth() const;
    void setLaneWidth(double metres);
    double gridMetres() const;
    void setGridMetres(double metres);
    bool snapOn() const;
    void setSnapOn(bool on);
    QString message() const;
    QString formId() const;
    void setFormId(const QString &id);
    int formPage() const;
    void setFormPage(int page);
    int formRevision() const;
    QVariantMap meta() const;
    QVariantList parties() const;
    QVariantList photos() const;
    QVariantList records() const;
    QString dataPath() const;
    bool hasAerial() const;
    QString aerialStatus() const;

    Q_INVOKABLE void attachCanvas(QObject *canvas);
    Q_INVOKABLE void newCase(const QString &name, const QString &templateName);
    Q_INVOKABLE void openSample();
    Q_INVOKABLE void openAerialSample();
    Q_INVOKABLE void openCase(const QString &id);
    Q_INVOKABLE bool save();
    Q_INVOKABLE void goHome();
    Q_INVOKABLE void refreshCases(const QString &query);
    Q_INVOKABLE bool deleteCase(const QString &id);
    Q_INVOKABLE bool duplicateCase(const QString &id);
    Q_INVOKABLE void placeTemplate(const QString &id);
    Q_INVOKABLE void activateLibrary(const QString &name, const QString &notification, int flag, const QString &uuid);
    Q_INVOKABLE QVariantList searchLibrary(const QString &query, int groupIndex) const;
    Q_INVOKABLE void openSymbolLibrary();
    Q_INVOKABLE void closeSymbolLibrary();
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void removeSelection();
    Q_INVOKABLE void duplicateSelection();
    Q_INVOKABLE void selectObject(const QString &id);
    Q_INVOKABLE void setProp(const QString &key, const QVariant &value);
    Q_INVOKABLE void setMetaField(const QString &key, const QString &value);
    Q_INVOKABLE void setParty(int index, const QString &key, const QString &value);
    Q_INVOKABLE void addParty();
    Q_INVOKABLE void importPhoto(const QString &path, const QString &caption);
    Q_INVOKABLE void addRecord(const QString &text);
    Q_INVOKABLE void removePhoto(const QString &id);
    Q_INVOKABLE QVariantList formCatalog() const;
    Q_INVOKABLE QVariantList currentFields() const;
    Q_INVOKABLE void setFormValue(const QString &key, const QString &value);
    Q_INVOKABLE bool exportScene(const QString &path, const QString &paper, bool landscape, int scaleDenom);
    Q_INVOKABLE bool exportForm(const QString &path, const QString &paper);
    Q_INVOKABLE void importAerial(const QString &path);
    Q_INVOKABLE QString confirmCalibration(double metres);
    Q_INVOKABLE void clearDrawings();
    Q_INVOKABLE QString proportionalize();
    Q_INVOKABLE QString scaleToMeasures();
    Q_INVOKABLE void fit();
    Q_INVOKABLE int rebuildIndex();
    Q_INVOKABLE void addTextAt(double x, double y, const QString &text);
    Q_INVOKABLE void setOpacity(double opacity);
    QImage renderFormPreview(const QString &id, QSize *size) const;
    void runCapture(QQuickWindow *window, const QString &directory);

signals:
    void updated();
    void screenChanged();
    void selectionChanged();
    void textRequested(double x, double y);
    void symbolLibraryRequested();
    void symbolLibraryClosed();

private:
    void enterScreen(const QString &name);
    void bindDocument();
    void markDirty();
    void touchStatus();
    QByteArray buildJson() const;
    void loadJson(const QByteArray &bytes);
    void resetCase(const QString &name, const QString &edition);
    void syncFormsFromMeta();
    QString absoluteCasePath(const QString &relative) const;
    void copyIntoCase(const QString &source, const QString &relative);
    void rememberAerialImage();

    Catalog m_catalog;
    FormLayout m_forms;
    CaseStore m_store;
    SceneDocument m_document;
    SceneCanvas *m_canvas;
    QTimer m_autosave;
    QString m_screen;
    QString m_edition;
    QString m_id;
    QString m_created;
    bool m_dirty;
    bool m_loading;
    int m_tab;
    int m_tool;
    QString m_hint;
    QString m_message;
    bool m_showCases;
    int m_lanes;
    double m_laneWidth;
    QString m_formId;
    int m_formPage;
    int m_formRevision;
    QVariantMap m_meta;
    QVariantList m_parties;
    QVariantList m_photos;
    QVariantList m_records;
    QHash<QString, QVariantMap> m_formValues;
    QVariantList m_cases;
    QString m_query;
    QImage m_aerialImage;
};

} // namespace sr

#endif
