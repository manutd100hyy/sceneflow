#include "export/PdfExporter.h"
#include "forms/FormLayout.h"
#include "io/CaseStore.h"
#include "model/Catalog.h"
#include "model/Geometry.h"
#include "model/SceneDocument.h"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QPainter>
#include <QTemporaryDir>
#include <QtTest>

using namespace sr;

class TstCore : public QObject {
    Q_OBJECT
private slots:
    void offsetHorizontal();
    void aerialRoundTrip();
    void proportionalizeMovesLinkedObject();
    void medianScale();
    void jsonRoundTrip();
    void catalogCarAndCrossroad();
    void caseStoreSearchRebuildDuplicate();
    void vectorPdfHeader();
    void filletRightAngle();
    void proportionalizeRightAngle();
    void libraryMenuResolvesSymbols();
};

void TstCore::offsetHorizontal()
{
    QVector<QPointF> line;
    line << QPointF(0, 0) << QPointF(10, 0);
    const QVector<QPointF> off = offsetPolyline(line, 1.0);
    QCOMPARE(off.size(), 2);
    QVERIFY(qAbs(off.at(0).y() - 1.0) < 1e-6);
    QVERIFY(qAbs(off.at(1).y() - 1.0) < 1e-6);
    QVERIFY(qAbs(off.at(1).x() - 10.0) < 1e-6);
}

void TstCore::aerialRoundTrip()
{
    AerialPose pose;
    pose.mpp = 0.05;
    pose.originX = 400;
    pose.originY = 300;
    pose.rotationDeg = 15;
    const QPointF pixel(520, 180);
    const QPointF world = pose.pixelToWorld(pixel);
    const QPointF back = pose.worldToPixel(world);
    QVERIFY(qAbs(back.x() - pixel.x()) < 1e-4);
    QVERIFY(qAbs(back.y() - pixel.y()) < 1e-4);
}

void TstCore::proportionalizeMovesLinkedObject()
{
    SceneDocument doc;
    SceneObject car;
    car.type = QStringLiteral("symbol");
    car.name = QStringLiteral("小轿车");
    car.symbolId = car.name;
    car.x = 0;
    car.y = 0;
    const QString carId = doc.addObject(car);

    SceneObject dim;
    dim.type = QStringLiteral("dimension");
    dim.points << QPointF(0, 0) << QPointF(10, 0);
    dim.measured = 5;
    dim.fromId = carId;
    doc.addObject(dim);

    const QString msg = doc.proportionalize();
    QVERIFY(msg.contains(QStringLiteral("已按实测")));
    const SceneObject *moved = doc.object(carId);
    QVERIFY(moved);
    QVERIFY2(qAbs(moved->x - 5.0) < 1e-6, qPrintable(msg));
    const SceneObject *dimObj = doc.object(doc.selectionId());
    QVERIFY(dimObj);
    QVERIFY(qAbs(dist(dimObj->points.at(0), dimObj->points.at(1)) - 5.0) < 1e-4);
    doc.undo();
    QVERIFY(qAbs(doc.object(carId)->x) < 1e-6);
}

void TstCore::medianScale()
{
    SceneDocument doc;
    SceneObject mark;
    mark.type = QStringLiteral("text");
    mark.x = 10;
    mark.y = 0;
    mark.name = QStringLiteral("点");
    doc.addObject(mark);

    SceneObject dim;
    dim.type = QStringLiteral("dimension");
    dim.points << QPointF(0, 0) << QPointF(10, 0);
    dim.measured = 5;
    doc.addObject(dim);

    const QString msg = doc.scaleToMeasures();
    QVERIFY2(msg.contains(QStringLiteral("整体缩放")), qPrintable(msg));
    const SceneObject *scaled = 0;
    const QVector<SceneObject> &objs = doc.objects();
    for (int i = 0; i < objs.size(); ++i) {
        if (objs.at(i).type == QLatin1String("text"))
            scaled = &objs.at(i);
    }
    QVERIFY(scaled);
    // 缩放中心是标注中点 (5, 0)，文字从 x=10 收到一半距离。
    QVERIFY(qAbs(scaled->x - 7.5) < 1e-4);
}

void TstCore::jsonRoundTrip()
{
    SceneDocument doc;
    SceneObject line;
    line.type = QStringLiteral("roadline");
    line.points << QPointF(1.25, -3.5) << QPointF(8, 2);
    line.lineStyle = 2;
    doc.addObject(line);
    AerialLayer aerial;
    aerial.file = QStringLiteral("aerial/shot.png");
    aerial.mpp = 0.04;
    aerial.calibrated = true;
    aerial.rotation = 12;
    doc.setAerial(aerial);

    SceneDocument back;
    back.fromJson(doc.toJson());
    QCOMPARE(back.objectCount(), 1);
    QCOMPARE(back.objects().at(0).lineStyle, 2);
    QVERIFY(qAbs(back.objects().at(0).points.at(0).x() - 1.25) < 1e-9);
    QCOMPARE(back.aerial().file, aerial.file);
    QVERIFY(qAbs(back.aerial().mpp - 0.04) < 1e-12);
    QVERIFY(back.aerial().calibrated);
}

void TstCore::catalogCarAndCrossroad()
{
    Catalog catalog;
    QString error;
    QVERIFY2(catalog.load(QStringLiteral("/workspace/data"), &error), qPrintable(error));
    QVERIFY(catalog.symbolCount() >= 140);
    QCOMPARE(catalog.templateCount(), 45);
    const QRectF car = catalog.naturalBounds(QStringLiteral("小轿车"), 0);
    QVERIFY2(car.width() > 3.4 && car.width() < 4.0, qPrintable(QString::number(car.width())));
    const QString id = catalog.templateIdForName(QStringLiteral("十字路口"));
    QVERIFY(!id.isEmpty());
    const TemplateDef *tpl = catalog.templateById(id);
    QVERIFY(tpl);
    QVERIFY(tpl->lines.size() > 4);
}

void TstCore::caseStoreSearchRebuildDuplicate()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    CaseStore store;
    QString error;
    QVERIFY2(store.open(dir.path(), &error), qPrintable(error));

    const QString id = QStringLiteral("11111111-2222-3333-4444-555555555555");
    const QByteArray json = QByteArray(
        "{\"formatVersion\":1,\"units\":\"m\",\"id\":\"11111111-2222-3333-4444-555555555555\","
        "\"edition\":\"sketch\",\"created\":\"2026-09-25T10:00:00\",\"modified\":\"2026-09-25T10:00:00\","
        "\"meta\":{\"name\":\"人民路碰撞\",\"caseNumber\":\"2026-001\",\"accidentTime\":\"2026-09-25 09:30\","
        "\"location\":\"人民路与解放路交叉口\",\"officer\":\"王强\","
        "\"parties\":[{\"name\":\"张明\",\"plate\":\"辽A12345\"}]}}");
    QCOMPARE(store.createCase(QStringLiteral("人民路碰撞"), QStringLiteral("sketch"), json), id);
    const QString folder = store.caseFolder(id);
    QVERIFY(folder.contains(QLatin1Char('_')));
    QVERIFY(folder.size() > 16);

    const QVariantList hit = store.search(QStringLiteral("辽A12345"));
    QCOMPARE(hit.size(), 1);
    QCOMPARE(hit.at(0).toMap().value(QStringLiteral("officer")).toString(), QStringLiteral("王强"));
    QCOMPARE(store.search(QStringLiteral("不存在的地点")).size(), 0);

    QFile::remove(dir.path() + QStringLiteral("/index.db"));
    CaseStore rebuilt;
    QVERIFY(rebuilt.open(dir.path(), &error));
    QCOMPARE(rebuilt.rebuildIndex(&error), 1);
    QCOMPARE(rebuilt.search(QStringLiteral("张明")).size(), 1);

    QString copyId;
    QVERIFY2(rebuilt.duplicateCase(id, &copyId, &error), qPrintable(error));
    QVERIFY(copyId != id);
    const QString copyFolder = rebuilt.caseFolder(copyId);
    QVERIFY(copyFolder != folder);
    QVERIFY(copyFolder.contains(QLatin1Char('_')));
    QCOMPARE(rebuilt.search(QString()).size(), 2);
    QVERIFY(rebuilt.deleteCase(copyId, &error));
    QCOMPARE(rebuilt.search(QString()).size(), 1);
    QVERIFY(QDir(dir.path() + QStringLiteral("/trash/") + copyFolder).exists());
}

void TstCore::vectorPdfHeader()
{
    Catalog catalog;
    QString error;
    QVERIFY(catalog.load(QStringLiteral("/workspace/data"), &error));
    FormLayout forms;
    QVERIFY(forms.load(QStringLiteral("/workspace/data")));

    SceneDocument doc;
    doc.addRoad(QVector<QPointF>() << QPointF(0, 0) << QPointF(20, 0), 2, 3.5);
    SceneObject car;
    car.type = QStringLiteral("symbol");
    car.symbolId = QStringLiteral("小轿车");
    car.name = car.symbolId;
    car.x = 8;
    car.y = 1.5;
    car.rotation = 15;
    doc.addObject(car);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SceneSheet sheet;
    sheet.paper = QStringLiteral("A3");
    sheet.landscape = true;
    sheet.scaleDenom = 200;
    sheet.title = QStringLiteral("道路交通事故现场图");
    const QString scenePath = dir.path() + QStringLiteral("/scene-a3.pdf");
    QVERIFY2(exportScenePdf(scenePath, doc, catalog, 0, sheet, &error), qPrintable(error));
    QFile scene(scenePath);
    QVERIFY(scene.open(QIODevice::ReadOnly));
    QCOMPARE(scene.read(5), QByteArray("%PDF-"));
    QVERIFY(scene.size() > 800);

    QVariantMap values;
    values.insert(QStringLiteral("地点"), QStringLiteral("人民路"));
    const QString formPath = dir.path() + QStringLiteral("/survey-a4.pdf");
    QVERIFY2(exportFormPdf(formPath, forms, QStringLiteral("survey"), values, QStringLiteral("A4"), &error),
             qPrintable(error));
    QFile form(formPath);
    QVERIFY(form.open(QIODevice::ReadOnly));
    const QByteArray formBytes = form.readAll();
    QCOMPARE(formBytes.left(5), QByteArray("%PDF-"));
    QVERIFY(formBytes.size() > 1000);
    // 表格线来自 SVG 路径，不应再把扫描底图嵌成图像。
    QVERIFY(!formBytes.contains("/Subtype /Image"));
    QVERIFY(!formBytes.contains("/Subtype/Image"));
    QImage preview(400, 520, QImage::Format_ARGB32_Premultiplied);
    preview.fill(Qt::white);
    {
        QPainter painter(&preview);
        painter.scale(400.0 / 1536.0, 520.0 / 2048.0);
        const FormDef *survey = forms.find(QStringLiteral("survey"));
        QVERIFY(survey && !survey->pages.isEmpty());
        forms.paintPage(painter, survey->pages.at(0), values);
    }
    int dark = 0;
    for (int y = 40; y < preview.height() - 40; y += 3) {
        for (int x = 20; x < preview.width() - 20; x += 3) {
            if (qGray(preview.pixel(x, y)) < 180)
                ++dark;
        }
    }
    QVERIFY2(dark > 200, qPrintable(QString::number(dark)));
    QVERIFY(exportFormPdf(dir.path() + QStringLiteral("/cert-a3.pdf"), forms, QStringLiteral("certificate"),
                          values, QStringLiteral("A3"), &error));
}

void TstCore::proportionalizeRightAngle()
{
    SceneDocument doc;
    SceneObject mark;
    mark.type = QStringLiteral("text");
    mark.x = 0;
    mark.y = 0;
    const QString id = doc.addObject(mark);
    SceneObject dim;
    dim.type = QStringLiteral("dimension");
    dim.subType = 1;
    dim.points << QPointF(0, 0) << QPointF(3, 4);
    dim.measured = 14;
    dim.fromId = id;
    doc.addObject(dim);
    const QString msg = doc.proportionalize();
    QVERIFY2(msg.contains(QStringLiteral("已按实测")), qPrintable(msg));
    const SceneObject *moved = doc.object(id);
    QVERIFY(qAbs(moved->x + 3.0) < 1e-4);
    QVERIFY(qAbs(moved->y + 4.0) < 1e-4);
    const SceneObject *scaled = doc.object(doc.selectionId());
    const double leg = qAbs(scaled->points.at(1).x() - scaled->points.at(0).x())
            + qAbs(scaled->points.at(1).y() - scaled->points.at(0).y());
    QVERIFY(qAbs(leg - 14.0) < 1e-3);
}

void TstCore::filletRightAngle()
{
    SceneDocument doc;
    SceneObject west;
    west.type = QStringLiteral("roadline");
    west.lineStyle = 1;
    west.name = QStringLiteral("路边线");
    west.points << QPointF(-30, -7) << QPointF(-12, -7);
    SceneObject south;
    south.type = QStringLiteral("roadline");
    south.lineStyle = 1;
    south.name = QStringLiteral("路边线");
    south.points << QPointF(-7, -30) << QPointF(-7, -12);
    doc.addObject(west);
    doc.addObject(south);
    const QString msg = doc.filletJunctions(6);
    QVERIFY2(msg.contains(QStringLiteral("圆角")), qPrintable(msg));
    QCOMPARE(doc.objectCount(), 3);
    bool nearArc = false;
    const QVector<SceneObject> &objs = doc.objects();
    for (int i = 0; i < objs.size(); ++i) {
        if (objs.at(i).name != QStringLiteral("路口圆角"))
            continue;
        for (int k = 0; k < objs.at(i).points.size(); ++k) {
            if (dist(objs.at(i).points.at(k), QPointF(-8.76, -8.76)) < 0.45)
                nearArc = true;
        }
    }
    QVERIFY(nearArc);
}

static QStringList libraryNames(const QVariantList &rows)
{
    QStringList names;
    for (int i = 0; i < rows.size(); ++i)
        names << rows.at(i).toMap().value(QStringLiteral("name")).toString();
    return names;
}

static QVariantMap findMenuItem(const QVariantList &groups, const QString &name)
{
    for (int g = 0; g < groups.size(); ++g) {
        const QVariantList items = groups.at(g).toMap().value(QStringLiteral("items")).toList();
        for (int i = 0; i < items.size(); ++i) {
            const QVariantMap row = items.at(i).toMap();
            if (row.value(QStringLiteral("name")).toString() == name)
                return row;
        }
    }
    return QVariantMap();
}

void TstCore::libraryMenuResolvesSymbols()
{
    Catalog catalog;
    QString error;
    const QString dataDir = QString::fromUtf8(SR_DATA_DIR);
    QVERIFY2(catalog.load(dataDir, &error), qPrintable(error));

    const QVariantList groups = catalog.libraryGroups();
    QVERIFY(!groups.isEmpty());
    QCOMPARE(groups.at(0).toMap().value(QStringLiteral("name")).toString(), QStringLiteral("常用"));

    QStringList accident;
    int symbolItems = 0;
    for (int g = 0; g < groups.size(); ++g) {
        const QVariantMap group = groups.at(g).toMap();
        const QVariantList items = group.value(QStringLiteral("items")).toList();
        if (group.value(QStringLiteral("name")).toString() == QStringLiteral("交通事故元素")) {
            for (int i = 0; i < items.size(); ++i)
                accident << items.at(i).toMap().value(QStringLiteral("name")).toString();
        }
        for (int i = 0; i < items.size(); ++i) {
            const QVariantMap row = items.at(i).toMap();
            if (row.value(QStringLiteral("notification")).toString() != QLatin1String("AddTufuNotification"))
                continue;
            const QString resolved = catalog.resolveSymbolName(
                    row.value(QStringLiteral("uuid")).toString(),
                    row.value(QStringLiteral("name")).toString());
            QVERIFY2(!resolved.isEmpty(), qPrintable(group.value(QStringLiteral("name")).toString()
                                                      + QLatin1Char('/') + row.value(QStringLiteral("name")).toString()));
            ++symbolItems;
        }
    }
    QVERIFY(symbolItems > 100);
    QVERIFY(accident.contains(QStringLiteral("小轿车")));
    QVERIFY(accident.contains(QStringLiteral("客车")));
    QVERIFY(accident.contains(QStringLiteral("货车")));
    QVERIFY(accident.contains(QStringLiteral("电动自行车")));

    QCOMPARE(catalog.resolveSymbolName(findMenuItem(groups, QStringLiteral("高速服务区")).value(QStringLiteral("uuid")).toString(),
                                       QStringLiteral("高速服务区")),
             QStringLiteral("高速公路服务区"));
    QCOMPARE(catalog.resolveSymbolName(findMenuItem(groups, QStringLiteral("公、铁路交口")).value(QStringLiteral("uuid")).toString(),
                                       QStringLiteral("公、铁路交口")),
             QStringLiteral("道路与铁路平交口"));
    QCOMPARE(catalog.resolveSymbolName(findMenuItem(groups, QStringLiteral("机动车行驶轨迹")).value(QStringLiteral("uuid")).toString(),
                                       QStringLiteral("机动车行驶轨迹")),
             QStringLiteral("机动车行驶方向"));
    QCOMPARE(catalog.resolveSymbolName(findMenuItem(groups, QStringLiteral("摩托车行驶轨迹")).value(QStringLiteral("uuid")).toString(),
                                       QStringLiteral("摩托车行驶轨迹")),
             QStringLiteral("非机动车行驶方向"));
    QCOMPARE(catalog.resolveSymbolName(findMenuItem(groups, QStringLiteral("行人运动轨迹")).value(QStringLiteral("uuid")).toString(),
                                       QStringLiteral("行人运动轨迹")),
             QStringLiteral("人员行驶方向"));

    const QStringList common = libraryNames(catalog.searchLibrary(QString(), 0));
    QVERIFY(common.contains(QStringLiteral("小轿车")));
    QVERIFY(common.contains(QStringLiteral("客车")));
    QVERIFY(common.contains(QStringLiteral("货车")));
    QVERIFY(!common.contains(QStringLiteral("桥梁")));

    const QStringList truckAlias = libraryNames(catalog.searchLibrary(QStringLiteral("小货车"), 0));
    QVERIFY(truckAlias.contains(QStringLiteral("货车")));
    QVERIFY(libraryNames(catalog.searchLibrary(QStringLiteral("轻型货车"), 0)).contains(QStringLiteral("货车")));
    const QStringList van = libraryNames(catalog.searchLibrary(QStringLiteral("面包车"), 0));
    QVERIFY(van.contains(QStringLiteral("客车")));
    QVERIFY(van.contains(QStringLiteral("小轿车")));
    QVERIFY(libraryNames(catalog.searchLibrary(QStringLiteral("桥梁"), 0)).contains(QStringLiteral("桥梁")));
    QCOMPARE(libraryNames(catalog.searchLibrary(QStringLiteral("小轿车"), 2)).count(QStringLiteral("小轿车")), 1);

    const QVariantMap car = findMenuItem(groups, QStringLiteral("小轿车"));
    QVERIFY(car.value(QStringLiteral("icon")).toString().contains(QStringLiteral("symbol_car.png")));
    QVERIFY(findMenuItem(groups, QStringLiteral("客车")).value(QStringLiteral("icon")).toString().contains(QStringLiteral("symbol_bus.png")));
    QVERIFY(findMenuItem(groups, QStringLiteral("货车")).value(QStringLiteral("icon")).toString().contains(QStringLiteral("symbol_truck.png")));
    QVERIFY(findMenuItem(groups, QStringLiteral("人体")).value(QStringLiteral("icon")).toString().contains(QStringLiteral("symbol_person.png")));
    const QString iconFile = QDir(dataDir).absoluteFilePath(QStringLiteral("../assets/menuicons/Symbols/symbol_car.png"));
    QVERIFY2(QFile::exists(iconFile), qPrintable(iconFile));
}

QTEST_MAIN(TstCore)
#include "tst_core.moc"
