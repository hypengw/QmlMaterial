// #include <QtGui/6.11.0/QtGui/rhi/qrhi.h>
#include <QtGui/rhi/qrhi.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickGraphicsDevice>
#include <QQuickItem>
#include <QQuickRenderControl>
#include <QQuickRenderTarget>
#include <QQuickWindow>
#include <QTimer>
#include <QUrl>
#include <cstdlib>
#include <iostream>

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "Usage: qm_grab <input.qml> <output.png> [width=800] "
                 "[height=600] [delay_ms=200]\n";
    return 1;
  }

  const QString inputQml = QString::fromLocal8Bit(argv[1]);
  const QString outputPng = QString::fromLocal8Bit(argv[2]);
  const int width = argc >= 4 ? std::atoi(argv[3]) : 800;
  const int height = argc >= 5 ? std::atoi(argv[4]) : 600;
  const int delayMs = argc >= 6 ? std::atoi(argv[5]) : 200;

  if (!QFileInfo::exists(inputQml)) {
    std::cerr << "Input file not found: " << inputQml.toStdString() << "\n";
    return 1;
  }

  QGuiApplication app(argc, argv);

  // Let QQuickRenderControl pick and own the RHI backend automatically.
  QQuickRenderControl rc;
  QQuickWindow win(&rc);
  win.resize(width, height);

  if (!rc.initialize()) {
    std::cerr << "QQuickRenderControl initialise failed\n";
    return 1;
  }

  QRhi *rhi = rc.rhi();
  if (!rhi) {
    std::cerr << "No QRhi after initialize\n";
    return 1;
  }

  const QSize sz(width, height);

  // Create colour texture + depth/stencil renderbuffer.
  QRhiTexture *colorTex = rhi->newTexture(
      QRhiTexture::RGBA8, sz, 1,
      QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource);
  if (!colorTex->create()) {
    std::cerr << "Failed to create color texture\n";
    return 1;
  }

  QRhiRenderBuffer *dsRb =
      rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, sz, 1,
                           QRhiRenderBuffer::UsedWithSwapChainOnly);
  // UsedWithSwapChainOnly is wrong here — use default flags
  dsRb->deleteLater();
  dsRb = rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, sz);
  if (!dsRb->create()) {
    std::cerr << "Failed to create depth/stencil renderbuffer\n";
    return 1;
  }

  QRhiTextureRenderTargetDescription rtDesc(colorTex);
  rtDesc.setDepthStencilBuffer(dsRb);
  QRhiTextureRenderTarget *rt = rhi->newTextureRenderTarget(rtDesc);
  QRhiRenderPassDescriptor *rp = rt->newCompatibleRenderPassDescriptor();
  rt->setRenderPassDescriptor(rp);
  if (!rt->create()) {
    std::cerr << "Failed to create render target\n";
    return 1;
  }

  win.setRenderTarget(QQuickRenderTarget::fromRhiRenderTarget(rt));

  // QML engine + import paths.
  QQmlEngine engine;
  engine.addImportPath(QCoreApplication::applicationDirPath() +
                       "/../qml_modules");
  const QByteArray envPath = qgetenv("QML_IMPORT_PATH");
  if (!envPath.isEmpty()) {
    for (const auto &p :
         QString::fromLocal8Bit(envPath).split(':', Qt::SkipEmptyParts))
      engine.addImportPath(p);
  }

  QQmlComponent component(
      &engine, QUrl::fromLocalFile(QFileInfo(inputQml).absoluteFilePath()));
  if (component.isError()) {
    for (const auto &err : component.errors())
      std::cerr << "QML error: " << err.toString().toStdString() << "\n";
    return 1;
  }

  QObject *obj = component.create();
  if (component.isError() || !obj) {
    for (const auto &err : component.errors())
      std::cerr << "QML error: " << err.toString().toStdString() << "\n";
    return 1;
  }

  QQuickItem *rootItem = qobject_cast<QQuickItem *>(obj);
  if (!rootItem) {
    std::cerr << "Root object is not a QQuickItem\n";
    delete obj;
    return 1;
  }

  rootItem->setParentItem(win.contentItem());
  rootItem->setSize(QSizeF(width, height));

  int exitCode = 0;

  QTimer::singleShot(delayMs, &app, [&]() {
    rc.polishItems();
    rc.beginFrame();
    rc.sync();
    rc.render();

    // Read back the rendered texture into a QImage.
    QRhiReadbackResult readResult;
    bool readDone = false;
    readResult.completed = [&readDone]() { readDone = true; };

    QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
    batch->readBackTexture(QRhiReadbackDescription(colorTex), &readResult);

    // Submit a no-op pass that processes the readback.
    QRhiCommandBuffer *cb = rc.commandBuffer();
    cb->beginPass(rt, Qt::black, {1.0f, 0}, batch);
    cb->endPass();

    rc.endFrame();

    if (!readDone) {
      // Flush to ensure readback completes (synchronous on most backends).
      rhi->finish();
    }

    if (readResult.data.isEmpty()) {
      std::cerr << "Readback returned empty data\n";
      exitCode = 1;
      app.quit();
      return;
    }

    QImage img(reinterpret_cast<const uchar *>(readResult.data.constData()),
               readResult.pixelSize.width(), readResult.pixelSize.height(),
               QImage::Format_RGBA8888_Premultiplied);
    // Mirror vertically — RHI textures origin is bottom-left for OpenGL.
    img = img.flipped(Qt::Vertical);

    if (!img.save(outputPng)) {
      std::cerr << "Failed to save PNG: " << outputPng.toStdString() << "\n";
      exitCode = 1;
    }

    app.quit();
  });

  app.exec();

  delete rt;
  delete rp;
  delete dsRb;
  delete colorTex;

  return exitCode;
}
