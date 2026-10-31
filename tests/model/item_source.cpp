#include <QtTest>
#include <limits>
#include <QQmlEngine>
#include <QQmlComponent>
#include "qml_material/model/item_source.hpp"
#include "delegate_recycler.hpp"
#include "viewport_controller.hpp"

using namespace qml_material;

class TestSource : public ItemSource {
public:
    using ItemSource::publish;
    using ItemSource::publishValues;
    ItemPublishResult replace(QVector<ItemRecord> rows, QVector<ItemChange> changes = {},
                              bool reset = true) {
        return publish(makeItemDataSnapshot(std::move(rows), true, publicationBase()),
                       std::move(changes),
                       reset);
    }
};

class TypedRow {
    Q_GADGET
    Q_PROPERTY(QString title MEMBER title)
public:
    QString title;
};
Q_DECLARE_METATYPE(TypedRow)

class CountingData : public TypedItemDataSnapshot<TypedRow> {
public:
    mutable int    reads       = 0;
    mutable int    descriptors = 0, indexes = 0;
    int            total           = 1000;
    bool           badIndex        = false;
    bool           badKey          = false;
    quint64        contentRevision = 1;
    int            count() const override { return total; }
    ItemDescriptor descriptorAt(int index) const override {
        ++descriptors;
        return { badKey ? QString() : QString::number(quint64(9007199254740993ULL) + index),
                 QStringLiteral("track"),
                 contentRevision };
    }
    std::optional<int> indexOfKey(const QString& key) const override {
        ++indexes;
        bool       ok    = false;
        const auto value = key.toULongLong(&ok);
        if (! ok || value < 9007199254740993ULL || value - 9007199254740993ULL >= quint64(total))
            return {};
        return badIndex ? 9999 : int(value - 9007199254740993ULL);
    }
    TypedRow itemAt(int index) const override {
        ++reads;
        return { QString::number(index) };
    }
};

class ItemSourceTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void valuePublicationSharesUnchangedRows() {
        TestSource source;
        auto       data = std::make_shared<CountingData>();
        data->total     = 100000;
        QVERIFY(source.publish(data));
        auto        before    = source.snapshot();
        const auto* unchanged = &before->descriptorAt(500);
        data->reads = data->descriptors = data->indexes = 0;
        QSignalSpy commits(&source, &ItemSource::committed);
        QVERIFY(source.publishValues(
            { { 99999, QVariant::fromValue(TypedRow { "last" }) },
              { 127, QVariant::fromValue(TypedRow { "a" }), QStringLiteral("alternate") },
              { 128, QVariant::fromValue(TypedRow { "b" }) },
              { 3, QVariant::fromValue(TypedRow { "first" }) } }));
        QCOMPARE(data->reads, 0);
        QCOMPARE(data->descriptors, 0);
        QCOMPARE(data->indexes, 0);
        QCOMPARE(source.count(), 100000);
        QCOMPARE(&source.snapshot()->descriptorAt(500), unchanged);
        QCOMPARE(source.valueAt(3).value<TypedRow>().title, "first");
        QCOMPARE(source.snapshot()->descriptorAt(127).reuseType, "alternate");
        QCOMPARE(source.snapshot()->descriptorAt(3).reuseType, "track");
        QCOMPARE(before->descriptorAt(127).reuseType, "track");
        QCOMPARE(before->valueAt(3).value<TypedRow>().title, "3");
        const auto change = qvariant_cast<ItemChangeSet>(commits.front().front());
        QVERIFY(! change.reset);
        QCOMPARE(change.changes.size(), 3);
        QCOMPARE(change.changes[1].index, 127);
        QCOMPARE(change.changes[1].count, 2);
        const auto* first = &source.snapshot()->descriptorAt(3);
        QVERIFY(source.publishValues({ { 128, QVariant::fromValue(TypedRow { "new" }) } }));
        QCOMPARE(&source.snapshot()->descriptorAt(3), first);
        QCOMPARE(change.after->valueAt(128).value<TypedRow>().title, "b");
        QCOMPARE(source.valueAt(128).value<TypedRow>().title, "new");
    }
    void invalidValuePublicationKeepsSnapshot() {
        TestSource source;
        QVERIFY(source.replace({ { "a", "type", 1 }, { "b", "type", 2 } }));
        auto before = source.snapshot();
        QVERIFY(! source.publishValues({ { -1, 3 } }));
        QVERIFY(! source.publishValues({ { 2, 3 } }));
        QVERIFY(! source.publishValues({ { 0, 3 }, { 0, 4 } }));
        QVERIFY(! source.publishValues({ { 0, 3, {}, before->descriptorAt(0).contentRevision } }));
        QCOMPARE(source.snapshot(), before);
        QVERIFY(source.publishValues({}));
        QCOMPARE(source.snapshot(), before);
        QVERIFY(source.publishValues({ { 0, QVariant(), QStringLiteral("other") } }));
        QVERIFY(! source.valueAt(0).isValid());
        QCOMPARE(source.snapshot()->descriptorAt(0).key, "a");
        QCOMPARE(source.snapshot()->descriptorAt(0).reuseType, "other");
        QVERIFY(source.snapshot()->descriptorAt(0).contentRevision >
                before->descriptorAt(0).contentRevision);
        QVERIFY(source.replace({ { "a", {}, 1, std::numeric_limits<quint64>::max() } }));
        QVERIFY(! source.publishValues({ { 0, 4 } }));
    }
    void valuePublicationHasNoSnapshotChain() {
        TestSource source;
        QVERIFY(source.replace({ { "a", {}, 1 }, { "b", {}, 2 } }));
        for (int i = 0; i < 1000; ++i) {
            std::weak_ptr<const ItemSnapshot> obsolete = source.snapshot();
            QVERIFY(source.publishValues({ { 0, i } }));
            QVERIFY(obsolete.expired());
        }
        QCOMPARE(source.valueAt(0).toInt(), 999);
        QCOMPARE(source.valueAt(1).toInt(), 2);
    }
    void reentrantValuePublication() {
        TestSource source;
        QVERIFY(source.replace({ { "a", {}, 0 } }));
        int commits = 0;
        connect(&source, &ItemSource::committed, &source, [&](const ItemChangeSet& change) {
            ++commits;
            QCOMPARE(source.snapshot(), change.after);
            if (commits == 1) {
                QVERIFY(source.publishValues({ { 0, 2 } }));
                QCOMPARE(source.valueAt(0).toInt(), 1);
            }
        });
        QVERIFY(source.publishValues({ { 0, 1 } }));
        QCOMPARE(commits, 2);
        QCOMPARE(source.valueAt(0).toInt(), 2);
    }
    void reentrantObservers() {
        TestSource      source;
        QObject         observer;
        int             first = 0, second = 0, states = 0;
        ItemSnapshotPtr queued;
        connect(&source, &ItemSource::committed, &observer, [&](const ItemChangeSet& change) {
            ++first;
            QCOMPARE(source.snapshot(), change.after);
            if (first == 1) {
                QVERIFY(source.replace({ { "b", {}, 2 } }));
                QVERIFY(source.replace({ { "c", {}, 3 } }));
                QCOMPARE(source.snapshot(), change.after);
            }
            if (first == 2) queued = change.after;
            if (first == 3) QCOMPARE(change.before, queued);
        });
        connect(&source, &ItemSource::committed, &observer, [&](const ItemChangeSet& change) {
            ++second;
            QCOMPARE(source.snapshot(), change.after);
            QCOMPARE(source.revision(), change.after->revision);
            QCOMPARE(source.count(), change.after->count());
        });
        connect(&source, &ItemSource::stateChanged, &observer, [&] {
            ++states;
        });
        QVERIFY(source.replace({ { "a", {}, 1 } }));
        QCOMPARE(first, 3);
        QCOMPARE(second, 3);
        QCOMPARE(states, 3);
        QCOMPARE(source.indexOfKey("c"), 0);
    }
    void rejectionKeepsData() {
        TestSource source;
        QVERIFY(source.replace({ { "a", {}, 1 }, { "b", {}, 2 } }));
        const auto before = source.snapshot();
        QSignalSpy commits(&source, &ItemSource::committed);
        auto       rejected = [&](const ItemPublishResult& result) {
            return ! result && ! result.error.isEmpty() && source.snapshot() == before &&
                   ! source.errorString().isEmpty() && commits.isEmpty();
        };
        QVERIFY(rejected(source.replace({ { "a", {}, 1 }, { "a", {}, 2 } })));
        QVERIFY(rejected(source.replace({}, { { ItemChange::Remove, 9, 1 } }, false)));
        QVERIFY(rejected(source.replace({}, { { ItemChange::Remove, 0, 2 } }, true)));
        QVERIFY(rejected(source.replace({ { "b", {}, 2 }, { "a", {}, 1 } }, {}, false)));
        QVERIFY(rejected(source.replace(
            { { "b", {}, 2 }, { "a", {}, 1 } }, { { ItemChange::Update, 0, 2 } }, false)));
        QVERIFY(rejected(source.replace({ { "a", {}, 9 }, { "b", {}, 2 } }, {}, false)));
        QVERIFY(rejected(source.replace(
            { { "a", {}, 1 }, { "b", {}, 2 } }, { { ItemChange::Move, 0, 1, 2 } }, false)));
        for (int mode = 0; mode < 4; ++mode) {
            auto data      = std::make_shared<CountingData>();
            data->badKey   = mode == 0;
            data->badIndex = mode == 1;
            if (mode == 2) data->contentRevision = 0;
            if (mode == 3) data->total = -1;
            QVERIFY(rejected(source.publish(data)));
            QCOMPARE(data->reads, 0);
        }
        QVERIFY(source.replace(
            { { "b", {}, 2 }, { "a", {}, 1 } }, { { ItemChange::Move, 0, 1, 1 } }, false));
        QVERIFY(source.errorString().isEmpty());
        QVERIFY(source.replace({}));
        QCOMPARE(source.count(), 0);
        QCOMPARE(before->valueAt(0).toInt(), 1);
    }
    void contentRevisionCannotMoveBackwards() {
        TestSource source;
        auto       first       = std::make_shared<CountingData>();
        first->contentRevision = 2;
        QVERIFY(source.publish(first));
        const auto before = source.snapshot();
        auto       older  = std::make_shared<CountingData>();
        QVERIFY(! source.publish(older));
        QCOMPARE(source.snapshot(), before);
        QCOMPARE(first->reads, 0);
        QCOMPARE(older->reads, 0);
    }
    void lazyTypedDataAndLifetime() {
        auto                        data     = std::make_shared<CountingData>();
        std::weak_ptr<CountingData> lifetime = data;
        ItemSnapshotPtr             saved;
        {
            TestSource source;
            QVERIFY(source.publish(data));
            QCOMPARE(data->reads, 0);
            saved = source.snapshot();
            ViewportController controller;
            QVERIFY(controller.apply(saved, 300, 40, 0, 0).accepted);
            controller.measure(0, 80, 0);
            controller.apply(saved, 300, 40, 0, 0);
            QCOMPARE(data->reads, 0);
            QCOMPARE(saved->indexOfKey("9007199254740993"), 0);
            QQmlEngine    engine;
            QQmlComponent component(&engine);
            component.setData("import QtQuick\nItem { required property QtObject row; "
                              "property string title: row.value.title }",
                              QUrl());
            QVERIFY2(component.isReady(), qPrintable(component.errorString()));
            DelegateRecycler recycler(&engine);
            auto             item = recycler.acquire(&component, saved, 0);
            QVERIFY(item);
            QCOMPARE(item->property("title").toString(), "0");
            QCOMPARE(data->reads, 1);
            QCOMPARE(recycler.row(item)->value().metaType(), QMetaType::fromType<TypedRow>());
        }
        data.reset();
        QVERIFY(! lifetime.expired());
        QCOMPARE(saved->valueAt(42).value<TypedRow>().title, "42");
        saved.reset();
        QVERIFY(lifetime.expired());
    }
    void contextAndSourceDestruction() {
        TestSource source;
        auto       context       = std::make_unique<QObject>();
        int        notifications = 0;
        connect(&source, &ItemSource::committed, context.get(), [&] {
            ++notifications;
        });
        context.reset();
        QVERIFY(source.replace({ { "a", {}, 1 } }));
        QCOMPARE(notifications, 0);
        auto                 disposable = new TestSource;
        QPointer<TestSource> guard(disposable);
        connect(disposable, &ItemSource::committed, disposable, [disposable] {
            delete disposable;
        });
        const auto result = disposable->replace({ { "a", {}, 1 } });
        QVERIFY(result);
        QVERIFY(! guard);
    }
};
int run_item_source(int argc, char** argv) {
    ItemSourceTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "item_source.moc"
