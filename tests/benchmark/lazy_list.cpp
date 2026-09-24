#include <QGuiApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickRenderControl>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <algorithm>
#include <cmath>
#include <memory>
#ifdef Q_OS_LINUX
#    include <sys/resource.h>
#endif

static qint64 peakRssKiB() {
#ifdef Q_OS_LINUX
    rusage usage {};
    if (getrusage(RUSAGE_SELF, &usage) == 0) return usage.ru_maxrss;
#endif
    return -1;
}

static qint64 residentKiB() {
#ifdef Q_OS_LINUX
    QFile status("/proc/self/status");
    if (status.open(QIODevice::ReadOnly))
        for (const auto& line : status.readAll().split('\n'))
            if (line.startsWith("VmRSS:"))
                return line.simplified().split(' ').value(1).toLongLong();
#endif
    return -1;
}

static double uncoveredHeight(QQuickItem* view) {
    QList<QPair<double, double>> spans;
    const auto                   collect = [&](auto&& self, QQuickItem* parent) -> void {
        for (auto* item : parent->childItems()) {
            if (! item->isVisible()) continue;
            if (item->objectName() == "row") {
                const double top = item->mapToItem(view, QPointF()).y();
                spans.append(
                    { std::max(0.0, top), std::min(view->height(), top + item->height()) });
            } else
                self(self, item);
        }
    };
    collect(collect, view);
    std::sort(spans.begin(), spans.end());
    double end = 0, covered = 0;
    for (const auto& span : spans) {
        if (span.second <= std::max(end, span.first)) continue;
        covered += span.second - std::max(end, span.first);
        end = span.second;
    }
    return std::max(0.0, view->height() - covered);
}

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    const auto      args = app.arguments();
    if (args.size() < 3 || (args[1] != "lazy" && args[1] != "qt") ||
        (args[2] != "fixed" && args[2] != "variable")) {
        qCritical("Usage: lazy_list_benchmark lazy|qt fixed|variable [rows=100000] [steps=2000]");
        return 2;
    }
    const bool lazy     = args[1] == "lazy";
    const bool variable = args[2] == "variable";
    const int  count    = args.size() > 3 ? args[3].toInt() : 100000;
    const int  steps    = args.size() > 4 ? args[4].toInt() : 2000;
    if (count < 100 || steps < 1) return 2;
    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/../qml_modules");
    QQuickRenderControl render;
    QQuickWindow        window(&render);
    const QByteArray    viewType = lazy ? "MD.LazyList" : "ListView";
    const QByteArray    model =
        lazy ? "source: MD.ListSnapshotSource { keyRole: 'key'; items: root.dataRows }\n"
               "cacheExtent: 200; estimatedItemExtent: 48; poolLimit: 16\n"
             : "model: root.dataRows; cacheBuffer: 200; reuseItems: true\n";
    const QByteArray row =
        lazy ? "required property MD.LazyRow row; property var entry: row.value\n"
               "Connections { target: row; function onReused() { root.reused++ } }\n"
             : "required property var modelData; property var entry: modelData\n"
               "ListView.onReused: root.reused++\n";
    QQmlComponent component(&engine);
    component.setData(
        "import QtQuick\nimport Qcm.Material as MD\nItem { id: root; "
        "width: 400; height: 600; property var dataRows: []; property int created: 0; "
        "property int destroyed: 0; property int reused: 0; " +
            viewType + " { objectName: 'view'; anchors.fill: parent; clip: true; " + model +
            "delegate: Rectangle { objectName: 'row'; " + row +
            "width: 400; height: entry.height; color: '#eeeeee'; "
            "Component.onCompleted: root.created++; Component.onDestruction: root.destroyed++; "
            "Text { x: 8; y: 8; text: entry.text; font.pixelSize: 14 } } } }",
        QUrl());
    std::unique_ptr<QObject> object(component.create());
    if (! object) {
        qCritical() << component.errors();
        return 1;
    }
    auto* root = qobject_cast<QQuickItem*>(object.get());
    auto* view = root->findChild<QQuickItem*>("view");
    if (! view) return 1;
    root->setParentItem(window.contentItem());
    const auto polish = [&] {
        QCoreApplication::processEvents();
        render.polishItems();
    };
    QVariantList data;
    double       total = 0;
    for (int i = 0; i < count; ++i) {
        const int height = variable ? 32 + i % 8 * 8 : 48;
        total += height;
        data.append(QVariantMap { { "key", QString::number(i) },
                                  { "height", height },
                                  { "text", QString("Row %1").arg(i) } });
    }
    const auto    baselineRss = peakRssKiB();
    QElapsedTimer timer;
    timer.start();
    root->setProperty("dataRows", data);
    const auto adapterNs = timer.nsecsElapsed();
    timer.restart();
    for (int i = 0; i < 12; ++i) polish();
    const auto    initialNs = timer.nsecsElapsed();
    QList<qint64> samples;
    QJsonArray    memory;
    int           peakLive        = 0;
    double        firstFrameBlank = 0, settledBlank = 0;
    const double  travel = total - 600;
    for (int step = 0; step < steps; ++step) {
        const double phase  = std::fmod(step * 137.0, travel * 2);
        const double offset = phase <= travel ? phase : travel * 2 - phase;
        timer.restart();
        view->setProperty("contentY", offset);
        polish();
        const auto firstNs = timer.nsecsElapsed();
        firstFrameBlank    = std::max(firstFrameBlank, uncoveredHeight(view));
        timer.restart();
        for (int i = 0; i < 3; ++i) polish();
        samples.append(firstNs + timer.nsecsElapsed());
        settledBlank = std::max(settledBlank, uncoveredHeight(view));
        peakLive     = std::max(
            peakLive, root->property("created").toInt() - root->property("destroyed").toInt());
        if (step % std::max(1, steps / 20) == 0 || step == steps - 1)
            memory.append(QJsonObject {
                { "step", step },
                { "rss_kib", residentKiB() },
                { "delegate_objects",
                  root->property("created").toInt() - root->property("destroyed").toInt() } });
    }
    std::sort(samples.begin(), samples.end());
    const auto finalRss = residentKiB();
    engine.collectGarbage();
    const auto collectedRss = residentKiB();
    QJsonObject result { { "view", args[1] },
                         { "heights", args[2] },
                         { "qt", qVersion() },
                         { "build_type", QM_BENCHMARK_BUILD_TYPE },
                         { "rows", count },
                         { "steps", steps },
                         { "adapter_assignment_ms", adapterNs / 1e6 },
                         { "initial_polish_ms", initialNs / 1e6 },
                         { "initialization_ms", (adapterNs + initialNs) / 1e6 },
                         { "step_median_ms", samples[samples.size() / 2] / 1e6 },
                         { "step_p95_ms",
                           samples[std::min(samples.size() - 1, samples.size() * 95 / 100)] / 1e6 },
                         { "created", root->property("created").toInt() },
                         { "destroyed", root->property("destroyed").toInt() },
                         { "reused", root->property("reused").toInt() },
                         { "peak_delegate_objects", peakLive },
                         { "baseline_peak_rss_kib", baselineRss },
                         { "peak_rss_kib", peakRssKiB() },
                         { "final_rss_kib", finalRss },
                         { "after_gc_rss_kib", collectedRss },
                         { "memory_samples", memory },
                         { "max_first_polish_uncovered_px", firstFrameBlank },
                         { "max_settled_uncovered_px", settledBlank } };
    QTextStream(stdout) << QJsonDocument(result).toJson(QJsonDocument::Compact) << '\n';
    return settledBlank > 0.01 ? 1 : 0;
}
