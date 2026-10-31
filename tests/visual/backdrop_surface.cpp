#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTemporaryDir>
#include "qml_material/item/backdrop_surface.hpp"

class BackdropSurfaceTest : public QObject {
    Q_OBJECT
private slots:
    void capture() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
Window {
    width: 400; height: 260; visible: true
    Rectangle {
        id: background; objectName: 'background'; anchors.fill: parent; color: 'red'
        Image { objectName: 'image'; anchors.fill: parent; asynchronous: true; visible: false }
        Rectangle { objectName: 'stripe'; x: 100; width: 24; height: parent.height; color: 'white'; visible: false }
    }
    Item {
        id: host; objectName: 'host'; width: parent.width; height: parent.height
        MD.BackdropSurface {
            objectName: 'surface'; x: 60; y: 140; width: 280; height: 80
            sourceItem: background; blur: 0; radius: 24
            tint: 'transparent'; fallbackColor: 'yellow'
            Rectangle { x: 130; y: 30; width: 20; height: 20; color: 'white' }
        }
    }
})",
                          QUrl("qrc:/backdrop-test.qml"));
        QScopedPointer<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        auto* window     = qobject_cast<QQuickWindow*>(root.data());
        auto* surface    = root->findChild<qml_material::BackdropSurface*>("surface");
        auto* background = root->findChild<QQuickItem*>("background");
        auto* host       = root->findChild<QQuickItem*>("host");
        QVERIFY(window && surface && background && host);
        QVERIFY(QTest::qWaitForWindowExposed(window));
        QTRY_VERIFY(surface->captureEnabled());
        if (window->rendererInterface()->graphicsApi() == QSGRendererInterface::Software) {
            QVERIFY(! surface->property("effectAvailable").toBool());
            return;
        }
        QTRY_VERIFY(surface->property("effectAvailable").toBool());
        QCOMPARE(surface->captureRect(), QRectF(28, 108, 344, 144));
        const auto dpr = window->effectiveDevicePixelRatio();
        qInfo() << "backdrop capture DPR" << dpr;
        QCOMPARE(surface->captureSize(), QSize(qCeil(172 * dpr), qCeil(72 * dpr)));
        auto sample = [&](int x, int y) {
            const auto image = window->grabWindow();
            return image.isNull()
                       ? QColor()
                       : image.pixelColor(qRound(x * image.width() / qreal(window->width())),
                                          qRound(y * image.height() / qreal(window->height())));
        };
        QTRY_VERIFY(sample(90, 180).red() > 240 && sample(90, 180).green() < 10);
        QCOMPARE(sample(200, 180), QColor(Qt::white));
        const auto rounded = window->grabWindow();
        QVERIFY(!rounded.isNull());
        const auto scale = qreal(rounded.width()) / window->width();
        for (int y = qFloor(140 * scale); y < qCeil(168 * scale); ++y)
            for (int x = qFloor(60 * scale); x < qCeil(88 * scale); ++x)
                QVERIFY2(rounded.pixelColor(x, y).green() < 8,
                         "fallback color leaked through the rounded edge");
        auto* stripe = root->findChild<QQuickItem*>("stripe");
        stripe->setVisible(true);
        surface->setProperty("blur", 1.0);
        QTRY_VERIFY(sample(94, 180).green() > 5);
        stripe->setVisible(false);
        surface->setProperty("blur", 0.0);
        background->setProperty("color", QColor(Qt::green));
        QTRY_VERIFY(sample(90, 180).green() > 120 && sample(90, 180).red() < 10);
        QCOMPARE(sample(200, 180), QColor(Qt::white));
        surface->setProperty("tint", QColor(Qt::blue));
        QTRY_COMPARE(sample(90, 180), QColor(Qt::blue));
        QVERIFY(sample(61, 141).green() > 120);
        surface->setProperty("tint", QColor(Qt::transparent));
        host->setX(12);
        QCOMPARE(host->x(), 12.0);
        QTRY_COMPARE(surface->captureRect().x(), 40.0);
        host->setScale(1.5);
        QTRY_COMPARE(surface->captureSize(), QSize(qCeil(258 * dpr), qCeil(108 * dpr)));
        host->setScale(1);
        QTRY_COMPARE(surface->captureSize(), QSize(qCeil(172 * dpr), qCeil(72 * dpr)));
        QTemporaryDir images;
        QImage        blue(32, 32, QImage::Format_RGB32);
        blue.fill(Qt::blue);
        const auto imagePath = images.filePath("blue.png");
        QVERIFY(blue.save(imagePath));
        auto* image = root->findChild<QQuickItem*>("image");
        image->setVisible(true);
        image->setProperty("source", QUrl::fromLocalFile(imagePath));
        QTRY_COMPARE(image->property("status").toInt(), 1);
        QTRY_COMPARE(sample(100, 180), QColor(Qt::blue));
        image->setVisible(false);
        QTRY_VERIFY(sample(100, 180).green() > 120);
        surface->setMaximumTextureSize(100);
        QVERIFY(surface->captureSize().width() <= 100);
        QVERIFY(surface->captureSize().height() <= 100);
        surface->setActive(false);
        QVERIFY(! surface->captureEnabled());
        QTRY_VERIFY(! surface->property("effectAvailable").toBool());
        background->setProperty("color", QColor(Qt::red));
        surface->setActive(true);
        QTRY_VERIFY(surface->property("effectAvailable").toBool());
        QTRY_VERIFY(sample(100, 180).red() > 240 && sample(100, 180).green() < 10);
        window->setPersistentGraphics(false);
        window->setPersistentSceneGraph(false);
        window->hide();
        window->releaseResources();
        QTRY_VERIFY(! surface->captureEnabled());
        window->show();
        QVERIFY(QTest::qWaitForWindowExposed(window));
        QTRY_VERIFY(surface->property("effectAvailable").toBool());
        QTRY_VERIFY(sample(100, 180).red() > 240);
        surface->setSourceItem(host);
        QVERIFY(! surface->captureEnabled());
        surface->setSourceItem(background);
        QTRY_VERIFY(surface->captureEnabled());
        host->setRotation(10);
        QTRY_VERIFY(! surface->captureEnabled());
        host->setRotation(0);
        QTRY_VERIFY(surface->captureEnabled());
        QQuickWindow other;
        QTest::ignoreMessage(
            QtWarningMsg,
            "QQuickItem: Cannot use same item on different windows at the same time.");
        background->setParentItem(other.contentItem());
        QVERIFY(! surface->captureEnabled());
        background->setParentItem(window->contentItem());
        QTRY_VERIFY(surface->captureEnabled());
        QQuickItem empty(window->contentItem());
        surface->setSourceItem(&empty);
        QVERIFY(! surface->captureEnabled());
        empty.setSize(background->size());
        QVERIFY(surface->captureEnabled());
        empty.setWidth(0);
        QVERIFY(! surface->captureEnabled());
        surface->setSourceItem(background);
        QVERIFY(surface->captureEnabled());
        delete background;
        QVERIFY(! surface->captureEnabled());
        QVERIFY(! surface->sourceItem());
    }
};
int run_backdrop_surface(int argc, char** argv) {
    BackdropSurfaceTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "backdrop_surface.moc"
