#include <QtTest>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlExpression>
#include <QSortFilterProxyModel>
#include <QQuickWindow>
#include <utility>
#include "qml_material/model/item_model_source.hpp"
#include "delegate_recycler.hpp"
#include "viewport_controller.hpp"

using namespace qml_material;

class CreationHook : public QObject {
    Q_OBJECT
public:
    std::function<void()> callback;
    Q_INVOKABLE void      invoke() { callback(); }
};

class Rows : public QAbstractListModel {
public:
    QStringList            values { "a", "b", "c" };
    mutable int            reads   = 0;
    bool                   more    = false;
    int                    fetches = 0;
    QHash<int, QByteArray> roleNames() const override { return { { Qt::UserRole, "id" } }; }
    int                    rowCount(const QModelIndex& parent = {}) const override {
        return parent.isValid() ? 0 : values.size();
    }
    QVariant data(const QModelIndex& index, int role) const override {
        ++reads;
        return index.isValid() && role == Qt::UserRole ? QVariant(values[index.row()]) : QVariant();
    }
    bool setData(const QModelIndex& index, const QVariant& value, int role) override {
        if (! index.isValid() || role != Qt::UserRole) return false;
        change(index.row(), value.toString());
        return true;
    }
    void insert(int at, QString value) {
        beginInsertRows({}, at, at);
        values.insert(at, value);
        endInsertRows();
    }
    void remove(int at) {
        beginRemoveRows({}, at, at);
        values.removeAt(at);
        endRemoveRows();
    }
    void move(int from, int to) {
        if (! beginMoveRows({}, from, from, {}, to > from ? to + 1 : to)) return;
        values.move(from, to);
        endMoveRows();
    }
    void change(int at, QString value) {
        values[at] = value;
        Q_EMIT dataChanged(index(at), index(at), { Qt::UserRole });
    }
    void reset(QStringList items) {
        beginResetModel();
        values = items;
        endResetModel();
    }
    bool canFetchMore(const QModelIndex&) const override { return more; }
    void fetchMore(const QModelIndex&) override { ++fetches; }
};

class LazyDataTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void initTestCase() {
        qmlRegisterUncreatableType<LazyRow>("Test", 1, 0, "LazyRow", "Supplied by recycler");
    }
    void rowActivityNotifications() {
        ListSnapshotSource source;
        source.setItems({ 1, 2 });
        QQmlEngine    engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nItem { required property QtObject row }", QUrl());
        QVERIFY(component.isReady());
        DelegateRecycler recycler(&engine);
        auto             item = recycler.acquire(&component, source.snapshot(), 0);
        QVERIFY(item);
        auto       row = recycler.row(item);
        QSignalSpy data(row, &LazyRow::changed);
        QSignalSpy active(row, &LazyRow::activeChanged);
        QSignalSpy pooled(row, &LazyRow::pooled);
        QVERIFY(recycler.rebind(item, source.snapshot(), 1));
        QCOMPARE(active.size(), 0);
        QCOMPARE(data.size(), 1);
        recycler.release(item);
        QCOMPARE(active.size(), 1);
        QCOMPARE(pooled.size(), 1);
        QCOMPARE(recycler.acquire(&component, source.snapshot(), 0), item);
        QCOMPARE(active.size(), 2);
        QCOMPARE(data.size(), 2);
        recycler.release(item);
        QVERIFY(! row->active());
        QCOMPARE(pooled.size(), 2);
    }
    void pooledChildInputState() {
        QQmlEngine   engine;
        QQuickWindow window;
        window.setGeometry(0, 0, 200, 200);
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            Item {
                required property var row
                width: 100; height: 80
                property int cancellations: 0
                FocusScope {
                    anchors.fill: parent
                    TextInput { id: editor; objectName: "editor"; text: "edit" }
                    MouseArea {
                        objectName: "mouse"
                        anchors.fill: parent
                        onPressed: editor.forceActiveFocus()
                        onCanceled: parent.parent.cancellations++
                    }
                }
            }
        )",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        ListSnapshotSource source;
        source.setItems({ 1, 2 });
        DelegateRecycler recycler(&engine);
        auto*            item = recycler.acquire(&component, source.snapshot(), 0);
        QVERIFY(item);
        recycler.attach(item, window.contentItem());
        recycler.layout(item, { 0, 0, 100, 80 });
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* mouse  = item->findChild<QQuickItem*>("mouse");
        auto* editor = item->findChild<QQuickItem*>("editor");
        QVERIFY(mouse);
        QVERIFY(editor);
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, { 20, 20 });
        QVERIFY(mouse->property("pressed").toBool());
        QVERIFY(editor->hasActiveFocus());
        QCOMPARE(window.mouseGrabberItem(), mouse);
        recycler.release(item);
        QVERIFY(! mouse->property("pressed").toBool());
        QVERIFY(window.mouseGrabberItem() != mouse);
        QVERIFY(! editor->hasActiveFocus());
        QVERIFY(! editor->hasFocus());
        QCOMPARE(item->property("cancellations").toInt(), 1);
        auto* reused = recycler.acquire(&component, source.snapshot(), 1);
        QCOMPARE(reused, item);
        recycler.attach(reused, window.contentItem());
        QVERIFY(! editor->hasActiveFocus());
        QVERIFY(! mouse->property("pressed").toBool());
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, { 20, 20 });
        QCOMPARE(item->property("cancellations").toInt(), 1);
    }
    void snapshotsAndInvalidKeys() {
        ListSnapshotSource source;
        source.setKeyRole("id");
        source.setItems({ QVariantMap { { "id", "a" } }, QVariantMap { { "id", "b" } } });
        const auto before = source.snapshot();
        QCOMPARE(before->indexOfKey("b"), 1);
        source.setItems({ QVariantMap { { "id", "a" } }, QVariantMap { { "id", "a" } } });
        QCOMPARE(source.count(), 2);
        QVERIFY(! source.errorString().isEmpty());
        QCOMPARE(before->count(), 2);
        QCOMPARE(before->indexOfKey("b"), 1);
        source.setKeyRole({});
        QVERIFY(! source.stableKeys());
        const auto transient = source.snapshot();
        source.setItems({ 1, 2, 3 });
        QCOMPARE(source.indexOfKey(transient->descriptorAt(0).key), -1);
    }
    void incrementalModel() {
        Rows            model;
        ItemModelSource source;
        source.setKeyRole("id");
        source.setModel(&model);
        QCOMPARE(source.count(), 3);
        QCOMPARE(model.reads, 3);
        QSignalSpy changes(&source, &ItemSource::committed);
        const auto before = source.snapshot();
        model.insert(0, "x");
        QCOMPARE(model.reads, 4);
        QCOMPARE(source.indexOfKey("b"), 2);
        QCOMPARE(before->indexOfKey("b"), 1);
        auto change = qvariant_cast<ItemChangeSet>(changes.last().front());
        QVERIFY(! change.reset);
        QCOMPARE(change.changes[0].kind, ItemChange::Insert);
        QCOMPARE(change.before, before);
        QCOMPARE(change.after, source.snapshot());
        model.move(2, 0);
        QCOMPARE(model.reads, 4);
        QCOMPARE(source.indexOfKey("b"), 0);
        change = qvariant_cast<ItemChangeSet>(changes.last().front());
        QCOMPARE(change.changes[0].destination, 0);
        model.move(0, 3);
        QCOMPARE(source.indexOfKey("b"), 3);
        model.remove(0);
        QCOMPARE(model.reads, 4);
        model.change(1, "changed");
        QCOMPARE(model.reads, 5);
        QCOMPARE(source.snapshot()->valueAt(1).toMap().value("id").toString(), "changed");
        model.change(1, "a");
        QVERIFY(! source.errorString().isEmpty());
        model.change(1, "valid");
        QVERIFY(source.errorString().isEmpty());
        QCOMPARE(source.count(), model.rowCount());
        model.reset({ "reset" });
        QCOMPARE(source.indexOfKey("reset"), 0);
    }
    void persistentIdentityAndRoleWrites() {
        Rows                  model;
        QSortFilterProxyModel proxy;
        proxy.setSourceModel(&model);
        proxy.setSortRole(Qt::UserRole);
        ItemModelSource source;
        source.setKeyMode(ItemModelSource::PersistentIndexKey);
        source.setModel(&proxy);
        QCOMPARE(source.count(), 3);
        const auto key    = source.snapshot()->descriptorAt(1).key;
        auto       object = source.objectForKey(key);
        QVERIFY(object);
        QCOMPARE(model.reads, 3);
        model.insert(0, "x");
        QCOMPARE(source.indexOfKey(key), 2);
        model.move(2, 0);
        QCOMPARE(source.indexOfKey(key), 0);
        proxy.sort(0, Qt::DescendingOrder);
        QCOMPARE(source.valueAt(source.indexOfKey(key)).toMap().value("id").toString(), "b");
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("entry", object);
        QQmlExpression expression(engine.rootContext(), nullptr, "entry.id = 'edited'");
        expression.evaluate();
        QVERIFY2(! expression.hasError(), qPrintable(expression.error().toString()));
        QCOMPARE(model.values.front(), "edited");
        QCOMPARE(object->value("id").toString(), "edited");
        QCOMPARE(source.valueAt(source.indexOfKey(key)).toMap().value("id").toString(), "edited");
        model.change(0, "external");
        QCOMPARE(object->value("id").toString(), "external");
        model.remove(0);
        QVERIFY(! source.setRole(key, "id", "stale"));
        expression.evaluate();
        QVERIFY(! model.values.contains("edited"));
        const auto oldKey = source.snapshot()->descriptorAt(0).key;
        model.reset({ "replacement" });
        QCOMPARE(source.indexOfKey(oldKey), -1);
        QVERIFY(! source.setRole(oldKey, "id", "stale"));
        QCOMPARE(model.values.front(), "replacement");
    }
    void valueChangesPreserveMeasurements() {
        Rows model;
        model.values.clear();
        for (int i = 0; i < 100; ++i) model.values.append(QString::number(i));
        ItemModelSource source;
        source.setKeyMode(ItemModelSource::PersistentIndexKey);
        source.setDeferDataChanges(true);
        source.setModel(&model);
        ViewportController view;
        QVERIFY(view.apply(source.snapshot(), 300, 40, 2, 0).accepted);
        view.measure(0, 80, 0);
        view.measure(10, 100, 0);
        view.measure(20, 60, 0);
        const double offset = view.layout().offset(20) + 7;
        QSignalSpy   commits(&source, &ItemSource::committed);
        model.change(0, "zero");
        model.change(5, "uncached");
        model.change(20, "anchor");
        source.flushDataChanges();
        const auto change = qvariant_cast<ItemChangeSet>(commits.front().front());
        const auto result = view.applyValues(change.after, change.changes, 300, 40, 2, offset);
        QVERIFY(result.accepted);
        QCOMPARE(result.offset, offset - 40);
        QCOMPARE(view.layout().extent(0), 40.0);
        QCOMPARE(view.layout().extent(10), 100.0);
        QCOMPARE(view.layout().extent(20), 40.0);
        QCOMPARE(view.layout().offset(20) - result.offset, -7.0);
        const auto before = view.snapshot();
        QVERIFY(! view.applyValues(source.snapshot(),
                                   { { ItemChange::Update, 99, 2 } },
                                   300,
                                   40,
                                   2,
                                   result.offset)
                      .accepted);
        QCOMPARE(view.snapshot(), before);
        QCOMPARE(view.layout().extent(10), 100.0);
        const auto resized = view.applyValues(source.snapshot(), {}, 200, 40, 2, result.offset);
        QVERIFY(resized.accepted);
        QCOMPARE(view.layout().extent(10), 40.0);
    }
    void deferredDataChanges() {
        Rows model;
        model.values.clear();
        for (int i = 0; i < 5000; ++i) model.values.append(QString::number(i));
        ItemModelSource source;
        source.setKeyMode(ItemModelSource::PersistentIndexKey);
        source.setDeferDataChanges(true);
        source.setModel(&model);
        const auto before = source.snapshot();
        const int  reads  = model.reads;
        QSignalSpy commits(&source, &ItemSource::committed);
        for (int i = 99; i >= 0; --i) model.change(i * 37, QStringLiteral("changed%1").arg(i));
        model.change(0, "final");
        QCOMPARE(commits.size(), 0);
        QCOMPARE(model.reads, reads);
        QCOMPARE(before->valueAt(0).toMap().value("id").toString(), "0");
        QCoreApplication::processEvents();
        QCOMPARE(commits.size(), 1);
        QCOMPARE(model.reads, reads + 100);
        QCOMPARE(source.valueAt(0).toMap().value("id").toString(), "final");
        QCOMPARE(before->valueAt(0).toMap().value("id").toString(), "0");
        const auto change = qvariant_cast<ItemChangeSet>(commits.front().front());
        QCOMPARE(change.changes.size(), 100);
        QVERIFY(! change.reset);
        QCoreApplication::processEvents();
        QCOMPARE(commits.size(), 1);
        model.change(1, "one");
        model.change(2, "two");
        model.change(3, "three");
        source.flushDataChanges();
        const auto contiguous = qvariant_cast<ItemChangeSet>(commits.back().front());
        QCOMPARE(contiguous.changes.size(), 1);
        QCOMPARE(contiguous.changes.front().index, 1);
        QCOMPARE(contiguous.changes.front().count, 3);
        QCOMPARE(model.reads, reads + 103);
    }
    void deferredDataBeforeStructure() {
        Rows            model;
        ItemModelSource source;
        source.setKeyMode(ItemModelSource::PersistentIndexKey);
        source.setDeferDataChanges(true);
        source.setModel(&model);
        const auto key = source.snapshot()->descriptorAt(1).key;
        QSignalSpy commits(&source, &ItemSource::committed);
        model.change(1, "inserted");
        model.insert(0, "prefix");
        QCOMPARE(commits.size(), 2);
        QCOMPARE(source.indexOfKey(key), 2);
        QCOMPARE(source.valueAt(2).toMap().value("id").toString(), "inserted");
        model.change(2, "moved");
        model.move(2, 0);
        QCOMPARE(source.indexOfKey(key), 0);
        QCOMPARE(source.valueAt(0).toMap().value("id").toString(), "moved");
        model.change(0, "removed");
        model.remove(0);
        QCOMPARE(source.indexOfKey(key), -1);
        model.change(0, "reset");
        model.reset({ "replacement" });
        QCOMPARE(source.count(), 1);
        QCOMPARE(source.valueAt(0).toMap().value("id").toString(), "replacement");
        const int changes = commits.size();
        QCoreApplication::processEvents();
        QCOMPARE(commits.size(), changes);
        QVERIFY(source.errorString().isEmpty());
        model.change(0, "old model");
        Rows replacement;
        source.setModel(&replacement);
        QCoreApplication::processEvents();
        QCOMPARE(source.count(), 3);
        QCOMPARE(source.valueAt(0).toMap().value("id").toString(), "a");
    }
    void deferredRoleEditing() {
        Rows            model;
        ItemModelSource source;
        source.setKeyMode(ItemModelSource::PersistentIndexKey);
        source.setDeferDataChanges(true);
        source.setModel(&model);
        const auto key    = source.snapshot()->descriptorAt(1).key;
        auto       object = source.objectForKey(key);
        QVERIFY(object);
        model.change(0, "pending");
        QQmlEngine engine;
        engine.rootContext()->setContextProperty("entry", object);
        QQmlExpression expression(engine.rootContext(), nullptr, "entry.id = 'edited'");
        expression.evaluate();
        QVERIFY2(! expression.hasError(), qPrintable(expression.error().toString()));
        QCOMPARE(source.valueAt(0).toMap().value("id").toString(), "pending");
        QCOMPARE(source.valueAt(1).toMap().value("id").toString(), "edited");
        QCOMPARE(object->value("id").toString(), "edited");
        model.change(2, "synchronous");
        source.setDeferDataChanges(false);
        QCOMPARE(source.valueAt(2).toMap().value("id").toString(), "synchronous");
        model.change(0, "immediate");
        QCOMPARE(source.valueAt(0).toMap().value("id").toString(), "immediate");
    }
    void deferredReadReentryAndLifetime() {
        class ReentrantRows : public Rows {
        public:
            mutable std::function<void()> onRead;
            QVariant                      data(const QModelIndex& index, int role) const override {
                if (auto action = std::exchange(onRead, {})) action();
                return Rows::data(index, role);
            }
        } model;
        ItemModelSource source;
        source.setKeyMode(ItemModelSource::PersistentIndexKey);
        source.setDeferDataChanges(true);
        source.setModel(&model);
        QSignalSpy commits(&source, &ItemSource::committed);
        model.change(0, "first");
        model.onRead = [&] {
            model.change(1, "reentrant");
        };
        QTRY_COMPARE(source.valueAt(1).toMap().value("id").toString(), "reentrant");
        QCOMPARE(commits.size(), 1);
        QVERIFY(source.errorString().isEmpty());
        auto disposable = std::make_unique<Rows>();
        source.setModel(disposable.get());
        disposable->change(0, "pending");
        disposable.reset();
        QCoreApplication::processEvents();
        QCOMPARE(source.count(), 0);
        auto adapter = std::make_unique<ItemModelSource>();
        adapter->setDeferDataChanges(true);
        adapter->setKeyMode(ItemModelSource::PersistentIndexKey);
        adapter->setModel(&model);
        model.change(0, "deleted adapter");
        adapter.reset();
        QCoreApplication::processEvents();
    }
    void contentVersionsInvalidateMeasurements() {
        Rows            model;
        ItemModelSource source;
        source.setKeyRole("id");
        source.setModel(&model);
        ViewportController view;
        view.apply(source.snapshot(), 300, 40, 0, 0);
        view.measure(0, 100, 0);
        view.measure(1, 80, 0);
        const auto a = source.snapshot()->descriptorAt(0).contentRevision;
        const auto b = source.snapshot()->descriptorAt(1).contentRevision;
        model.change(0, "a");
        QVERIFY(source.snapshot()->descriptorAt(0).contentRevision > a);
        QCOMPARE(source.snapshot()->descriptorAt(1).contentRevision, b);
        view.apply(source.snapshot(), 300, 40, 0, 0);
        QCOMPARE(view.layout().extent(0), 40.0);
        QCOMPARE(view.layout().extent(1), 80.0);
        model.move(1, 0);
        QCOMPARE(source.snapshot()->descriptorAt(0).contentRevision, b);
        view.apply(source.snapshot(), 300, 40, 0, 0);
        QCOMPARE(view.layout().extent(0), 80.0);
        model.insert(0, "new");
        QCOMPARE(source.snapshot()->descriptorAt(1).contentRevision, b);
        model.reset(model.values);
        QVERIFY(source.snapshot()->descriptorAt(1).contentRevision > b);
    }
    void snapshotVersionsPreserveUnchangedRows() {
        ListSnapshotSource source;
        source.setKeyRole("id");
        source.setItems({ QVariantMap { { "id", "a" } }, QVariantMap { { "id", "b" } } });
        const auto old = source.snapshot();
        source.setItems({ QVariantMap { { "id", "new" } },
                          QVariantMap { { "id", "a" } },
                          QVariantMap { { "id", "b" }, { "text", "changed" } } });
        QCOMPARE(source.snapshot()->descriptorAt(1).contentRevision,
                 old->descriptorAt(0).contentRevision);
        QVERIFY(source.snapshot()->descriptorAt(2).contentRevision >
                old->descriptorAt(1).contentRevision);
    }
    void sourceLifetimeAndReentry() {
        ItemModelSource source;
        source.setKeyRole("id");
        auto model = std::make_unique<Rows>();
        source.setModel(model.get());
        connect(&source, &ItemSource::committed, &source, [&](const ItemChangeSet& change) {
            if (change.after->indexOfKey("x") >= 0 && change.after->indexOfKey("y") < 0)
                model->insert(0, "y");
        });
        QVector<quint64> revisions;
        connect(&source, &ItemSource::committed, &source, [&](const ItemChangeSet& change) {
            revisions.append(change.after->revision);
        });
        model->insert(0, "x");
        QCOMPARE(revisions.size(), 2);
        QCOMPARE(revisions[1], revisions[0] + 1);
        QCOMPARE(source.indexOfKey("y"), 0);
        model.reset();
        QCOMPARE(source.count(), 0);
        QVERIFY(! source.model());
        auto                         disposable = new ListSnapshotSource;
        QPointer<ListSnapshotSource> guard(disposable);
        connect(disposable, &ItemSource::committed, disposable, [disposable] {
            delete disposable;
        });
        disposable->setItems({ 1 });
        QVERIFY(! guard);
    }
    void fetchingIsQueuedAndCoalesced() {
        Rows model;
        model.more = true;
        ItemModelSource source;
        source.setKeyRole("id");
        source.setModel(&model);
        source.requestMore(ItemDemand(source.snapshot(), 0, source.count(), true));
        source.requestMore(ItemDemand(source.snapshot(), 0, source.count(), true));
        QCOMPARE(model.fetches, 0);
        QCoreApplication::processEvents();
        QCOMPARE(model.fetches, 1);
        QCoreApplication::processEvents();
        QCOMPARE(model.fetches, 1);
        source.requestMore(ItemDemand(source.snapshot(), 0, source.count(), true));
        source.setModel(nullptr);
        QCoreApplication::processEvents();
        QCOMPARE(model.fetches, 1);
    }
    void recyclerBindingAndIsolation() {
        QQmlEngine    engine;
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Test
            Item {
                required property LazyRow row
                property string observed: row.key + ':' + row.value.id + ':' + row.index
                property string initial: ''
                property bool allowed: false
                enabled: allowed
                Component.onCompleted: initial = observed
            }
        )",
                          QUrl("qrc:/row.qml"));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        ListSnapshotSource source;
        source.setKeyRole("id");
        source.setTypeRole("type");
        source.setItems({ QVariantMap { { "id", "a" }, { "type", "one" } },
                          QVariantMap { { "id", "b" }, { "type", "one" } },
                          QVariantMap { { "id", "c" }, { "type", "two" } } });
        DelegateRecycler recycler(&engine);
        auto             item = recycler.acquire(&component, source.snapshot(), 0);
        QVERIFY2(item, qPrintable(recycler.errorString()));
        QCOMPARE(item->property("initial").toString(), "a:a:0");
        auto row = recycler.row(item);
        connect(row, &LazyRow::changed, row, [row] {
            QCOMPARE(row->key(), row->value().toMap().value("id").toString());
        });
        recycler.release(item);
        QVERIFY(! row->active());
        QCOMPARE(recycler.poolSize(), 1);
        QCOMPARE(recycler.acquire(&component, source.snapshot(), 1), item);
        QCOMPARE(item->property("observed").toString(), "b:b:1");
        QVERIFY(! item->isEnabled());
        item->setProperty("allowed", true);
        QVERIFY(item->isEnabled());
        recycler.release(item);
        auto other = recycler.acquire(&component, source.snapshot(), 2);
        QVERIFY(other && other != item);
        recycler.setPoolLimit(0);
        QCOMPARE(recycler.poolSize(), 0);
        QPointer<QQuickItem> tracked(other);
        recycler.clear();
        QVERIFY(! tracked);
    }
    void recyclerReentrantClear() {
        QQmlEngine    engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nItem { required property QtObject row }", QUrl());
        ListSnapshotSource source;
        source.setItems({ 1, 2 });
        DelegateRecycler recycler(&engine);
        auto             item = recycler.acquire(&component, source.snapshot(), 0);
        QVERIFY(item);
        auto row = recycler.row(item);
        recycler.release(item);
        connect(row, &LazyRow::reused, row, [&] {
            recycler.clear();
        });
        QVERIFY(! recycler.acquire(&component, source.snapshot(), 1));
        QCOMPARE(recycler.poolSize(), 0);
    }
    void rowUpdateAndComponentIdentity() {
        QQmlEngine       engine;
        QQmlComponent    first(&engine), second(&engine);
        const QByteArray code = "import QtQuick\nItem { required property QtObject row; property "
                                "var observedValue: row.value }";
        first.setData(code, QUrl());
        second.setData(code, QUrl());
        ListSnapshotSource source;
        source.setKeyRole("id");
        source.setItems({ QVariantMap { { "id", "a" }, { "text", "before" } } });
        DelegateRecycler recycler(&engine);
        auto             a = recycler.acquire(&first, source.snapshot(), 0);
        QVERIFY(a);
        source.setItems({ QVariantMap { { "id", "a" }, { "text", "after" } } });
        QVERIFY(recycler.rebind(a, source.snapshot(), 0));
        QCOMPARE(a->property("observedValue").toMap().value("text").toString(), "after");
        recycler.release(a);
        auto b = recycler.acquire(&second, source.snapshot(), 0);
        QVERIFY(b && b != a);
        delete b;
        auto c = recycler.acquire(&second, source.snapshot(), 0);
        QVERIFY(c);
        QPointer<QQuickItem> old(a);
        recycler.setPoolLimit(0);
        QVERIFY(! old);
    }
    void clearDuringPoolAndBind() {
        QQmlEngine    engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nItem { required property QtObject row }", QUrl());
        ListSnapshotSource source;
        source.setItems({ 1 });
        DelegateRecycler recycler(&engine);
        auto             item = recycler.acquire(&component, source.snapshot(), 0);
        QVERIFY(item);
        QPointer<QQuickItem> tracked(item);
        connect(recycler.row(item), &LazyRow::pooled, &recycler, [&] {
            recycler.clear();
        });
        recycler.release(item);
        QVERIFY(! tracked);
        item = recycler.acquire(&component, source.snapshot(), 0);
        QVERIFY(item);
        tracked = item;
        connect(recycler.row(item), &LazyRow::changed, &recycler, [&] {
            recycler.clear();
        });
        QVERIFY(! recycler.rebind(item, source.snapshot(), 0));
        QVERIFY(! tracked);
    }
    void failedCreation() {
        QQmlEngine    engine;
        QQmlComponent component(&engine);
        component.setData("import QtQml\nQtObject { required property QtObject row }", QUrl());
        ListSnapshotSource source;
        source.setItems({ 1 });
        DelegateRecycler recycler(&engine);
        QVERIFY(! recycler.acquire(&component, source.snapshot(), 0));
        QVERIFY(! recycler.errorString().isEmpty());
        QCOMPARE(recycler.poolSize(), 0);
    }
    void invalidChangeIsRejected() {
        class Source : public ItemSource {
        public:
            using ItemSource::publish;
        } source;
        source.publish(makeItemDataSnapshot({ { "a", {}, 1 } }));
        source.publish(makeItemDataSnapshot({}), { { ItemChange::Remove, 9, 1 } }, false);
        QCOMPARE(source.count(), 1);
        QVERIFY(! source.errorString().isEmpty());
    }
    void cancellationDuringCreation() {
        QQmlEngine   engine;
        CreationHook hook;
        engine.rootContext()->setContextProperty("creationHook", &hook);
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            Item {
                required property QtObject row
                Component.onCompleted: creationHook.invoke()
            }
        )",
                          QUrl());
        ListSnapshotSource source;
        source.setItems({ 1 });
        auto recycler = std::make_unique<DelegateRecycler>(&engine);
        hook.callback = [&] {
            recycler->clear();
        };
        QVERIFY(! recycler->acquire(&component, source.snapshot(), 0));
        hook.callback = [&] {
            recycler.reset();
        };
        QVERIFY(! recycler->acquire(&component, source.snapshot(), 0));
        QVERIFY(! recycler);
    }
};
int run_lazy_data(int argc, char** argv) {
    QCoreApplication::setAttribute(Qt::AA_Use96Dpi, true);
    LazyDataTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}
#include "lazy_data.moc"
