#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);
  QQmlApplicationEngine engine;
  // Load the bundled QML from the module resource prefix (/qt/qml/Demo/)
  engine.load(u"qrc:/qt/qml/Demo/main.qml"_qs);
  if (engine.rootObjects().isEmpty()) {
    return -1;
  }
  return app.exec();
}
