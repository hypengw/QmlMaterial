#include <QtTest>
#include <QSemaphore>
#include <QThreadPool>
#include "qml_material/control/color_picker.hpp"

class ImageColorPickerTest : public QObject {
    Q_OBJECT
private slots:
    void latestResult() {
        using qml_material::ColorPicker;
        auto* pool = QThreadPool::globalInstance();
        const auto threads = pool->maxThreadCount();
        pool->setMaxThreadCount(1);
        QSemaphore started, release;
        pool->start([&] {
            started.release();
            release.acquire();
        });
        QVERIFY(started.tryAcquire(1, 5000));
        ColorPicker picker;
        QSignalSpy completed(&picker, &ColorPicker::picked);
        QImage image(256, 256, QImage::Format_RGB32);
        image.fill(QColor("#c00000"));
        picker.setImage(image);
        picker.pick();
        image.fill(QColor("#00c000"));
        picker.setImage(image);
        picker.pick();
        release.release();
        QTRY_COMPARE(completed.size(), 1);
        const auto green = picker.color();
        QVERIFY(green.green() > green.red());
        picker.pick();
        QTRY_COMPARE(completed.size(), 2);
        QCOMPARE(picker.color(), green);
        pool->waitForDone();
        pool->setMaxThreadCount(threads);
    }

    void clearedImage() {
        qml_material::ColorPicker picker;
        QSignalSpy completed(&picker, &qml_material::ColorPicker::picked);
        QImage image(8, 8, QImage::Format_RGB32);
        image.fill(Qt::blue);
        picker.setImage(image);
        picker.pick();
        picker.clearImage();
        QThreadPool::globalInstance()->waitForDone();
        QCoreApplication::processEvents();
        QVERIFY(picker.image().isNull());
        QVERIFY(completed.isEmpty());
    }
};

int run_image_color_picker(int argc, char** argv) {
    ImageColorPickerTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "image_color_picker.moc"
