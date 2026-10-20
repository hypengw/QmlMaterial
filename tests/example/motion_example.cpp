#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QDir>
#include "qml_material/anim/spring_motion.hpp"

using namespace qml_material;

namespace
{
QAbstractAnimation* clock(QObject* motion) { return motion->findChild<QAbstractAnimation*>(); }

void polish(QQuickItem* item) {
    item->ensurePolished();
    for (auto* child : item->childItems()) polish(child);
}
} // namespace

class MotionExampleTest : public QObject {
    Q_OBJECT

private slots:
    void init() {
        QTest::failOnWarning(QRegularExpression(
            ".*(Binding loop|Unable to assign|Cannot assign|TypeError|ReferenceError).*"));
    }

    void example_data() {
        QTest::addColumn<int>("width");
        QTest::addColumn<int>("presetIndex");
        for (const int width : { 320, 900 })
            for (int preset = 0; preset < 3; ++preset)
                QTest::newRow(qPrintable(QString("%1-%2").arg(width).arg(preset)))
                    << width << preset;
    }

    void example() {
        QFETCH(int, width);
        QFETCH(int, presetIndex);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        const auto path = QFINDTESTDATA("../../example/compositions/Motion.qml");
        QVERIFY(! path.isEmpty());
        QQmlComponent            component(&engine, QUrl::fromLocalFile(path));
        std::unique_ptr<QObject> object(component.createWithInitialProperties(
            { { "width", width }, { "height", 640 }, { "presetIndex", presetIndex } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(root);
        QQuickWindow window;
        window.resize(width, 640);
        root->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        polish(root);
        QCoreApplication::processEvents();
        polish(root);
        QCOMPARE(root->property("progress").toReal(), 0.0);
        QVERIFY(! root->property("running").toBool());
        auto* controls = root->findChild<QQuickItem*>("motionControls");
        QVERIFY(controls);
        for (auto* child : controls->childItems()) {
            QVERIFY(child->x() >= 0);
            QVERIFY(child->x() + child->width() <= controls->width() + .1);
            QVERIFY(child->y() + child->height() <= controls->height() + .1);
        }
        auto* motion = root->findChild<SpringMotion*>("spatialMotion");
        QVERIFY(motion);
        const auto                     scheme = token::Motion {}.spring().expressive();
        const QList<token::SpringSpec> presets { scheme.fastSpatial(),
                                                 scheme.defaultSpatial(),
                                                 scheme.slowSpatial() };
        QCOMPARE(motion->spec(), presets[presetIndex]);
        QSignalSpy finished(motion, &SpringMotion::finished);
        auto*      play = root->findChild<QObject*>("playButton");
        QVERIFY(play);
        QVERIFY(QMetaObject::invokeMethod(play, "click"));
        QVERIFY(motion->isRunning());
        auto* animation = clock(motion);
        QVERIFY(animation);
        animation->setCurrentTime(100);
        const auto middle   = root->property("progress").toReal();
        const auto velocity = motion->velocity();
        QVERIFY(middle > 0);
        const auto directory = qEnvironmentVariable("QM_MOTION_CAPTURE_DIR");
        if (! directory.isEmpty()) {
            animation->pause();
            const auto frame = window.grabWindow();
            QVERIFY(! frame.isNull());
            QVERIFY(QDir().mkpath(directory));
            QVERIFY(
                frame.save(directory + QString("/motion-%1-%2.png").arg(width).arg(presetIndex)));
        }
        const int nextPreset = (presetIndex + 1) % 3;
        QVERIFY(root->setProperty("presetIndex", nextPreset));
        QCOMPARE(motion->spec(), presets[nextPreset]);
        QCOMPARE(motion->value(), middle);
        QCOMPARE(motion->velocity(), velocity);
        QVERIFY(QMetaObject::invokeMethod(root, "toggleTarget"));
        QCOMPARE(motion->value(), middle);
        QCOMPARE(motion->velocity(), velocity);
        QCOMPARE(finished.size(), 0);
        QVERIFY(QMetaObject::invokeMethod(root, "finish"));
        QCOMPARE(root->property("progress").toReal(), 0.0);
        QCOMPARE(finished.size(), 1);
        QVERIFY(QMetaObject::invokeMethod(root, "finish"));
        QCOMPARE(finished.size(), 1);
        QVERIFY(QMetaObject::invokeMethod(root, "toggleTarget"));
        animation->setCurrentTime(100);
        QVERIFY(QMetaObject::invokeMethod(root, "stop"));
        QVERIFY(! motion->isRunning());
        QCOMPARE(motion->velocity(), 0.0);
        QVERIFY(motion->value() != motion->targetValue());
        QVERIFY(QMetaObject::invokeMethod(root, "finish"));
        QCOMPARE(root->property("progress").toReal(), 1.0);
        QVERIFY(QMetaObject::invokeMethod(root, "toggleTarget"));
        animation->setCurrentTime(40);
        QVERIFY(motion->isRunning());
        QVERIFY(root->setProperty("animationsEnabled", false));
        QCOMPARE(root->property("progress").toReal(), 0.0);
        QVERIFY(! motion->isRunning());
        QVERIFY(QMetaObject::invokeMethod(root, "reset"));
        QCOMPARE(root->property("progress").toReal(), 0.0);
        QVERIFY(QMetaObject::invokeMethod(root, "toggleTarget"));
        QCOMPARE(root->property("progress").toReal(), 1.0);
        QVERIFY(! root->property("running").toBool());
        QVERIFY(root->setProperty("animationsEnabled", true));
        QVERIFY(QMetaObject::invokeMethod(root, "toggleTarget"));
        root->setVisible(false);
        QVERIFY(! root->property("running").toBool());
        QCOMPARE(root->property("progress").toReal(), 0.0);
        root->setVisible(true);
        QVERIFY(QMetaObject::invokeMethod(root, "toggleTarget"));
        window.hide();
        QVERIFY(! motion->isRunning());
        QCOMPARE(motion->value(), 1.0);
        window.show();
        QVERIFY(QMetaObject::invokeMethod(root, "toggleTarget"));
        QVERIFY(motion->isRunning());
        QPointer<SpringMotion> guard(motion);
        const int              before = finished.size();
        object.reset();
        QVERIFY(! guard);
        QCoreApplication::processEvents();
        QCOMPARE(finished.size(), before);
    }
};

int run_motion_example(int argc, char** argv) {
    MotionExampleTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "motion_example.moc"
