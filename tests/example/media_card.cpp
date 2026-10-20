#include <QtTest/QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QAbstractItemModel>
#include <QJSValue>
#include <algorithm>
#include <cmath>
#include <limits>
#include <QDir>
#include <QMutex>
#include <private/qquickanimation_p.h>
#include "qml_material/control/button_control.hpp"
#include "qml_material/control/control.hpp"
#include "qml_material/control/slider.hpp"
#include "qml_material/input/interaction_source.hpp"

namespace
{
void polish(QQuickItem* item) {
    item->ensurePolished();
    for (auto* child : item->childItems()) polish(child);
}

void click(QObject* button) {
    QVERIFY(button);
    QVERIFY(QMetaObject::invokeMethod(button, "click"));
}

void verifyText(QQuickItem* item) {
    if (item->isVisible() && item->property("truncated").isValid())
        QVERIFY2(! item->property("truncated").toBool(),
                 qPrintable(item->property("text").toString()));
    for (auto* child : item->childItems()) verifyText(child);
}
} // namespace

class MediaCardTest : public QObject {
    Q_OBJECT

private slots:
    void init() {
        QTest::failOnWarning(
            QRegularExpression(".*(Binding loop|Unable to assign|Cannot "
                               "assign|Cannot open|TypeError|ReferenceError|polish.*loop).*"));
    }

    void mediaLayouts() {
        QQmlEngine   engine;
        QQuickWindow window;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine,
                                QUrl("qrc:/Qcm/Material/Example/compositions/MediaCard.qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(window.contentItem());
        click(root->findChild<QObject*>("previousTrack"));
        QCOMPARE(root->property("trackIndex").toInt(), 2);
        click(root->findChild<QObject*>("nextTrack"));
        QCOMPARE(root->property("trackIndex").toInt(), 0);
        click(root->findChild<QObject*>("playPause"));
        QVERIFY(root->property("playing").toBool());
        click(root->findChild<QObject*>("nextTrack"));
        QCOMPARE(root->property("trackIndex").toInt(), 1);
        auto* artwork = root->findChild<QQuickItem*>("mediaArtwork");
        QVERIFY(artwork);
        QTRY_COMPARE(artwork->property("status").toInt(), 1);
        for (int width : { 288, 640 }) {
            for (bool expanded : { false, true }) {
                root->setWidth(width);
                root->setProperty("expanded", expanded);
                root->setHeight(root->implicitHeight());
                polish(root);
                QCOMPARE(root->height(), expanded ? 184 : 128);
                if (expanded) verifyText(root);
                QCoreApplication::processEvents();
                polish(root);
                QCOMPARE(root->property("trackIndex").toInt(), 1);
                QVERIFY(root->property("playing").toBool());
                for (const auto* name :
                     { "mediaArtwork", "mediaTitle", "mediaControls", "mediaTimeline" }) {
                    auto* item = root->findChild<QQuickItem*>(name);
                    QVERIFY(item);
                    const auto bounds = item->mapRectToItem(root, item->boundingRect());
                    QVERIFY2(root->boundingRect().contains(bounds), name);
                }
                auto* seek = root->findChild<QQuickItem*>("mediaSeek");
                QCOMPARE(seek->isVisible(), expanded);
                for (const auto* name : { "playPause", "previousTrack", "nextTrack" }) {
                    auto* button = root->findChild<qml_material::ButtonControl*>(name);
                    QVERIFY(button);
                    QCOMPARE(button->size(), QSizeF(48, 48));
                    QCOMPARE(button->background()->size(), QSizeF(48, 48));
                    QCOMPARE(button->icon()->width(), 24);
                    QCOMPARE(button->padding(), 12);
                }
                QCOMPARE(artwork->size(), QSizeF(width, expanded ? 184 : 128));
                QCOMPARE(artwork->property("radius").toReal(), 28);
                QCOMPARE(qobject_cast<qml_material::Control*>(root)
                             ->background()
                             ->property("radius")
                             .toReal(),
                         28);
            }
        }
        click(root->findChild<QObject*>("playPause"));
        QVERIFY(! root->property("playing").toBool());
    }

    void mediaSeek() {
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(640, 400);
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine,
                                QUrl("qrc:/Qcm/Material/Example/compositions/MediaCard.qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(window.contentItem());
        root->setWidth(640);
        root->setHeight(root->implicitHeight());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        polish(root);
        auto* seek = root->findChild<qml_material::Slider*>("mediaSeek");
        auto* wave = root->findChild<QQuickItem*>("mediaWave");
        QVERIFY(seek && wave);
        QCOMPARE(root->height(), 184);
        QCOMPARE(seek->height(), 48);
        QCOMPARE(wave->height(), 48);
        QVERIFY(wave->isVisible());
        QCOMPARE(wave->width(), seek->width());
        QCOMPARE(wave->property("progress").toReal(), seek->position());
        QCOMPARE(wave->property("waveLength").toReal(), 20);
        QCOMPARE(wave->property("waveAmplitude").toReal(), 1.5);
        QCOMPARE(wave->property("waveSpeed").toReal(), 8);
        QVERIFY(! wave->property("transitionEnabled").toBool());
        QCOMPARE(seek->value(), 52);
        QCOMPARE(root->findChild<QObject*>("mediaElapsed")->property("text").toString(),
                 QString("0:52"));
        QCOMPARE(root->findChild<QObject*>("mediaDuration")->property("text").toString(),
                 QString("5:03"));
        QVERIFY(! wave->property("animate").toBool());
        QSignalSpy commits(seek, SIGNAL(seekRequested(double)));
        QVERIFY(commits.isValid());
        click(root->findChild<QObject*>("playPause"));
        QVERIFY(wave->property("animate").toBool());
        QTRY_COMPARE(wave->property("amplitudeFraction").toReal(), 1);
        const auto phaseBeforeFrame = wave->property("phase").toReal();
        QTRY_VERIFY(wave->property("phase").toReal() != phaseBeforeFrame);
        qml_material::InteractionAccumulator input(this);
        input.setSource(seek->interactionSource());
        const auto point =
            seek->mapToScene(QPointF(seek->width() * .7, seek->height() / 2)).toPoint();
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, point);
        QTRY_VERIFY(input.values().pressed);
        QVERIFY(! wave->property("animate").toBool());
        QCOMPARE(root->property("position").toReal(), 52);
        QCOMPARE(commits.size(), 0);
        QCOMPARE(seek->position(), 52.0 / 303.0);
        const auto dragPoint =
            seek->mapToScene(QPointF(seek->width() * .8, seek->height() / 2)).toPoint();
        QTest::mouseMove(&window, dragPoint);
        QVERIFY(seek->position() > .65);
        QCOMPARE(commits.size(), 0);
        QCOMPARE(wave->property("progress").toReal(), seek->position());
        QVERIFY(root->findChild<QQuickItem*>("mediaElapsed")->isVisible());
        QVERIFY(! root->findChild<QQuickItem*>("previousTrack")->isVisible());
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, dragPoint);
        QVERIFY(root->property("position").toReal() > 200);
        QCOMPARE(root->property("position").toReal(), seek->value());
        QCOMPARE(commits.size(), 1);
        QVERIFY(! root->findChild<QQuickItem*>("mediaElapsed")->isVisible());
        seek->forceActiveFocus();
        const auto before = seek->value();
        QTest::keyClick(&window, Qt::Key_Right);
        QCOMPARE(root->property("position").toReal(), before + 1);
        root->setProperty("expanded", false);
        QCOMPARE(seek->value(), before + 1);
        click(root->findChild<QObject*>("nextTrack"));
        QCOMPARE(root->property("position").toReal(), 0);
        QCOMPARE(seek->to(), 248);
        root->setProperty("expanded", true);
        seek->forceActiveFocus();
        root->setProperty("position", 248);
        QTest::keyClick(&window, Qt::Key_Right);
        QCOMPARE(root->property("position").toReal(), 248);
        root->setProperty("position", 0);
        QTest::keyClick(&window, Qt::Key_Left);
        QCOMPARE(root->property("position").toReal(), 0);
        root->setProperty("position", 52);
        QTRY_VERIFY(wave->property("animating").toBool());
        root->setVisible(false);
        QVERIFY(! wave->property("animating").toBool());
        root->setVisible(true);
        QTRY_VERIFY(wave->property("animating").toBool());
        click(root->findChild<QObject*>("playPause"));
        QVERIFY(! wave->property("animate").toBool());
        const auto stoppedPhase = wave->property("phase").toReal();
        QTRY_COMPARE(wave->property("amplitudeFraction").toReal(), 0);
        QCOMPARE(wave->property("phase").toReal(), stoppedPhase);
        QVERIFY(! wave->property("animating").toBool());
        QCOMPARE(wave->property("progress").toReal(), seek->position());
        const auto committed = commits.size();
        const auto cancelPoint =
            seek->mapToScene(QPointF(seek->width() * .2, seek->height() / 2)).toPoint();
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, cancelPoint);
        QCOMPARE(root->property("position").toReal(), 52);
        QTest::keyClick(&window, Qt::Key_Escape);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, cancelPoint);
        QCOMPARE(root->property("position").toReal(), 52);
        QCOMPARE(commits.size(), committed);
        root->setProperty("seekAvailable", false);
        QVERIFY(! seek->isEnabled());
        QVERIFY(! seek->handle()->isVisible());
        QVERIFY(wave->property("transitionEnabled").toBool());
        click(root->findChild<QObject*>("playPause"));
        QTRY_COMPARE(wave->property("amplitudeFraction").toReal(), 1);
        QVERIFY(wave->property("animating").toBool());
        seek->setProperty("listening", false);
        QVERIFY(! wave->property("animate").toBool());
        QTRY_VERIFY(! wave->property("animating").toBool());
    }
};

int run_media_card(int argc, char** argv) {
    MediaCardTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "media_card.moc"
