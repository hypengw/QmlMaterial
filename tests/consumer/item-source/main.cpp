#include "track.hpp"
#include <qml_material/model/item_source.hpp>
#include <qml_material/model/lazy_row.hpp>
#include <qml_material/view/lazy_list.hpp>
#include <QHash>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickRenderControl>
#include <QQuickWindow>
#include <QtTest>
#include <algorithm>
#include <vector>

using namespace qml_material;
struct Entry {
    QString key;
    Track   value;
    quint64 version = 1;
};
class TrackData final : public TypedItemDataSnapshot<Track> {
public:
    explicit TrackData(std::vector<Entry> rows): m_rows(std::move(rows)) {
        for (int i = 0; i < count(); ++i) m_keys.insert(m_rows[i].key, i);
    }
    int            count() const override { return int(m_rows.size()); }
    ItemDescriptor descriptorAt(int index) const override {
        const auto& row = m_rows.at(index);
        return { row.key, QStringLiteral("track"), row.version };
    }
    std::optional<int> indexOfKey(const QString& key) const override {
        const auto found = m_keys.constFind(key);
        return found == m_keys.cend() ? std::nullopt : std::optional<int>(*found);
    }
    Track itemAt(int index) const override {
        ++reads;
        return m_rows.at(index).value;
    }
    mutable int reads = 0;

private:
    const std::vector<Entry> m_rows;
    QHash<QString, int>      m_keys;
};
class TrackSource final : public ItemSource {
public:
    ItemPublishResult set(std::vector<Entry> rows, QVector<ItemChange> changes = {},
                          bool reset = true) {
        data = std::make_shared<TrackData>(std::move(rows));
        return publish(data, std::move(changes), reset);
    }
    std::shared_ptr<const TrackData> data;
};

class ConsumerTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void installedInterface_data() {
        QTest::addColumn<bool>("horizontal");
        QTest::newRow("vertical") << false;
        QTest::newRow("horizontal") << true;
    }
    void installedInterface() {
        QFETCH(bool, horizontal);
        TrackSource        source;
        std::vector<Entry> rows;
        for (int i = 0; i < 1000; ++i)
            rows.push_back({ QString::number(9007199254740993ULL + i), { QString::number(i) } });
        QVERIFY(source.set(rows));
        QCOMPARE(source.data->reads, 0);
        const auto old = source.snapshot();
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("consumerSource", &source);
        engine.rootContext()->setContextProperty("horizontal", horizontal);
        QQuickRenderControl render;
        QQuickWindow        window(&render);
        QQmlComponent       component(&engine);
        component.setData(R"(
            import QtQuick
            import Consumer
            import Qcm.Material as MD
            MD.LazyList {
                width: 300; height: 120; cacheExtent: 0
                orientation: horizontal ? Qt.Horizontal : Qt.Vertical
                source: consumerSource
                property int demandedFirst: -1
                onItemsNeeded: demand => { demandedFirst = demand.valid ? demand.first : -1; }
                delegate: Item {
                    required property MD.LazyRow row
                    property track entry: row.value
                    property string observed: entry.title
                    implicitHeight: 40
                    implicitWidth: 40
                }
            }
        )",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        auto view = std::unique_ptr<LazyList>(qobject_cast<LazyList*>(component.create()));
        QVERIFY(view);
        QCOMPARE(view->orientation(), horizontal ? Qt::Horizontal : Qt::Vertical);
        view->setParentItem(window.contentItem());
        auto frames = [&] {
            for (int i = 0; i < 10; ++i) {
                QCoreApplication::processEvents();
                render.polishItems();
            }
        };
        frames();
        QVERIFY(view->liveCount() > 0);
        QVERIFY(source.data->reads < 20);
        QVERIFY(view->currentDemand().valid());
        QCOMPARE(view->property("demandedFirst").toInt(), view->currentDemand().first());
        const auto find = [&]() -> QQuickItem* {
            for (auto host : view->contentItem()->childItems())
                for (auto item : host->childItems()) {
                    auto row = item->property("row").value<LazyRow*>();
                    if (row && row->active() && row->key() == "9007199254740993") return item;
                }
            return nullptr;
        };
        auto item = find();
        QVERIFY(item);
        QCOMPARE(item->property("observed").toString(), "0");
        rows[0].value.title = "updated";
        ++rows[0].version;
        std::rotate(rows.begin(), rows.begin() + 1, rows.begin() + 3);
        QVERIFY(source.set(
            rows, { { ItemChange::Update, 0, 1 }, { ItemChange::Move, 0, 1, 2 } }, false));
        frames();
        QCOMPARE(source.indexOfKey("9007199254740993"), 2);
        QVERIFY(source.data->reads < 20);
        QVERIFY(view->positionAtKey("9007199254740993"));
        frames();
        item = find();
        QVERIFY(item);
        QCOMPARE(item->property("observed").toString(), "updated");
        QCOMPARE(old->valueAt(0).value<Track>().title, "0");
        auto demand = view->currentDemand();
        view.reset();
        QVERIFY(! demand.valid());
    }
};
QTEST_MAIN(ConsumerTest)
#include "main.moc"
