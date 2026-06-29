#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QCommandLineParser>
#include <QSurfaceFormat>
#include <QLoggingCategory>
#include <QString>
#include <QtQml/QQmlExtensionPlugin>

#ifdef Q_OS_WIN
#    include <QAbstractNativeEventFilter>
#    include <windows.h>

// Blocks WM_GETOBJECT to prevent Windows UI Automation (UIA) from activating
// Qt's accessibility bridge, which causes a dangling-pointer crash when buttons
// are pressed on Windows (Qt bug: objects deleted mid-accessibility-iteration).
class UiaBlocker : public QAbstractNativeEventFilter {
public:
    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override {
        if (eventType == "windows_generic_MSG") {
            if (static_cast<MSG*>(message)->message == WM_GETOBJECT) {
                *result = 0;
                return true;
            }
        }
        return false;
    }
};
#endif

using namespace Qt::StringLiterals;

#ifdef QML_STATIC
Q_IMPORT_QML_PLUGIN(Qcm_MaterialPlugin)
#endif

int main(int argc, char* argv[]) {
    QGuiApplication gui_app(argc, argv);

#ifdef Q_OS_WIN
    static UiaBlocker uiaBlocker;
    gui_app.installNativeEventFilter(&uiaBlocker);
#endif

    QCoreApplication::setApplicationName("qml_material");
    QCoreApplication::setApplicationVersion("0.0.1");

#ifdef __EMSCRIPTEN__
    QSurfaceFormat format;
    format.setMajorVersion(3);
#endif

    QQmlApplicationEngine engine;

    engine.addImportPath("qrc:/");
#ifdef QT_QML_OUTPUT_DIRECTORY
    engine.addImportPath(QStringLiteral(QT_QML_OUTPUT_DIRECTORY));
#endif
    engine.load(u"qrc:/main.qml"_s);

    return gui_app.exec();
}