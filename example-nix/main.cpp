#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);
  QQmlApplicationEngine engine;
  // Load the bundled QML from the module resource prefix (/qt/qml/Demo/)
  engine.loadFromModule("Demo", "main");
  if (engine.rootObjects().isEmpty()) {
    return -1;
  }
  return app.exec();
}
