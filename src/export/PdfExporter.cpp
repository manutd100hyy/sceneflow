#include "PdfExporter.h"

#include "forms/FormLayout.h"
#include "model/Catalog.h"
#include "model/Geometry.h"
#include "model/SceneDocument.h"
#include "render/ScenePainter.h"

#include <QImage>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QtMath>

namespace sr {

SceneSheet::SceneSheet()
    : paper(QStringLiteral("A3"))
    , landscape(true)
    , scaleDenom(200)
    , title(QStringLiteral("道路交通事故现场图"))
{
}

static QPageSize::PageSizeId pageId(const QString &paper)
{
    return paper == QLatin1String("A3") ? QPageSize::A3 : QPageSize::A4;
}

double pdfMillimetresPerMetre(int scaleDenom)
{
    if (scaleDenom <= 0)
        return 0;
    return 1000.0 / double(scaleDenom);
}

static void prepare(QPdfWriter &writer, const QString &paper, bool landscape)
{
    writer.setPageSize(QPageSize(pageId(paper)));
    writer.setPageOrientation(landscape ? QPageLayout::Landscape : QPageLayout::Portrait);
    writer.setResolution(144);
    writer.setTitle(QStringLiteral("SketchRoad"));
    writer.setCreator(QStringLiteral("SketchRoad"));
}

bool exportScenePdf(const QString &path, const SceneDocument &document, const Catalog &catalog,
                    const QImage *aerial, const SceneSheet &sheet, QString *error)
{
    QPdfWriter writer(path);
    prepare(writer, sheet.paper, sheet.landscape);
    QPainter painter(&writer);
    if (!painter.isActive()) {
        if (error)
            *error = QStringLiteral("无法创建 PDF");
        return false;
    }
    const double pxPerMm = writer.resolution() / 25.4;
    const double margin = 12 * pxPerMm;
    const double titleH = 28 * pxPerMm;
    const QRectF page(0, 0, writer.width(), writer.height());
    const QRectF frame(margin, margin, page.width() - margin * 2, page.height() - margin * 2 - titleH);
    const QRectF title(margin, page.height() - margin - titleH, page.width() - margin * 2, titleH);

    QRectF bounds = document.contentBounds(&catalog);
    if (!bounds.isValid())
        bounds = QRectF(-10, -10, 20, 20);
    bounds = bounds.adjusted(-2, -2, 2, 2);
    int denom = sheet.scaleDenom;
    double pxPerM = 0;
    if (denom <= 0) {
        const double zx = frame.width() / qMax(0.1, bounds.width());
        const double zy = frame.height() / qMax(0.1, bounds.height());
        pxPerM = qMin(zx, zy);
        const double mmPerM = pxPerM / pxPerMm;
        denom = qMax(1, int(qRound(1000.0 / qMax(0.01, mmPerM))));
    } else {
        const double mmPerM = pdfMillimetresPerMetre(denom);
        pxPerM = mmPerM * pxPerMm;
    }
    const QPointF center = bounds.center();
    const QPointF frameCenter = frame.center();

    painter.fillRect(page, Qt::white);
    painter.setPen(QPen(QColor(30, 50, 50), 1));
    painter.drawRect(frame);
    painter.setClipRect(frame);
    painter.save();
    painter.translate(frameCenter.x() - center.x() * pxPerM, frameCenter.y() + center.y() * pxPerM);
    painter.scale(pxPerM, -pxPerM);
    PaintOptions options;
    options.grid = false;
    options.forPrint = true;
    options.aerial = aerial;
    paintScene(painter, document, catalog, options);
    painter.restore();
    painter.setClipping(false);

    painter.fillRect(title, QColor(248, 250, 249));
    painter.drawRect(title);
    QFont font(QStringLiteral("WenQuanYi Micro Hei"));
    font.setPixelSize(int(4.2 * pxPerMm));
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(QColor(20, 40, 40));
    painter.drawText(QRectF(title.x() + 8, title.y() + 4, title.width() * 0.45, title.height() * 0.45),
                     Qt::AlignLeft | Qt::AlignVCenter, sheet.title);
    font.setBold(false);
    font.setPixelSize(int(3.1 * pxPerMm));
    painter.setFont(font);
    const QString info = QStringLiteral("地点：%1    时间：%2    天气：%3    绘图：%4    比例 1:%5")
            .arg(sheet.location.isEmpty() ? QStringLiteral("未填") : sheet.location)
            .arg(sheet.accidentTime.isEmpty() ? QStringLiteral("未填") : sheet.accidentTime)
            .arg(sheet.weather.isEmpty() ? QStringLiteral("未填") : sheet.weather)
            .arg(sheet.drawer.isEmpty() ? QStringLiteral("未填") : sheet.drawer)
            .arg(denom);
    painter.drawText(QRectF(title.x() + 8, title.y() + title.height() * 0.42, title.width() - 80, title.height() * 0.5),
                     Qt::AlignLeft | Qt::AlignVCenter, info);

    const double barM = denom >= 500 ? 20 : 10;
    const double barPx = barM * pxPerM;
    const QPointF barOrigin(title.right() - barPx - 16, title.center().y());
    painter.setPen(QPen(Qt::black, 1.4));
    painter.drawLine(barOrigin, barOrigin + QPointF(barPx, 0));
    painter.drawLine(barOrigin, barOrigin + QPointF(0, 4));
    painter.drawLine(barOrigin + QPointF(barPx, 0), barOrigin + QPointF(barPx, 4));
    painter.drawText(QRectF(barOrigin.x(), barOrigin.y() + 6, barPx, 16), Qt::AlignCenter,
                     QString::number(barM) + QStringLiteral(" m"));
    painter.end();
    return true;
}

bool exportFormPdf(const QString &path, const FormLayout &layout, const QString &formId,
                   const QVariantMap &values, const QString &paper, QString *error)
{
    const FormDef *form = layout.find(formId);
    if (!form || form->pages.isEmpty()) {
        if (error)
            *error = QStringLiteral("没有这种文书");
        return false;
    }
    QPdfWriter writer(path);
    prepare(writer, paper, false);
    QPainter painter(&writer);
    if (!painter.isActive()) {
        if (error)
            *error = QStringLiteral("无法创建 PDF");
        return false;
    }
    for (int i = 0; i < form->pages.size(); ++i) {
        if (i > 0)
            writer.newPage();
        const FormPage &page = form->pages.at(i);
        const double margin = 8.0 * writer.resolution() / 25.4;
        const QRectF area(margin, margin, writer.width() - margin * 2, writer.height() - margin * 2);
        const double sx = area.width() / page.width;
        const double sy = area.height() / page.height;
        const double scale = qMin(sx, sy);
        const double ox = area.x() + (area.width() - page.width * scale) * 0.5;
        const double oy = area.y() + (area.height() - page.height * scale) * 0.5;
        painter.save();
        painter.translate(ox, oy);
        painter.scale(scale, scale);
        layout.paintPage(painter, page, values);
        painter.restore();
    }
    painter.end();
    return true;
}

} // namespace sr
