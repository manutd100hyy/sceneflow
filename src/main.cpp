#include "app/AppController.h"

#include "render/SceneCanvas.h"

#include <QApplication>
#include <QFont>
#include <QPalette>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickImageProvider>
#include <QQuickWindow>
#include <QTimer>

namespace {

class FormPreviewProvider : public QQuickImageProvider {
public:
    explicit FormPreviewProvider(sr::AppController *app)
        : QQuickImageProvider(QQuickImageProvider::Image)
        , m_app(app)
    {
    }

    QImage requestImage(const QString &id, QSize *size, const QSize &requested) Q_DECL_OVERRIDE
    {
        Q_UNUSED(requested);
        return m_app->renderFormPreview(id, size);
    }

private:
    sr::AppController *m_app;
};

} // namespace

int main(int argc, char *argv[])
{
    QString captureDir;
    for (int i = 1; i < argc; ++i) {
        const QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == QLatin1String("--capture") && i + 1 < argc)
            captureDir = QString::fromLocal8Bit(argv[++i]);
    }
    if (!captureDir.isEmpty())
        qputenv("QT_QUICK_BACKEND", "software");
    qputenv("QT_QUICK_CONTROLS_CONF", ":/qtquickcontrols2.conf");

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("SketchRoad"));
    QCoreApplication::setApplicationName(QStringLiteral("SketchRoad"));
    QFont font(QStringLiteral("WenQuanYi Micro Hei"));
    font.setPixelSize(15);
    app.setFont(font);
    QPalette pal;
    const QColor ink(32, 53, 66);
    const QColor teal(17, 123, 112);
    const QColor paper(244, 247, 246);
    const QColor card(255, 255, 255);
    for (int group = 0; group < QPalette::NColorGroups; ++group) {
        const QPalette::ColorGroup g = static_cast<QPalette::ColorGroup>(group);
        pal.setColor(g, QPalette::Window, paper);
        pal.setColor(g, QPalette::WindowText, ink);
        pal.setColor(g, QPalette::Base, card);
        pal.setColor(g, QPalette::AlternateBase, QColor(231, 244, 241));
        pal.setColor(g, QPalette::Text, ink);
        pal.setColor(g, QPalette::Button, card);
        pal.setColor(g, QPalette::ButtonText, ink);
        pal.setColor(g, QPalette::Highlight, teal);
        pal.setColor(g, QPalette::HighlightedText, Qt::white);
        pal.setColor(g, QPalette::BrightText, ink);
        pal.setColor(g, QPalette::Dark, teal);
        pal.setColor(g, QPalette::Mid, QColor(197, 216, 212));
        pal.setColor(g, QPalette::Light, QColor(231, 244, 241));
        pal.setColor(g, QPalette::Midlight, QColor(215, 239, 233));
        pal.setColor(g, QPalette::Shadow, QColor(138, 163, 158));
    }
    app.setPalette(pal);

    qmlRegisterType<sr::SceneCanvas>("SketchRoad", 1, 0, "SceneCanvas");

    sr::AppController controller;
    QString error;
    if (!controller.start(&error)) {
        qWarning("启动失败: %s", qPrintable(error));
        return 1;
    }

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("formpage"), new FormPreviewProvider(&controller));
    engine.rootContext()->setContextProperty(QStringLiteral("app"), &controller);
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;

    if (!captureDir.isEmpty()) {
        QQuickWindow *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QTimer::singleShot(400, window, [window, &controller, captureDir, &app]() {
            controller.runCapture(window, captureDir);
            app.quit();
        });
    }
    return app.exec();
}
