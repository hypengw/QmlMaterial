#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QtQuick/private/qquickpath_p.h>
#include <QtQuick/private/qquickanimation_p.h>
#include <QPainterPathStroker>
#include <QQuickWindow>
#include <QQuickItem>
#include <cmath>
#include <limits>
#include "qml_material/shape/material_shapes.hpp"
#include "qml_material/anim/loading_indicator_updator.hpp"
#include "qml_material/control/progress_indicator.hpp"

using namespace qml_material;

class MaterialShapesTest : public QObject {
    Q_OBJECT
private:
    static bool finite(const QPainterPath& path) {
        for (int i = 0; i < path.elementCount(); ++i)
            if (! std::isfinite(path.elementAt(i).x) || ! std::isfinite(path.elementAt(i).y))
                return false;
        return true;
    }
    static bool sameOutline(const QPainterPath& a, const QPainterPath& b) {
        QPainterPathStroker stroker;
        stroker.setWidth(.003);
        const auto outline = stroker.createStroke(b);
        for (int i = 0; i < 150; ++i)
            if (! outline.contains(a.pointAtPercent(qreal(i) / 150))) return false;
        return true;
    }
private slots:
    void cubicAndRounding() {
        const Cubic cubic { { 0, 0 }, { 0, 1 }, { 1, 1 }, { 1, 0 } };
        const auto [left, right] = cubic.split(.37);
        for (int i = 0; i <= 10; ++i) {
            const qreal t = qreal(i) / 10;
            QVERIFY(QLineF(left.point(t), cubic.point(t * .37)).length() < 1e-10);
            QVERIFY(QLineF(right.point(t), cubic.point(.37 + t * .63)).length() < 1e-10);
        }
        const RoundedPolygon square({ { { 1, 1 }, { .2, .6 } },
                                      { { -1, 1 }, { .4, .2 } },
                                      { { -1, -1 }, { .3, 1 } },
                                      { { 1, -1 }, { .2 } } });
        const auto           curves = square.cubics();
        QVERIFY(! curves.empty());
        for (size_t i = 0; i < curves.size(); ++i)
            QVERIFY(QLineF(curves[i].anchor1, curves[(i + 1) % curves.size()].anchor0).length() <
                    .0002);
        const auto transform = QTransform().translate(10, 20).scale(3, 2);
        QVERIFY(sameOutline(square.transformed(transform).path(), transform.map(square.path())));
        QCOMPARE(square.transformed(transform).center(), QPointF(10, 20));
        QVERIFY_EXCEPTION_THROWN(RoundedPolygon({ { { 0, 0 }, {} }, { { 1, 1 }, {} } }),
                                 std::invalid_argument);
    }
    void roundingSpaceUsage() {
        const RoundedPolygon polygon(
            { { { 0, 0 }, { 1, 0 } }, { { 1, 0 }, { 1, 1 } }, { { .5, 1 }, {} } });
        const auto& edge = polygon.features()[1].cubics.front();
        QVERIFY(QLineF(edge.anchor0, QPointF(.5, 0)).length() < 1e-4);
        QVERIFY(QLineF(edge.anchor1, QPointF(.5, 0)).length() < 1e-4);
    }
    void namedShapes() {
        QCOMPARE(QMetaEnum::fromType<MaterialShape::Type>().keyCount() - 1, MaterialShapes::count);
        QCOMPARE(MaterialShapes::loadingSequence().front(), MaterialShape::SoftBurst);
        QCOMPARE(MaterialShapes::loadingSequence().back(), MaterialShape::Oval);
        for (int i = 0; i < MaterialShapes::count; ++i) {
            const auto  type  = static_cast<MaterialShape::Type>(i);
            const auto& shape = MaterialShapes::polygon(type);
            const auto  path  = shape.path();
            QVERIFY2(! path.isEmpty() && finite(path),
                     QMetaEnum::fromType<MaterialShape::Type>().valueToKey(i));
            const auto bounds = path.controlPointRect();
            QVERIFY(QRectF(-.0001, -.0001, 1.0002, 1.0002).contains(bounds));
            QVERIFY(std::abs(std::max(bounds.width(), bounds.height()) - 1) < .0001);
            QCOMPARE(path.currentPosition(), QPointF(path.elementAt(0).x, path.elementAt(0).y));
            const auto radial = MaterialShapes::polygon(type, true).path();
            for (int p = 0; p < 200; ++p)
                QVERIFY(QLineF(QPointF(.5, .5), radial.pointAtPercent(qreal(p) / 200)).length() <
                        .505);
        }
    }
    void allMorphPairs() {
        for (bool radial : { false, true })
            for (int i = 0; i < MaterialShapes::count; ++i)
                for (int j = 0; j < MaterialShapes::count; ++j) {
                    const auto a     = static_cast<MaterialShape::Type>(i),
                               b     = static_cast<MaterialShape::Type>(j);
                    const auto morph = MaterialShapes::morph(a, b, radial);
                    QVERIFY(morph->curveCount() > 0);
                    for (qreal t : { -.05, 0.0, .25, .5, .75, 1.0, 1.05 }) {
                        const auto path = morph->path(t);
                        QVERIFY2(finite(path),
                                 qPrintable(QString("%1 -> %2 at %3").arg(i).arg(j).arg(t)));
                        const auto curves = morph->cubics(t);
                        for (size_t k = 0; k < curves.size(); ++k)
                            QCOMPARE(curves[k].anchor1, curves[(k + 1) % curves.size()].anchor0);
                    }
                    for (int endpoint = 0; endpoint < 2; ++endpoint) {
                        const auto expected =
                            MaterialShapes::polygon(endpoint ? b : a, radial).path().boundingRect();
                        const auto actual = morph->path(endpoint).boundingRect();
                        QVERIFY(QLineF(expected.topLeft(), actual.topLeft()).length() < .003);
                        QVERIFY(QLineF(expected.bottomRight(), actual.bottomRight()).length() <
                                .003);
                    }
                }
    }
    void morphEndpointsAndCache() {
        for (const auto a :
             { MaterialShape::Heart, MaterialShape::PixelCircle, MaterialShape::SoftBoom }) {
            const auto b     = MaterialShape::Pill;
            const auto morph = MaterialShapes::morph(a, b);
            QCOMPARE(morph, MaterialShapes::morph(a, b));
            QVERIFY(sameOutline(morph->path(0), MaterialShapes::polygon(a).path()));
            QVERIFY(sameOutline(MaterialShapes::polygon(a).path(), morph->path(0)));
            QVERIFY(sameOutline(morph->path(1), MaterialShapes::polygon(b).path()));
            QVERIFY(sameOutline(MaterialShapes::polygon(b).path(), morph->path(1)));
            QVERIFY(sameOutline(morph->path(.5), morph->path(.500001)));
        }
    }
    void qmlPath() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import QtQuick.Shapes
            import Qcm.Material as MD
            MD.MaterialShapePath {
                shape: MD.MaterialShape.Heart
                toShape: MD.MaterialShape.Cookie9Sided
                size: Qt.size(120, 80)
                fillColor: "red"
                strokeWidth: 0
            }
        )",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        auto*                    path = qobject_cast<QQuickPath*>(object.get());
        QVERIFY(path);
        QSignalSpy changes(path, &QQuickPath::changed);
        auto       original = path->path();
        QVERIFY(object->setProperty("progress", .5));
        QVERIFY(changes.count() > 0);
        QVERIFY(path->path() != original);
        object->setProperty("progress", 0);
        QCOMPARE(path->path(), original);
        object->setProperty("size", QSizeF(240, 160));
        QCOMPARE(path->path(), QTransform().scale(2, 2).map(original));
        object->setProperty("startAngle", 90);
        QVERIFY(finite(path->path()));
        object->setProperty("toShape", MaterialShape::None);
        original = path->path();
        object->setProperty("progress", .8);
        QCOMPARE(path->path(), original);
        object->setProperty("progress", std::numeric_limits<double>::quiet_NaN());
        QCOMPARE(object->property("progress").toDouble(), .8);
        object->setProperty("shape", 1000);
        QCOMPARE(object->property("shape").toInt(), int(MaterialShape::Heart));
    }
    void waveAnimationLifecycle_data() {
        QTest::addColumn<QString>("type");
        QTest::newRow("linear") << QStringLiteral("LinearIndicator");
        QTest::newRow("circular") << QStringLiteral("CircularIndicator");
    }
    void waveAnimationLifecycle() {
        QFETCH(QString, type);
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(QString("import QtQuick\nimport Qcm.Material as MD\n"
                                  "MD.%1 { wavy: true; indeterminate: true; running: false }")
                              .arg(type)
                              .toUtf8(),
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* indicator = qobject_cast<ProgressIndicator*>(object.get());
        QVERIFY(indicator);
        QQuickWindow window;
        window.resize(240, 80);
        indicator->setParentItem(window.contentItem());
        window.show();
        QVERIFY(! indicator->animating());
        object->setProperty("running", true);
        QTRY_VERIFY(indicator->animating());
        object->setProperty("running", false);
        QTRY_VERIFY(! indicator->animating());
        object->setProperty("running", true);
        QVERIFY(indicator->animating());
        object->setProperty("enabled", false);
        QVERIFY(! indicator->animating());
        object->setProperty("enabled", true);
        QVERIFY(indicator->animating());
        object->setProperty("running", false);
        QTRY_VERIFY(! indicator->animating());
        object->setProperty("indeterminate", false);
        object->setProperty("value", .5);
        QTRY_VERIFY(indicator->animating());
        object->setProperty("completionBehavior", ProgressIndicator::Keep);
        object->setProperty("value", 1.);
        QTRY_VERIFY(! indicator->animating());
        object->setProperty("value", 0.);
        QTRY_VERIFY(! indicator->animating());
        object->setProperty("value", .5);
        object->setProperty("wavy", false);
        QTRY_VERIFY(! indicator->animating());
    }
    void loadingAnimation() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQuickWindow  window;
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.BusyIndicator { width: 80; height: 80; colors: ["red"]; running: true }
        )",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        auto*                    item = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(item);
        item->setParentItem(window.contentItem());
        auto* updater = item->findChild<LoadingIndicatorUpdator*>();
        QVERIFY(updater);
        QSignalSpy updates(updater, &LoadingIndicatorUpdator::updated);
        window.resize(80, 80);
        window.show();
        QTRY_VERIFY(updates.count() >= 2);
        QVERIFY(updater->progress() > 0);
        QVERIFY(item->isVisible());
        item->setProperty("running", false);
        QVERIFY(! item->isVisible());
        item->setProperty("running", true);
        const auto count = updates.count();
        QTRY_VERIFY(updates.count() > count);
    }
    void loadingContinuity() {
        LoadingIndicatorUpdator updater;
        updater.setColors({ Qt::red, Qt::blue, Qt::green });
        for (int i = 1; i <= 15; ++i) {
            updater.setProgress(i - 1e-7);
            const auto target   = updater.toShape();
            const auto rotation = updater.rotation();
            const auto color    = updater.color();
            QVERIFY(std::abs(updater.shapeProgress() - 1) < 1e-6);
            updater.setProgress(i);
            QCOMPARE(updater.shape(), target);
            QCOMPARE(updater.shapeProgress(), 0.0);
            QVERIFY(std::abs(std::remainder(updater.rotation() - rotation, 360)) < .001);
            QCOMPARE(updater.color(), color);
        }
        updater.setProgress(-1);
        QCOMPARE(updater.progress(), 15.0);
        updater.setProgress(std::numeric_limits<double>::infinity());
        QCOMPARE(updater.progress(), 15.0);
        updater.setColors({});
        QCOMPARE(updater.color(), QColor(Qt::transparent));
    }
};

int run_material_shapes(int argc, char** argv) {
    MaterialShapesTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "material_shapes.moc"
