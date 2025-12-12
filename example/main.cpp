#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QSurfaceFormat>

#include <QtQml/QQmlExtensionPlugin>

using namespace Qt::StringLiterals;

#ifdef QML_STATIC
Q_IMPORT_QML_PLUGIN(Qcm_MaterialPlugin)
#endif

int main(int argc, char* argv[]) {
// macOS defaults to an OpenGL 2.1 compatibility context if not specified.
// When forcing the OpenGL backend, request a core profile so Qt can use GLSL >= 330 variants.
#ifdef Q_OS_MACOS
    {
        QSurfaceFormat format;
        format.setRenderableType(QSurfaceFormat::OpenGL);
        format.setProfile(QSurfaceFormat::CoreProfile);
        format.setVersion(4, 1);
        format.setSamples(4);
        format.setDepthBufferSize(24);
        format.setStencilBufferSize(8);
        QSurfaceFormat::setDefaultFormat(format);
    }
#endif

#ifdef __EMSCRIPTEN__
    {
        QSurfaceFormat format;
        format.setMajorVersion(3);
        QSurfaceFormat::setDefaultFormat(format);
    }
#endif

    QGuiApplication gui_app(argc, argv);
    QCoreApplication::setApplicationName("qml_material");
    QCoreApplication::setApplicationVersion("0.0.1");

    QQmlApplicationEngine engine;
    engine.load(u"qrc:/main.qml"_s);

    return gui_app.exec();
}
