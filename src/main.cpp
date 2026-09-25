#include "app/AppController.h"

#include "render/SceneCanvas.h"

#include <QApplication>
#include <QFont>
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

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("SketchRoad"));
    QCoreApplication::setApplicationName(QStringLiteral("SketchRoad"));
    QFont font(QStringLiteral("WenQuanYi Micro Hei"));
    font.setPixelSize(15);
    app.setFont(font);

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
