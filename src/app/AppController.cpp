#include "AppController.h"

#include "export/PdfExporter.h"
#include "model/Geometry.h"
#include "render/SceneCanvas.h"
#include "render/ScenePainter.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <QQuickWindow>
#include <QStandardPaths>

namespace sr {

static QVariantMap partyMap(const QString &name = QString(), const QString &plate = QString())
{
    QVariantMap row;
    row.insert(QStringLiteral("name"), name);
    row.insert(QStringLiteral("plate"), plate);
    row.insert(QStringLiteral("phone"), QString());
    row.insert(QStringLiteral("idNo"), QString());
    row.insert(QStringLiteral("vehicle"), QString());
    return row;
}

AppController::AppController(QObject *parent)
    : QObject(parent)
    , m_canvas(0)
    , m_screen(QStringLiteral("home"))
    , m_edition(QStringLiteral("sketch"))
    , m_dirty(false)
    , m_loading(false)
    , m_tab(1)
    , m_tool(0)
    , m_hint(QStringLiteral("从左侧选择道路或图符，开始绘制"))
    , m_showCases(false)
    , m_lanes(2)
    , m_laneWidth(3.5)
    , m_formId(QStringLiteral("survey"))
    , m_formPage(0)
    , m_formRevision(1)
{
    m_autosave.setInterval(5000);
    connect(&m_autosave, &QTimer::timeout, this, [this]() {
        if (m_dirty && !m_id.isEmpty())
            save();
    });
    bindDocument();
}

void AppController::bindDocument()
{
    connect(&m_document, &SceneDocument::changed, this, [this]() {
        if (!m_loading)
            markDirty();
        emit selectionChanged();
        emit updated();
    });
    connect(&m_document, &SceneDocument::selectionChanged, this, [this]() {
        emit selectionChanged();
        emit updated();
    });
    connect(&m_document, &SceneDocument::historyChanged, this, [this]() { emit updated(); });
    connect(&m_document, &SceneDocument::viewChanged, this, [this]() { emit updated(); });
    connect(&m_document, &SceneDocument::messageRaised, this, [this](const QString &text) {
        m_message = text;
        emit updated();
    });
}

bool AppController::start(QString *error)
{
    const QString data = locateDataDir();
    if (!m_catalog.load(data, error))
        return false;
    m_forms.load(data);
    QString root = QString::fromLocal8Bit(qgetenv("SKETCHROAD_HOME"));
    if (root.isEmpty()) {
        root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (root.isEmpty())
            root = QDir::home().filePath(QStringLiteral("SketchRoad"));
    }
    if (!m_store.open(root, error))
        return false;
    refreshCases(QString());
    m_autosave.start();
    emit updated();
    return true;
}

QString AppController::screen() const { return m_screen; }

void AppController::enterScreen(const QString &name)
{
    if (m_screen == name)
        return;
    m_screen = name;
    emit screenChanged();
}
QString AppController::edition() const { return m_edition; }
QString AppController::caseName() const { return m_meta.value(QStringLiteral("name")).toString(); }
QString AppController::caseId() const { return m_id; }
bool AppController::dirty() const { return m_dirty; }
int AppController::activeTab() const { return m_tab; }
int AppController::tool() const { return m_tool; }
bool AppController::canUndo() const { return m_document.canUndo(); }
bool AppController::canRedo() const { return m_document.canRedo(); }
QString AppController::zoomText() const { return QString::number(int(m_document.zoom() / 8.0 * 100)) + QStringLiteral("%"); }
QString AppController::hint() const { return m_hint; }
QVariantMap AppController::selection() const { return m_document.selectionSummary(&m_catalog); }
QVariantList AppController::objects() const { return m_document.objectList(); }
QVariantList AppController::templates() const { return m_catalog.templateList(); }
QVariantList AppController::groups() const { return m_catalog.libraryGroups(); }
QVariantList AppController::cases() const { return m_cases; }
bool AppController::showCases() const { return m_showCases; }
int AppController::laneCount() const { return m_lanes; }
double AppController::laneWidth() const { return m_laneWidth; }
double AppController::gridMetres() const { return m_document.gridMetres(); }
bool AppController::snapOn() const { return m_document.snapEnabled(); }
QString AppController::message() const { return m_message; }
QString AppController::formId() const { return m_formId; }
int AppController::formPage() const { return m_formPage; }
int AppController::formRevision() const { return m_formRevision; }
QVariantMap AppController::meta() const { return m_meta; }
QVariantList AppController::parties() const { return m_parties; }
QVariantList AppController::photos() const { return m_photos; }
QVariantList AppController::records() const { return m_records; }
QString AppController::dataPath() const { return m_store.rootDir(); }
bool AppController::hasAerial() const { return !m_document.aerial().file.isEmpty(); }

QString AppController::statusText() const
{
    if (m_id.isEmpty())
        return QStringLiteral("未打开案例");
    return (m_dirty ? QStringLiteral("未保存") : QStringLiteral("已保存"))
            + QStringLiteral(" · 网格 ") + QString::number(m_document.gridMetres())
            + QStringLiteral(" m · ") + (m_document.snapEnabled() ? QStringLiteral("吸附开") : QStringLiteral("吸附关"));
}

QString AppController::aerialStatus() const
{
    const AerialLayer aerial = m_document.aerial();
    if (aerial.file.isEmpty())
        return QStringLiteral("未导入航拍照片");
    if (!aerial.calibrated)
        return QStringLiteral("未标定 · 1 像素 ≈ %1 米").arg(QString::number(aerial.mpp, 'f', 3));
    return QStringLiteral("已标定 · 1 像素 = %1 米").arg(QString::number(aerial.mpp, 'f', 4));
}

void AppController::setEdition(const QString &edition)
{
    m_edition = edition == QLatin1String("aerial") ? QStringLiteral("aerial") : QStringLiteral("sketch");
    emit updated();
}

void AppController::setActiveTab(int tab)
{
    tab = qBound(0, tab, 3);
    if (m_tab == tab)
        return;
    m_tab = tab;
    emit updated();
}

void AppController::setTool(int tool)
{
    m_tool = tool;
    if (m_canvas)
        m_canvas->setTool(tool);
    emit updated();
}

void AppController::setShowCases(bool on)
{
    if (m_showCases == on)
        return;
    m_showCases = on;
    if (on)
        refreshCases(m_query);
    emit updated();
}

void AppController::setLaneCount(int count)
{
    m_lanes = qBound(1, count, 8);
    if (m_canvas)
        m_canvas->setLaneCount(m_lanes);
    emit updated();
}

void AppController::setLaneWidth(double metres)
{
    m_laneWidth = qBound(2.0, metres, 6.0);
    if (m_canvas)
        m_canvas->setLaneWidth(m_laneWidth);
    emit updated();
}

void AppController::setGridMetres(double metres)
{
    m_document.setGridMetres(metres);
}

void AppController::setSnapOn(bool on)
{
    m_document.setSnapEnabled(on);
}

void AppController::setFormId(const QString &id)
{
    m_formId = id;
    m_formPage = 0;
    emit updated();
}

void AppController::setFormPage(int page)
{
    m_formPage = qMax(0, page);
    emit updated();
}

void AppController::markDirty()
{
    m_dirty = true;
}

void AppController::attachCanvas(QObject *canvas)
{
    m_canvas = qobject_cast<SceneCanvas *>(canvas);
    if (!m_canvas)
        return;
    m_canvas->setCatalog(&m_catalog);
    m_canvas->setDocument(&m_document);
    m_canvas->setLaneCount(m_lanes);
    m_canvas->setLaneWidth(m_laneWidth);
    m_canvas->setTool(m_tool);
    connect(m_canvas, &SceneCanvas::hintChanged, this, [this](const QString &text) {
        m_hint = text;
        emit updated();
    });
    connect(m_canvas, &SceneCanvas::needText, this, [this](double x, double y) {
        emit textRequested(x, y);
    });
    connect(m_canvas, &SceneCanvas::calibrationReady, this, [this]() {
        m_message = QStringLiteral("已记录两个参考点，请填写地面距离");
        emit updated();
    });
    rememberAerialImage();
    fit();
}

QString AppController::absoluteCasePath(const QString &relative) const
{
    const QString folder = m_store.caseFolder(m_id);
    if (folder.isEmpty())
        return QString();
    return QDir(m_store.rootDir()).filePath(QStringLiteral("cases/") + folder + QLatin1Char('/') + relative);
}

void AppController::copyIntoCase(const QString &source, const QString &relative)
{
    const QString dest = absoluteCasePath(relative);
    QDir().mkpath(QFileInfo(dest).absolutePath());
    if (QFile::exists(dest))
        QFile::remove(dest);
    QFile::copy(source, dest);
}

void AppController::rememberAerialImage()
{
    const QString file = m_document.aerial().file;
    m_aerialImage = QImage();
    if (!file.isEmpty())
        m_aerialImage.load(file);
}

void AppController::resetCase(const QString &name, const QString &edition)
{
    m_loading = true;
    m_document.fromJson(QJsonObject());
    m_document.clearHistory();
    m_id = newUuid();
    m_created = QDateTime::currentDateTime().toString(Qt::ISODate);
    m_edition = edition;
    m_meta.clear();
    m_meta.insert(QStringLiteral("name"), name);
    m_meta.insert(QStringLiteral("caseNumber"), QString());
    m_meta.insert(QStringLiteral("accidentTime"), QString());
    m_meta.insert(QStringLiteral("location"), QString());
    m_meta.insert(QStringLiteral("weather"), QStringLiteral("晴"));
    m_meta.insert(QStringLiteral("roadSurface"), QStringLiteral("沥青"));
    m_meta.insert(QStringLiteral("officer"), QString());
    m_meta.insert(QStringLiteral("drawer"), QString());
    m_meta.insert(QStringLiteral("surveyOrg"), QString());
    m_meta.insert(QStringLiteral("notes"), QString());
    m_parties = QVariantList() << partyMap() << partyMap();
    m_photos.clear();
    m_records.clear();
    m_formValues.clear();
    m_formValues.insert(QStringLiteral("survey"), QVariantMap());
    m_formValues.insert(QStringLiteral("inquiry"), QVariantMap());
    m_formValues.insert(QStringLiteral("interrogation"), QVariantMap());
    m_formValues.insert(QStringLiteral("certificate"), QVariantMap());
    m_aerialImage = QImage();
    m_loading = false;
    m_dirty = true;
}

QByteArray AppController::buildJson() const
{
    QJsonObject root;
    root.insert(QStringLiteral("formatVersion"), 1);
    root.insert(QStringLiteral("units"), QStringLiteral("m"));
    root.insert(QStringLiteral("id"), m_id);
    root.insert(QStringLiteral("edition"), m_edition);
    root.insert(QStringLiteral("created"), m_created);
    root.insert(QStringLiteral("modified"), QDateTime::currentDateTime().toString(Qt::ISODate));
    QJsonObject meta;
    const QStringList keys = m_meta.keys();
    for (int i = 0; i < keys.size(); ++i)
        meta.insert(keys.at(i), m_meta.value(keys.at(i)).toString());
    QJsonArray parties;
    for (int i = 0; i < m_parties.size(); ++i) {
        const QVariantMap row = m_parties.at(i).toMap();
        QJsonObject o;
        o.insert(QStringLiteral("name"), row.value(QStringLiteral("name")).toString());
        o.insert(QStringLiteral("plate"), row.value(QStringLiteral("plate")).toString());
        o.insert(QStringLiteral("phone"), row.value(QStringLiteral("phone")).toString());
        o.insert(QStringLiteral("idNo"), row.value(QStringLiteral("idNo")).toString());
        o.insert(QStringLiteral("vehicle"), row.value(QStringLiteral("vehicle")).toString());
        parties.append(o);
    }
    meta.insert(QStringLiteral("parties"), parties);
    root.insert(QStringLiteral("meta"), meta);
    QJsonObject scene = m_document.toJson();
    QJsonObject aerial = scene.value(QStringLiteral("aerial")).toObject();
    const QString aerialFile = aerial.value(QStringLiteral("file")).toString();
    if (!aerialFile.isEmpty())
        aerial.insert(QStringLiteral("file"), QStringLiteral("aerial/") + QFileInfo(aerialFile).fileName());
    scene.insert(QStringLiteral("aerial"), aerial);
    root.insert(QStringLiteral("scene"), scene);
    QJsonArray photos;
    for (int i = 0; i < m_photos.size(); ++i) {
        const QVariantMap row = m_photos.at(i).toMap();
        QJsonObject o;
        o.insert(QStringLiteral("id"), row.value(QStringLiteral("id")).toString());
        o.insert(QStringLiteral("file"), row.value(QStringLiteral("file")).toString());
        o.insert(QStringLiteral("caption"), row.value(QStringLiteral("caption")).toString());
        photos.append(o);
    }
    root.insert(QStringLiteral("photos"), photos);
    QJsonArray records;
    for (int i = 0; i < m_records.size(); ++i) {
        const QVariantMap row = m_records.at(i).toMap();
        QJsonObject o;
        o.insert(QStringLiteral("id"), row.value(QStringLiteral("id")).toString());
        o.insert(QStringLiteral("text"), row.value(QStringLiteral("text")).toString());
        o.insert(QStringLiteral("time"), row.value(QStringLiteral("time")).toString());
        records.append(o);
    }
    root.insert(QStringLiteral("records"), records);
    QJsonObject forms;
    const QStringList formKeys = m_formValues.keys();
    for (int i = 0; i < formKeys.size(); ++i) {
        const QVariantMap values = m_formValues.value(formKeys.at(i));
        QJsonObject o;
        const QStringList keys = values.keys();
        for (int k = 0; k < keys.size(); ++k)
            o.insert(keys.at(k), values.value(keys.at(k)).toString());
        forms.insert(formKeys.at(i), o);
    }
    root.insert(QStringLiteral("forms"), forms);
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

void AppController::loadJson(const QByteArray &bytes)
{
    m_loading = true;
    const QJsonObject root = QJsonDocument::fromJson(bytes).object();
    m_id = root.value(QStringLiteral("id")).toString();
    m_edition = root.value(QStringLiteral("edition")).toString(QStringLiteral("sketch"));
    m_created = root.value(QStringLiteral("created")).toString();
    const QJsonObject meta = root.value(QStringLiteral("meta")).toObject();
    m_meta.clear();
    const QStringList keys = meta.keys();
    for (int i = 0; i < keys.size(); ++i) {
        if (keys.at(i) == QLatin1String("parties"))
            continue;
        m_meta.insert(keys.at(i), meta.value(keys.at(i)).toString());
    }
    m_parties.clear();
    const QJsonArray parties = meta.value(QStringLiteral("parties")).toArray();
    for (int i = 0; i < parties.size(); ++i) {
        const QJsonObject o = parties.at(i).toObject();
        QVariantMap row = partyMap(o.value(QStringLiteral("name")).toString(), o.value(QStringLiteral("plate")).toString());
        row.insert(QStringLiteral("phone"), o.value(QStringLiteral("phone")).toString());
        row.insert(QStringLiteral("idNo"), o.value(QStringLiteral("idNo")).toString());
        row.insert(QStringLiteral("vehicle"), o.value(QStringLiteral("vehicle")).toString());
        m_parties.append(row);
    }
    if (m_parties.isEmpty())
        m_parties << partyMap() << partyMap();
    m_document.fromJson(root.value(QStringLiteral("scene")).toObject());
    const QString aerialFile = m_document.aerial().file;
    if (!aerialFile.isEmpty() && QFileInfo(aerialFile).isRelative()) {
        AerialLayer aerial = m_document.aerial();
        aerial.file = absoluteCasePath(aerialFile);
        m_document.setAerial(aerial);
    }
    rememberAerialImage();
    m_photos.clear();
    const QJsonArray photos = root.value(QStringLiteral("photos")).toArray();
    for (int i = 0; i < photos.size(); ++i) {
        const QJsonObject o = photos.at(i).toObject();
        QVariantMap row;
        row.insert(QStringLiteral("id"), o.value(QStringLiteral("id")).toString());
        row.insert(QStringLiteral("file"), o.value(QStringLiteral("file")).toString());
        row.insert(QStringLiteral("caption"), o.value(QStringLiteral("caption")).toString());
        m_photos.append(row);
    }
    m_records.clear();
    const QJsonArray records = root.value(QStringLiteral("records")).toArray();
    for (int i = 0; i < records.size(); ++i) {
        const QJsonObject o = records.at(i).toObject();
        QVariantMap row;
        row.insert(QStringLiteral("id"), o.value(QStringLiteral("id")).toString());
        row.insert(QStringLiteral("text"), o.value(QStringLiteral("text")).toString());
        row.insert(QStringLiteral("time"), o.value(QStringLiteral("time")).toString());
        m_records.append(row);
    }
    m_formValues.clear();
    const QJsonObject forms = root.value(QStringLiteral("forms")).toObject();
    const QStringList formKeys = forms.keys();
    for (int i = 0; i < formKeys.size(); ++i) {
        QVariantMap values;
        const QJsonObject o = forms.value(formKeys.at(i)).toObject();
        const QStringList ks = o.keys();
        for (int k = 0; k < ks.size(); ++k)
            values.insert(ks.at(k), o.value(ks.at(k)).toString());
        m_formValues.insert(formKeys.at(i), values);
    }
    m_loading = false;
    m_dirty = false;
    m_formRevision++;
}

void AppController::syncFormsFromMeta()
{
    QVariantMap survey = m_formValues.value(QStringLiteral("survey"));
    survey.insert(QStringLiteral("accidentTime"), m_meta.value(QStringLiteral("accidentTime")).toString());
    survey.insert(QStringLiteral("roadName"), m_meta.value(QStringLiteral("location")).toString());
    survey.insert(QStringLiteral("locationDescription"), m_meta.value(QStringLiteral("location")).toString());
    survey.insert(QStringLiteral("surveyPeople"), m_meta.value(QStringLiteral("officer")).toString());
    survey.insert(QStringLiteral("drawingPeople"), m_meta.value(QStringLiteral("drawer")).toString());
    survey.insert(QStringLiteral("surveyCompany"), m_meta.value(QStringLiteral("surveyOrg")).toString());
    m_formValues.insert(QStringLiteral("survey"), survey);
    QVariantMap cert = m_formValues.value(QStringLiteral("certificate"));
    cert.insert(QStringLiteral("accidentTime"), m_meta.value(QStringLiteral("accidentTime")).toString());
    cert.insert(QStringLiteral("accidentlocal"), m_meta.value(QStringLiteral("location")).toString());
    cert.insert(QStringLiteral("weather"), m_meta.value(QStringLiteral("weather")).toString());
    const QString letters = QStringLiteral("ABCDE");
    for (int i = 0; i < m_parties.size() && i < 5; ++i) {
        const QVariantMap row = m_parties.at(i).toMap();
        const QString mark = letters.mid(i, 1);
        cert.insert(QStringLiteral("party") + mark, row.value(QStringLiteral("name")).toString());
        cert.insert(QStringLiteral("licenseTag") + mark, row.value(QStringLiteral("plate")).toString());
        cert.insert(QStringLiteral("telephone") + mark, row.value(QStringLiteral("phone")).toString());
        cert.insert(QStringLiteral("identityNumber") + mark, row.value(QStringLiteral("idNo")).toString());
    }
    m_formValues.insert(QStringLiteral("certificate"), cert);
    m_formRevision++;
}

void AppController::newCase(const QString &name, const QString &templateName)
{
    const QString title = name.trimmed().isEmpty() ? QStringLiteral("未命名案例") : name.trimmed();
    resetCase(title, m_edition);
    if (!templateName.isEmpty() && templateName != QStringLiteral("空白")) {
        const QString id = m_catalog.templateIdForName(templateName);
        const TemplateDef *tpl = m_catalog.templateById(id);
        if (tpl)
            m_document.placeTemplate(*tpl, QPointF(0, 0));
    }
    m_document.ensureCompass();
    const QByteArray json = buildJson();
    if (m_store.createCase(title, m_edition, json).isEmpty()) {
        m_message = QStringLiteral("创建案例失败");
        emit updated();
        return;
    }
    enterScreen(QStringLiteral("workspace"));
    m_tab = 1;
    m_dirty = false;
    m_message = QStringLiteral("已创建案例");
    refreshCases(m_query);
    emit updated();
    fit();
}

void AppController::openSample()
{
    setEdition(QStringLiteral("sketch"));
    newCase(QStringLiteral("示例案例 · 城市路口"), QString());
    m_loading = true;
    m_meta.insert(QStringLiteral("location"), QStringLiteral("人民路与中山路交叉口"));
    m_meta.insert(QStringLiteral("accidentTime"), QStringLiteral("2026-09-25 14:30"));
    m_meta.insert(QStringLiteral("weather"), QStringLiteral("晴"));
    m_meta.insert(QStringLiteral("officer"), QStringLiteral("张明"));
    m_meta.insert(QStringLiteral("drawer"), QStringLiteral("张明"));
    m_meta.insert(QStringLiteral("caseNumber"), QStringLiteral("示例-001"));
    m_parties.clear();
    m_parties << partyMap(QStringLiteral("王强"), QStringLiteral("辽A12345"));
    m_parties << partyMap(QStringLiteral("李娜"), QStringLiteral("辽A67890"));

    const QString group = newUuid();
    const double arm = 34;
    const double mouth = 12;
    const double edge = 7;
    const double lane = 3.5;
    const auto addLine = [this, group](double x1, double y1, double x2, double y2, int style, const QString &name) {
        SceneObject obj;
        obj.type = QStringLiteral("roadline");
        obj.name = name;
        obj.groupId = group;
        obj.lineStyle = style;
        obj.points << QPointF(x1, y1) << QPointF(x2, y2);
        m_document.addObject(obj);
    };
    addLine(-arm, edge, -mouth, edge, 1, QStringLiteral("路边线"));
    addLine(-arm, -edge, -mouth, -edge, 1, QStringLiteral("路边线"));
    addLine(mouth, edge, arm, edge, 1, QStringLiteral("路边线"));
    addLine(mouth, -edge, arm, -edge, 1, QStringLiteral("路边线"));
    addLine(-edge, mouth, -edge, arm, 1, QStringLiteral("路边线"));
    addLine(edge, mouth, edge, arm, 1, QStringLiteral("路边线"));
    addLine(-edge, -arm, -edge, -mouth, 1, QStringLiteral("路边线"));
    addLine(edge, -arm, edge, -mouth, 1, QStringLiteral("路边线"));
    addLine(-arm, 0, -mouth, 0, 3, QStringLiteral("中心双实线"));
    addLine(mouth, 0, arm, 0, 3, QStringLiteral("中心双实线"));
    addLine(0, -arm, 0, -mouth, 3, QStringLiteral("中心双实线"));
    addLine(0, mouth, 0, arm, 3, QStringLiteral("中心双实线"));
    addLine(-arm, lane, -mouth, lane, 2, QStringLiteral("车道虚线"));
    addLine(-arm, -lane, -mouth, -lane, 2, QStringLiteral("车道虚线"));
    addLine(mouth, lane, arm, lane, 2, QStringLiteral("车道虚线"));
    addLine(mouth, -lane, arm, -lane, 2, QStringLiteral("车道虚线"));
    addLine(-lane, -arm, -lane, -mouth, 2, QStringLiteral("车道虚线"));
    addLine(lane, -arm, lane, -mouth, 2, QStringLiteral("车道虚线"));
    addLine(-lane, mouth, -lane, arm, 2, QStringLiteral("车道虚线"));
    addLine(lane, mouth, lane, arm, 2, QStringLiteral("车道虚线"));
    addLine(-edge + 0.6, mouth - 1.2, edge - 0.6, mouth - 1.2, 1, QStringLiteral("停止线"));
    addLine(-edge + 0.6, -mouth + 1.2, edge - 0.6, -mouth + 1.2, 1, QStringLiteral("停止线"));
    addLine(-mouth + 1.2, -edge + 0.6, -mouth + 1.2, edge - 0.6, 1, QStringLiteral("停止线"));
    addLine(mouth - 1.2, -edge + 0.6, mouth - 1.2, edge - 0.6, 1, QStringLiteral("停止线"));
    m_document.filletJunctions(6);

    m_document.addCrosswalk(QPointF(-edge, mouth + 1.6), QPointF(edge, mouth + 1.6), 3.2);
    m_document.addCrosswalk(QPointF(-edge, -mouth - 1.6), QPointF(edge, -mouth - 1.6), 3.2);
    m_document.addCrosswalk(QPointF(-mouth - 1.6, -edge), QPointF(-mouth - 1.6, edge), 3.2);
    m_document.addCrosswalk(QPointF(mouth + 1.6, -edge), QPointF(mouth + 1.6, edge), 3.2);
    m_document.addGuide(QPointF(-24, -lane * 0.5), QPointF(-16, -lane * 0.5));
    m_document.addGuide(QPointF(lane * 0.5, -24), QPointF(lane * 0.5, -16));

    QString carA;
    if (m_catalog.symbol(QStringLiteral("小轿车"))) {
        carA = m_document.placeSymbol(m_catalog, QStringLiteral("小轿车"), QPointF(-20, -lane * 0.5));
        m_document.setSelection(carA);
        m_document.setProperty(QStringLiteral("rotation"), 0, &m_catalog);
        const QString carB = m_document.placeSymbol(m_catalog, QStringLiteral("小轿车"), QPointF(lane * 0.5, -20));
        m_document.setSelection(carB);
        m_document.setProperty(QStringLiteral("rotation"), 90, &m_catalog);
    }
    QVector<QPointF> skid;
    skid << QPointF(-15.5, -2.4) << QPointF(-10.5, -1.7) << QPointF(-6.2, -0.6) << QPointF(-2.4, 0.8);
    m_document.addTrace(skid, 4);
    m_document.addDimension(QPointF(-30, -edge), QPointF(-30, edge), 0, QString(), QString());
    m_document.addDimension(QPointF(-24, -edge - 3.2), QPointF(-14, -edge - 3.2), 0, QString(), QString());
    m_document.addText(QPointF(-3, arm + 3.5), QStringLiteral("人民路"));
    m_document.addText(QPointF(arm - 2, 4.2), QStringLiteral("中山路"));
    m_document.parkCompass(&m_catalog);
    if (!carA.isEmpty())
        m_document.setSelection(carA);
    m_loading = false;
    syncFormsFromMeta();
    m_records.clear();
    QVariantMap rec;
    rec.insert(QStringLiteral("id"), newUuid());
    rec.insert(QStringLiteral("time"), m_meta.value(QStringLiteral("accidentTime")).toString());
    rec.insert(QStringLiteral("text"), QStringLiteral("两车在路口发生侧面碰撞，西进口道可见侧滑痕迹。"));
    m_records.append(rec);
    save();
    fit();
    m_message = QStringLiteral("已打开示例案例");
    emit updated();
}

void AppController::openAerialSample()
{
    setEdition(QStringLiteral("aerial"));
    newCase(QStringLiteral("示例案例 · 航拍路口"), QString());
    const QString relative = QStringLiteral("aerial/base.png");
    const QString path = absoluteCasePath(relative);
    QImage image(1100, 760, QImage::Format_RGB32);
    image.fill(QColor(168, 176, 166));
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(90, 96, 98));
    painter.drawRect(0, 340, 1100, 80);
    painter.drawRect(500, 0, 80, 760);
    painter.setPen(QPen(Qt::white, 3, Qt::DashLine));
    painter.drawLine(0, 380, 1100, 380);
    painter.drawLine(540, 0, 540, 760);
    painter.end();
    QDir().mkpath(QFileInfo(path).absolutePath());
    image.save(path);
    AerialLayer aerial;
    aerial.file = path;
    aerial.imageWidth = image.width();
    aerial.imageHeight = image.height();
    aerial.originX = image.width() / 2.0;
    aerial.originY = image.height() / 2.0;
    aerial.mpp = 0.05;
    aerial.opacity = 0.92;
    aerial.calibrated = true;
    aerial.knownMetres = 20;
    m_document.setAerial(aerial);
    m_aerialImage = image;
    if (m_catalog.symbol(QStringLiteral("小轿车")))
        m_document.placeSymbol(m_catalog, QStringLiteral("小轿车"), QPointF(-6, 1.2));
    m_document.addDimension(QPointF(-10, -6), QPointF(10, -6), 0, QString(), QString());
    m_document.parkCompass(&m_catalog);
    m_meta.insert(QStringLiteral("location"), QStringLiteral("航拍：建设大街与文化路"));
    m_meta.insert(QStringLiteral("accidentTime"), QStringLiteral("2026-09-25 10:00"));
    syncFormsFromMeta();
    save();
    if (m_canvas)
        m_canvas->update();
    fit();
    m_message = QStringLiteral("已打开航拍示例。可用「标定」按已知距离校正比例。");
    emit updated();
}

void AppController::openCase(const QString &id)
{
    QString error;
    const QByteArray bytes = m_store.loadCase(id, &error);
    if (bytes.isEmpty()) {
        m_message = error;
        emit updated();
        return;
    }
    loadJson(bytes);
    enterScreen(QStringLiteral("workspace"));
    m_showCases = false;
    m_tab = 1;
    m_message = QStringLiteral("已打开案例");
    emit updated();
    fit();
}

bool AppController::save()
{
    if (m_id.isEmpty())
        return false;
    syncFormsFromMeta();
    QImage thumb(480, 320, QImage::Format_ARGB32_Premultiplied);
    thumb.fill(QColor(243, 246, 245));
    QPainter painter(&thumb);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(240, 170);
    painter.scale(6, -6);
    PaintOptions options;
    options.grid = false;
    options.aerial = m_aerialImage.isNull() ? 0 : &m_aerialImage;
    paintScene(painter, m_document, m_catalog, options);
    painter.end();
    const bool ok = m_store.saveCase(m_id, buildJson(), thumb);
    if (ok)
        m_dirty = false;
    m_message = ok ? QStringLiteral("已保存") : QStringLiteral("保存失败");
    refreshCases(m_query);
    emit updated();
    return ok;
}

void AppController::goHome()
{
    if (m_dirty)
        save();
    enterScreen(QStringLiteral("home"));
    m_showCases = false;
    emit updated();
}

void AppController::refreshCases(const QString &query)
{
    m_query = query;
    m_cases = m_store.search(query);
    emit updated();
}

bool AppController::deleteCase(const QString &id)
{
    QString error;
    const bool ok = m_store.deleteCase(id, &error);
    if (!ok)
        m_message = error;
    if (id == m_id) {
        m_id.clear();
        enterScreen(QStringLiteral("home"));
    }
    refreshCases(m_query);
    return ok;
}

bool AppController::duplicateCase(const QString &id)
{
    QString created;
    QString error;
    const bool ok = m_store.duplicateCase(id, &created, &error);
    m_message = ok ? QStringLiteral("已复制案例") : error;
    refreshCases(m_query);
    emit updated();
    return ok;
}

void AppController::placeTemplate(const QString &id)
{
    const TemplateDef *tpl = m_catalog.templateById(id);
    if (!tpl)
        return;
    const QRectF bounds = m_document.contentBounds(&m_catalog);
    const QPointF anchor = bounds.isValid() && m_document.objectCount() > 1
            ? QPointF(bounds.right() + bounds.width() * 0.15 + 4, bounds.center().y())
            : QPointF(0, 0);
    m_document.placeTemplate(*tpl, anchor);
    m_message = QStringLiteral("已放入 ") + tpl->name;
    emit updated();
}

void AppController::activateLibrary(const QString &name, const QString &notification, int flag, const QString &uuid)
{
    if (!m_canvas)
        return;
    if (name == QStringLiteral("双车道")) { setLaneCount(2); setTool(2); return; }
    if (name == QStringLiteral("四车道")) { setLaneCount(4); setTool(2); return; }
    if (name == QStringLiteral("铅笔画路") || notification == QLatin1String("PencilDrawingWay")) { setTool(2); return; }
    if (name == QStringLiteral("环岛") || notification == QLatin1String("PencilDrawingCircle")) { setTool(10); return; }
    if (name == QStringLiteral("人行横道")) { setTool(8); return; }
    if (name == QStringLiteral("文字注释")) { setTool(7); return; }
    if (name == QStringLiteral("转向标志") || name == QStringLiteral("导向标志")) { setTool(9); return; }
    if (name == QStringLiteral("分道线")) { setLaneCount(1); setTool(2); return; }
    if (notification == QLatin1String("PencilDrawingTrace")) {
        m_canvas->setTraceType(flag);
        setTool(4);
        return;
    }
    if (notification == QLatin1String("PencilDrawingFallout")) {
        m_canvas->setDebrisVariant(flag);
        setTool(5);
        return;
    }
    if (notification == QLatin1String("onUsePencilDrawingDimension")) { setTool(6); return; }
    const QString resolved = m_catalog.resolveSymbolName(uuid, name);
    if (!resolved.isEmpty()) {
        m_canvas->setSymbolName(resolved);
        setTool(3);
        m_message = QStringLiteral("单击画布放置 ") + name;
        emit updated();
    }
}

QVariantList AppController::searchLibrary(const QString &query, int groupIndex) const
{
    return m_catalog.searchLibrary(query, groupIndex);
}

void AppController::openSymbolLibrary()
{
    emit symbolLibraryRequested();
}

void AppController::closeSymbolLibrary()
{
    emit symbolLibraryClosed();
}

void AppController::undo() { m_document.undo(); }
void AppController::redo() { m_document.redo(); }
void AppController::removeSelection() { m_document.removeSelection(); }
void AppController::duplicateSelection() { m_document.duplicateSelection(); }
void AppController::selectObject(const QString &id) { m_document.setSelection(id); emit selectionChanged(); }
void AppController::setProp(const QString &key, const QVariant &value) { m_document.setProperty(key, value, &m_catalog); }

void AppController::setMetaField(const QString &key, const QString &value)
{
    m_meta.insert(key, value);
    markDirty();
    syncFormsFromMeta();
    emit updated();
}

void AppController::setParty(int index, const QString &key, const QString &value)
{
    while (m_parties.size() <= index)
        m_parties.append(partyMap());
    QVariantMap row = m_parties.at(index).toMap();
    row.insert(key, value);
    m_parties[index] = row;
    markDirty();
    syncFormsFromMeta();
    emit updated();
}

void AppController::addParty()
{
    if (m_parties.size() >= 5)
        return;
    m_parties.append(partyMap());
    markDirty();
    emit updated();
}

void AppController::importPhoto(const QString &path, const QString &caption)
{
    if (path.isEmpty() || m_id.isEmpty())
        return;
    const QString id = newUuid();
    const QString ext = QFileInfo(path).suffix().isEmpty() ? QStringLiteral("jpg") : QFileInfo(path).suffix();
    const QString relative = QStringLiteral("photos/") + id + QLatin1Char('.') + ext;
    copyIntoCase(path, relative);
    QVariantMap row;
    row.insert(QStringLiteral("id"), id);
    row.insert(QStringLiteral("file"), relative);
    row.insert(QStringLiteral("caption"), caption);
    m_photos.append(row);
    markDirty();
    emit updated();
}

void AppController::addRecord(const QString &text)
{
    if (text.trimmed().isEmpty())
        return;
    QVariantMap row;
    row.insert(QStringLiteral("id"), newUuid());
    row.insert(QStringLiteral("text"), text.trimmed());
    row.insert(QStringLiteral("time"), QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm")));
    m_records.append(row);
    markDirty();
    emit updated();
}

void AppController::removePhoto(const QString &id)
{
    for (int i = 0; i < m_photos.size(); ++i) {
        if (m_photos.at(i).toMap().value(QStringLiteral("id")).toString() == id) {
            m_photos.removeAt(i);
            break;
        }
    }
    markDirty();
    emit updated();
}

QVariantList AppController::formCatalog() const
{
    QVariantList list;
    const QVector<FormDef> &forms = m_forms.forms();
    for (int i = 0; i < forms.size(); ++i) {
        QVariantMap row;
        row.insert(QStringLiteral("id"), forms.at(i).id);
        row.insert(QStringLiteral("title"), forms.at(i).title);
        row.insert(QStringLiteral("pages"), forms.at(i).pages.size());
        list.append(row);
    }
    return list;
}

QVariantList AppController::currentFields() const
{
    QVariantList list;
    const FormDef *form = m_forms.find(m_formId);
    if (!form || m_formPage < 0 || m_formPage >= form->pages.size())
        return list;
    const QVariantMap values = m_formValues.value(m_formId);
    const QVector<FormField> &fields = form->pages.at(m_formPage).fields;
    for (int i = 0; i < fields.size(); ++i) {
        const FormField &field = fields.at(i);
        if (field.kind == QLatin1String("sign"))
            continue;
        QVariantMap row;
        row.insert(QStringLiteral("key"), field.key);
        row.insert(QStringLiteral("label"), field.label);
        row.insert(QStringLiteral("kind"), field.kind);
        row.insert(QStringLiteral("value"), values.value(field.key).toString());
        row.insert(QStringLiteral("options"), field.options);
        list.append(row);
    }
    return list;
}

void AppController::setFormValue(const QString &key, const QString &value)
{
    QVariantMap values = m_formValues.value(m_formId);
    values.insert(key, value);
    m_formValues.insert(m_formId, values);
    m_formRevision++;
    markDirty();
    emit updated();
}

bool AppController::exportScene(const QString &path, const QString &paper, bool landscape, int scaleDenom)
{
    SceneSheet sheet;
    sheet.paper = paper;
    sheet.landscape = landscape;
    sheet.scaleDenom = scaleDenom;
    sheet.location = m_meta.value(QStringLiteral("location")).toString();
    sheet.accidentTime = m_meta.value(QStringLiteral("accidentTime")).toString();
    sheet.weather = m_meta.value(QStringLiteral("weather")).toString();
    sheet.drawer = m_meta.value(QStringLiteral("drawer")).toString();
    if (sheet.drawer.isEmpty())
        sheet.drawer = m_meta.value(QStringLiteral("officer")).toString();
    QString error;
    const bool ok = exportScenePdf(path, m_document, m_catalog, m_aerialImage.isNull() ? 0 : &m_aerialImage, sheet, &error);
    m_message = ok ? QStringLiteral("已导出现场图") : error;
    emit updated();
    return ok;
}

bool AppController::exportForm(const QString &path, const QString &paper)
{
    syncFormsFromMeta();
    QString error;
    const bool ok = exportFormPdf(path, m_forms, m_formId, m_formValues.value(m_formId), paper, &error);
    m_message = ok ? QStringLiteral("已导出文书") : error;
    emit updated();
    return ok;
}

void AppController::importAerial(const QString &path)
{
    if (path.isEmpty() || m_id.isEmpty())
        return;
    QImage image(path);
    if (image.isNull()) {
        m_message = QStringLiteral("无法读取照片");
        emit updated();
        return;
    }
    const QString relative = QStringLiteral("aerial/base.png");
    const QString dest = absoluteCasePath(relative);
    QDir().mkpath(QFileInfo(dest).absolutePath());
    image.save(dest);
    AerialLayer aerial = m_document.aerial();
    aerial.file = dest;
    aerial.imageWidth = image.width();
    aerial.imageHeight = image.height();
    aerial.originX = image.width() / 2.0;
    aerial.originY = image.height() / 2.0;
    aerial.mpp = 40.0 / qMax(1, image.width());
    aerial.calibrated = false;
    aerial.hasRefs = false;
    aerial.opacity = 0.9;
    m_document.setAerial(aerial);
    m_aerialImage = image;
    m_edition = QStringLiteral("aerial");
    if (m_canvas)
        m_canvas->update();
    markDirty();
    m_message = QStringLiteral("已导入航拍底图，请用两点标定地面距离");
    fit();
    emit updated();
}

QString AppController::confirmCalibration(double metres)
{
    const QString text = m_document.calibrateAerial(metres);
    rememberAerialImage();
    m_message = text;
    emit updated();
    return text;
}

void AppController::clearDrawings()
{
    m_document.clearDrawings();
    m_message = QStringLiteral("已清除图面，底图仍保留");
    emit updated();
}

QString AppController::proportionalize()
{
    const QString text = m_document.proportionalize();
    m_message = text;
    emit updated();
    return text;
}

QString AppController::scaleToMeasures()
{
    const QString text = m_document.scaleToMeasures();
    m_message = text;
    emit updated();
    return text;
}

void AppController::fit()
{
    double w = 900;
    double h = 640;
    if (m_canvas) {
        w = qMax(200.0, m_canvas->width());
        h = qMax(200.0, m_canvas->height());
    }
    m_document.fitView(w, h, &m_catalog);
}

int AppController::rebuildIndex()
{
    const int n = m_store.rebuildIndex(0);
    refreshCases(m_query);
    m_message = QStringLiteral("已根据案例目录重建索引 %1 条").arg(n);
    emit updated();
    return n;
}

void AppController::addTextAt(double x, double y, const QString &text)
{
    m_document.addText(QPointF(x, y), text);
}

void AppController::setOpacity(double opacity)
{
    AerialLayer aerial = m_document.aerial();
    aerial.opacity = qBound(0.1, opacity, 1.0);
    m_document.setAerial(aerial);
    markDirty();
}

QImage AppController::renderFormPreview(const QString &id, QSize *size) const
{
    const QStringList parts = id.split(QLatin1Char('/'));
    const QString formId = parts.value(0, m_formId);
    const int pageIndex = parts.value(1, QStringLiteral("0")).toInt();
    const FormDef *form = m_forms.find(formId);
    if (!form || pageIndex < 0 || pageIndex >= form->pages.size())
        return QImage();
    const FormPage &page = form->pages.at(pageIndex);
    QImage image(int(page.width * 0.45), int(page.height * 0.45), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.scale(0.45, 0.45);
    m_forms.paintPage(painter, page, m_formValues.value(formId));
    painter.end();
    if (size)
        *size = image.size();
    return image;
}

void AppController::runCapture(QQuickWindow *window, const QString &directory)
{
    QDir().mkpath(directory);
    const auto settle = [window]() {
        for (int i = 0; i < 6; ++i)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 40);
        if (window)
            window->update();
        QCoreApplication::processEvents();
    };
    const auto pump = [window](int ms) {
        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < ms)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 40);
        if (window)
            window->update();
        QCoreApplication::processEvents();
    };
    const auto grab = [&](const QString &name) {
        settle();
        if (!window)
            return;
        QImage image;
        if (window->contentItem()) {
            const QSharedPointer<QQuickItemGrabResult> shot = window->contentItem()->grabToImage();
            if (shot) {
                QEventLoop loop;
                QObject::connect(shot.data(), &QQuickItemGrabResult::ready, &loop, &QEventLoop::quit);
                QTimer::singleShot(2000, &loop, &QEventLoop::quit);
                loop.exec();
                image = shot->image();
            }
        }
        if (image.isNull())
            image = window->grabWindow();
        const QString path = QDir(directory).filePath(name + QStringLiteral(".png"));
        if (image.isNull() || !image.save(path))
            qWarning("截图失败: %s (%dx%d)", qPrintable(name), image.width(), image.height());
    };
    const auto resizeTo = [&](int w, int h) {
        if (!window)
            return;
        window->resize(w, h);
        settle();
    };
    enterScreen(QStringLiteral("home"));
    emit updated();
    resizeTo(1360, 860);
    grab(QStringLiteral("01-home-desktop"));
    resizeTo(390, 844);
    grab(QStringLiteral("02-home-phone"));
    openSample();
    resizeTo(1360, 860);
    m_tab = 1;
    emit updated();
    openSymbolLibrary();
    pump(400);
    grab(QStringLiteral("03-editor-desktop"));
    grab(QStringLiteral("11-library-desktop"));
    m_tab = 0;
    emit updated();
    grab(QStringLiteral("04-case-info"));
    m_tab = 2;
    emit updated();
    grab(QStringLiteral("05-photos"));
    m_tab = 3;
    emit updated();
    grab(QStringLiteral("06-forms"));
    m_tab = 1;
    resizeTo(1024, 768);
    emit updated();
    grab(QStringLiteral("07-editor-tablet"));
    resizeTo(390, 844);
    emit updated();
    grab(QStringLiteral("08-editor-phone"));
    openSymbolLibrary();
    pump(800);
    grab(QStringLiteral("12-library-phone"));
    closeSymbolLibrary();
    pump(300);
    m_showCases = true;
    refreshCases(QString());
    emit updated();
    grab(QStringLiteral("09-cases"));
    m_showCases = false;
    openAerialSample();
    resizeTo(1360, 860);
    emit updated();
    grab(QStringLiteral("10-aerial-desktop"));
    exportScene(QDir(directory).filePath(QStringLiteral("scene-a3.pdf")), QStringLiteral("A3"), true, 200);
    setFormId(QStringLiteral("survey"));
    exportForm(QDir(directory).filePath(QStringLiteral("form-survey-a4.pdf")), QStringLiteral("A4"));
    setFormId(QStringLiteral("inquiry"));
    exportForm(QDir(directory).filePath(QStringLiteral("form-inquiry-a4.pdf")), QStringLiteral("A4"));
    setFormId(QStringLiteral("interrogation"));
    exportForm(QDir(directory).filePath(QStringLiteral("form-interrogation-a4.pdf")), QStringLiteral("A4"));
    setFormId(QStringLiteral("certificate"));
    exportForm(QDir(directory).filePath(QStringLiteral("form-certificate-a4.pdf")), QStringLiteral("A4"));
    exportForm(QDir(directory).filePath(QStringLiteral("form-certificate-a3.pdf")), QStringLiteral("A3"));
}

} // namespace sr
