#include "FormLayout.h"

#include <QFile>
#include <QFont>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>

namespace sr {

bool FormLayout::load(const QString &dataDir)
{
    m_forms.clear();
    QFile file(dataDir + QStringLiteral("/forms.json"));
    if (!file.open(QIODevice::ReadOnly))
        return false;
    const QJsonArray forms = QJsonDocument::fromJson(file.readAll()).object().value(QStringLiteral("forms")).toArray();
    for (int i = 0; i < forms.size(); ++i) {
        const QJsonObject o = forms.at(i).toObject();
        FormDef def;
        def.id = o.value(QStringLiteral("id")).toString();
        def.title = o.value(QStringLiteral("title")).toString();
        const QJsonArray pages = o.value(QStringLiteral("pages")).toArray();
        for (int p = 0; p < pages.size(); ++p) {
            const QJsonObject pageObj = pages.at(p).toObject();
            FormPage page;
            page.title = pageObj.value(QStringLiteral("title")).toString();
            const QString rel = pageObj.value(QStringLiteral("image")).toString();
            if (!rel.isEmpty())
                page.imagePath = dataDir + QLatin1Char('/') + rel;
            page.width = pageObj.value(QStringLiteral("width")).toDouble(1536);
            page.height = pageObj.value(QStringLiteral("height")).toDouble(2048);
            const QJsonArray fields = pageObj.value(QStringLiteral("fields")).toArray();
            for (int f = 0; f < fields.size(); ++f) {
                const QJsonObject fieldObj = fields.at(f).toObject();
                FormField field;
                field.key = fieldObj.value(QStringLiteral("key")).toString();
                field.label = fieldObj.value(QStringLiteral("label")).toString();
                field.kind = fieldObj.value(QStringLiteral("kind")).toString();
                field.x = fieldObj.value(QStringLiteral("x")).toDouble();
                field.y = fieldObj.value(QStringLiteral("y")).toDouble();
                field.w = fieldObj.value(QStringLiteral("w")).toDouble();
                field.h = fieldObj.value(QStringLiteral("h")).toDouble();
                field.fontSize = fieldObj.value(QStringLiteral("size")).toInt(18);
                const QJsonArray options = fieldObj.value(QStringLiteral("options")).toArray();
                for (int n = 0; n < options.size(); ++n)
                    field.options << options.at(n).toString();
                page.fields.append(field);
            }
            def.pages.append(page);
        }
        m_forms.append(def);
    }
    return !m_forms.isEmpty();
}

const QVector<FormDef> &FormLayout::forms() const { return m_forms; }

const FormDef *FormLayout::find(const QString &id) const
{
    for (int i = 0; i < m_forms.size(); ++i) {
        if (m_forms.at(i).id == id)
            return &m_forms.at(i);
    }
    return 0;
}

void FormLayout::paintPage(QPainter &painter, const FormPage &page, const QVariantMap &values) const
{
    painter.save();
    if (!page.imagePath.isEmpty()) {
        const QImage image(page.imagePath);
        if (!image.isNull())
            painter.drawImage(QRectF(0, 0, page.width, page.height), image);
    } else {
        painter.fillRect(QRectF(0, 0, page.width, page.height), Qt::white);
        painter.setPen(Qt::black);
        painter.drawRect(QRectF(8, 8, page.width - 16, page.height - 16));
    }
    QFont font(QStringLiteral("WenQuanYi Micro Hei"));
    painter.setPen(QColor(20, 20, 20));
    for (int i = 0; i < page.fields.size(); ++i) {
        const FormField &field = page.fields.at(i);
        QString text = values.value(field.key).toString();
        if (field.kind == QLatin1String("check")) {
            const QString flag = text.trimmed();
            if (flag.isEmpty() || flag == QLatin1String("0") || flag == QLatin1String("false"))
                continue;
            text = QStringLiteral("√");
        }
        if (text.trimmed().isEmpty())
            continue;
        font.setPixelSize(qMax(12, field.fontSize));
        painter.setFont(font);
        const double left = field.x - field.w * 0.5;
        const double top = page.height - field.y - field.h * 0.5;
        const QRectF rect(left, top, field.w, field.h);
        const int flags = field.w > 280
                ? Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap
                : Qt::AlignCenter | Qt::TextWordWrap;
        painter.drawText(rect, flags, text);
    }
    painter.restore();
}

} // namespace sr
