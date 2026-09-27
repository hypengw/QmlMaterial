#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <array>
#include <cmath>
#include <memory>
#include <numbers>

namespace
{
constexpr int origin = 64;
constexpr int width  = 100;
constexpr int height = 60;

bool contains(double x, double y, const std::array<double, 4>& radii) {
    if (x < 0 || x > width || y < 0 || y > height) return false;
    const int    corner = (x > width / 2 ? 1 : 0) + (y > height / 2 ? 2 : 0);
    const double r      = radii[corner];
    const double dx     = std::max(r - std::min(x, width - x), 0.0);
    const double dy     = std::max(r - std::min(y, height - y), 0.0);
    return dx * dx + dy * dy <= r * r;
}

double reference(double px, double py, double sigma, const std::array<double, 4>& radii) {
    if (sigma == 0) return contains(px, py, radii) ? 1 : 0;
    constexpr double step = .25;
    double           sum  = 0;
    for (double y = std::max(0.0, py - 4 * sigma) + step / 2;
         y < std::min(double(height), py + 4 * sigma);
         y += step) {
        for (double x = std::max(0.0, px - 4 * sigma) + step / 2;
             x < std::min(double(width), px + 4 * sigma);
             x += step) {
            if (! contains(x, y, radii)) continue;
            const double dx = x - px;
            const double dy = y - py;
            sum += std::exp(-(dx * dx + dy * dy) / (2 * sigma * sigma));
        }
    }
    return sum * step * step / (2 * std::numbers::pi * sigma * sigma);
}

double coverage(const QImage& image, int x, int y) {
    const qreal dpr = image.devicePixelRatio();
    return 1 - image.pixelColor(qRound((origin + x) * dpr), qRound((origin + y) * dpr)).redF();
}

double samplePosition(const QImage& image, int coordinate) {
    const qreal dpr = image.devicePixelRatio();
    return (qRound((origin + coordinate) * dpr) + .5) / dpr - origin;
}
} // namespace

class BlurMaskTest : public QObject {
    Q_OBJECT
private slots:
    void corners_data() {
        QTest::addColumn<double>("sigma");
        QTest::addColumn<double>("blurSigma");
        QTest::addColumn<QVector4D>("radii");
        QTest::newRow("asymmetric") << 7.0 << 7.0 << QVector4D(8, 30, 8, 30);
        QTest::newRow("four-distinct") << 7.0 << 7.0 << QVector4D(0, 30, 8, 16);
        QTest::newRow("overlapping-tails") << 18.0 << 18.0 << QVector4D(30, 8, 24, 0);
        QTest::newRow("uniform") << 7.0 << 7.0 << QVector4D(30, 30, 30, 30);
        QTest::newRow("square") << 7.0 << 7.0 << QVector4D(0, 0, 0, 0);
        QTest::newRow("fractional") << 3.5 << 3.5 << QVector4D(4.5, 21.5, 11.5, 0);
        QTest::newRow("low-elevation") << 1.2 << 1.0 << QVector4D(8, 30, 8, 30);
    }

    void corners() {
        QFETCH(double, sigma);
        QFETCH(double, blurSigma);
        QFETCH(QVector4D, radii);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.BlurMaskImpl {
                property real tl: 0
                property real tr: 0
                property real bl: 0
                property real br: 0
                x: 64; y: 64; width: 100; height: 60
                corners.topLeft: tl
                corners.topRight: tr
                corners.bottomLeft: bl
                corners.bottomRight: br
                color: "black"
                style: MD.BlurMaskImpl.Normal
            }
        )",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QQuickWindow window;
        window.setColor(Qt::white);
        window.resize(228, 188);
        std::unique_ptr<QObject> object(component.create());
        auto*                    item = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(item);
        item->setParentItem(window.contentItem());
        const std::array<const char*, 4> properties { "tl", "tr", "bl", "br" };
        std::array<double, 4>            corners;
        for (int i = 0; i < 4; ++i) {
            corners[i] = radii[i];
            QVERIFY(item->setProperty(properties[i], corners[i]));
        }
        QVERIFY(item->setProperty("sigma", sigma));
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        const auto image = window.grabWindow();
        QVERIFY(! image.isNull());
        QVERIFY(window.rendererInterface()->graphicsApi() != QSGRendererInterface::Software);

        double maxError = 0;
        QPoint worstPoint;
        for (int y = -12; y <= height + 12; y += 6) {
            for (int x = -12; x <= width + 12; x += 6) {
                const double expected = reference(
                    samplePosition(image, x), samplePosition(image, y), blurSigma, corners);
                const double error = std::abs(coverage(image, x, y) - expected);
                if (expected < .0001) QVERIFY(coverage(image, x, y) < .01);
                if (error > maxError) {
                    maxError   = error;
                    worstPoint = QPoint(x, y);
                }
            }
        }
        // The one-pixel LUT has larger interpolation error at small sigma.
        QVERIFY2(maxError < (blurSigma <= 1 ? .04 : .025),
                 qPrintable(QString("error %1 at (%2, %3)")
                                .arg(maxError)
                                .arg(worstPoint.x())
                                .arg(worstPoint.y())));

        // Mutate the same node, then restore it without relying on animation timing.
        for (const auto* property : properties) QVERIFY(item->setProperty(property, 0.0));
        QVERIFY(item->setProperty("sigma", sigma + 4));
        const auto changed = window.grabWindow();
        QVERIFY(! changed.isNull());
        QVERIFY(image != changed);
        for (int i = 0; i < 4; ++i) QVERIFY(item->setProperty(properties[i], corners[i]));
        QVERIFY(item->setProperty("sigma", sigma));
        QCOMPARE(window.grabWindow(), image);

        QVERIFY(item->setProperty("style", 1));
        const auto solid = window.grabWindow();
        QVERIFY(item->setProperty("style", 2));
        const auto outer = window.grabWindow();
        QVERIFY(item->setProperty("style", 3));
        const auto inner = window.grabWindow();
        QVERIFY(! solid.isNull() && ! outer.isNull() && ! inner.isNull());
        for (int y = -12; y <= height + 12; y += 6) {
            for (int x = -12; x <= width + 12; x += 6) {
                const double normal = coverage(image, x, y);
                QVERIFY(coverage(solid, x, y) + .005 >= normal);
                QVERIFY(std::abs(coverage(outer, x, y) + coverage(inner, x, y) - normal) < .01);
            }
        }
    }
};

int run_visual_blur_mask(int argc, char** argv) {
    BlurMaskTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "visual_blur_mask.moc"
