#include "videobackend.h"
#include "videoitem.h"
#include <QApplication> // QApplication for normal initialisation
#include <QMetaType>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[]) {
  // Forsing software render for QML. For kick Wayland-bugs
  qputenv("QT_QUICK_BACKEND", "software");

  // Hyprland/Wayland settings
  qputenv("QT_QPA_PLATFORM", "wayland;xcb");

  QApplication app(argc, argv);

  qRegisterMetaType<QImage>("QImage");

  VideoBackend backend;
  VideoImageProvider *provider = new VideoImageProvider();

  QQmlApplicationEngine engine;

  // Register provider with name "video"
  engine.addImageProvider("video", provider);

  engine.rootContext()->setContextProperty("videoBackend", &backend);

  // Connect backend signal with frame update in C++ provider
  QObject::connect(
      &backend, &VideoBackend::frameReady,
      [provider](const QImage &frame) { provider->updateFrame(frame); });

  engine.load(QUrl("qrc:/App/qml/Main.qml"));
  if (engine.rootObjects().isEmpty())
    return -1;

  return app.exec();
}
