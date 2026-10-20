#include <QtTest>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickWindow>
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
        LazyRow    row;
        QSignalSpy data(&row, &LazyRow::changed);
        QSignalSpy active(&row, &LazyRow::activeChanged);
        QSignalSpy pooled(&row, &LazyRow::pooled);
        row.bind(source.snapshot(), 0);
        QCOMPARE(active.size(), 1);
        row.bind(source.snapshot(), 1);
        QCOMPARE(active.size(), 1);
        QCOMPARE(data.size(), 2);
        row.pool();
        QCOMPARE(active.size(), 2);
        QCOMPARE(data.size(), 2);
        QCOMPARE(pooled.size(), 1);
        row.bind(source.snapshot(), 0);
        QCOMPARE(active.size(), 3);
        QCOMPARE(data.size(), 3);
        connect(&row, &LazyRow::activeChanged, &row, [&] {
            if (! row.active()) row.bind(source.snapshot(), 1);
        });
        row.pool();
        QVERIFY(row.active());
        QCOMPARE(row.index(), 1);
        QCOMPARE(pooled.size(), 1);
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
        QCOMPARE(source.count(), 0);
        QVERIFY(! source.errorString().isEmpty());
        QCOMPARE(before->rows.size(), 2);
        QCOMPARE(before->indexOfKey("b"), 1);
        source.setKeyRole({});
        QVERIFY(! source.stableKeys());
        const auto transient = source.snapshot();
        source.setItems({ 1, 2, 3 });
        QCOMPARE(source.indexOfKey(transient->rows[0].key), -1);
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
        QCOMPARE(source.snapshot()->rows[1].value.toMap().value("id").toString(), "changed");
        model.change(1, "a");
        QVERIFY(! source.errorString().isEmpty());
        model.change(1, "valid");
        QVERIFY(source.errorString().isEmpty());
        QCOMPARE(source.count(), model.rowCount());
        model.reset({ "reset" });
        QCOMPARE(source.indexOfKey("reset"), 0);
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
        const auto a = source.snapshot()->rows[0].contentRevision;
        const auto b = source.snapshot()->rows[1].contentRevision;
        model.change(0, "a");
        QVERIFY(source.snapshot()->rows[0].contentRevision > a);
        QCOMPARE(source.snapshot()->rows[1].contentRevision, b);
        view.apply(source.snapshot(), 300, 40, 0, 0);
        QCOMPARE(view.layout().extent(0), 40.0);
        QCOMPARE(view.layout().extent(1), 80.0);
        model.move(1, 0);
        QCOMPARE(source.snapshot()->rows[0].contentRevision, b);
        view.apply(source.snapshot(), 300, 40, 0, 0);
        QCOMPARE(view.layout().extent(0), 80.0);
        model.insert(0, "new");
        QCOMPARE(source.snapshot()->rows[1].contentRevision, b);
        model.reset(model.values);
        QVERIFY(source.snapshot()->rows[1].contentRevision > b);
    }
    void snapshotVersionsPreserveUnchangedRows() {
        ListSnapshotSource source;
        source.setKeyRole("id");
        source.setItems({ QVariantMap { { "id", "a" } }, QVariantMap { { "id", "b" } } });
        const auto old = source.snapshot();
        source.setItems({ QVariantMap { { "id", "new" } },
                          QVariantMap { { "id", "a" } },
                          QVariantMap { { "id", "b" }, { "text", "changed" } } });
        QCOMPARE(source.snapshot()->rows[1].contentRevision, old->rows[0].contentRevision);
        QVERIFY(source.snapshot()->rows[2].contentRevision > old->rows[1].contentRevision);
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
        source.requestMore();
        source.requestMore();
        QCOMPARE(model.fetches, 0);
        QCoreApplication::processEvents();
        QCOMPARE(model.fetches, 1);
        QCoreApplication::processEvents();
        QCOMPARE(model.fetches, 1);
        source.requestMore();
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
        source.publish({ { "a", {}, 1 } }, true);
        source.publish({}, true, { { ItemChange::Remove, 9, 1 } }, false);
        QCOMPARE(source.count(), 0);
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
