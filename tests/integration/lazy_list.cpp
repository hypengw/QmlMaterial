#include "qml_material/view/lazy_list.hpp"
#include "qml_material/model/lazy_row.hpp"
#include "qml_material/model/item_model_source.hpp"
#include <QQmlContext>
#include <QQuickRenderControl>
#include <QQuickWindow>
#include <QtTest>
#include <QTimer>
#include <functional>
#include <utility>
#include <cmath>
#include <limits>

using namespace qml_material;

class InputLazyList : public LazyList {
public:
    using LazyList::LazyList;
    using Flickable::mousePressEvent;
    using Flickable::mouseMoveEvent;
    using Flickable::mouseReleaseEvent;
};

class DelegateCallback : public QObject {
    Q_OBJECT
public:
    std::function<void()> callback;
    Q_INVOKABLE void fire() {
        auto action = std::exchange(callback, {});
        if (action) action();
    }
};

class MutableRows : public QAbstractListModel {
public:
    QStringList keys;
    int rowCount(const QModelIndex& parent = {}) const override {
        return parent.isValid() ? 0 : keys.size();
    }
    QHash<int, QByteArray> roleNames() const override { return {{Qt::UserRole, "id"}}; }
    QVariant data(const QModelIndex& index, int role) const override {
        if (! index.isValid() || index.row() >= keys.size() || role != Qt::UserRole) return {};
        return keys[index.row()];
    }
    void prepend(int count) {
        beginInsertRows({}, 0, count - 1);
        for (int i = count - 1; i >= 0; --i) keys.prepend(QStringLiteral("new%1").arg(i));
        endInsertRows();
    }
    bool move(int from, int to) {
        if (! beginMoveRows({}, from, from, {}, to > from ? to + 1 : to)) return false;
        keys.move(from, to);
        endMoveRows();
        return true;
    }
    void remove(int row) {
        beginRemoveRows({}, row, row);
        keys.removeAt(row);
        endRemoveRows();
    }
    void reset(int count) {
        beginResetModel();
        keys.clear();
        for (int i = 0; i < count; ++i) keys.append(QString::number(i));
        endResetModel();
    }
};

class PagedRows : public QAbstractListModel {
public:
    int  count    = 0;
    int  requests = 0;
    bool progress = true;
    int  rowCount(const QModelIndex& parent = {}) const override {
        return parent.isValid() ? 0 : count;
    }
    QHash<int, QByteArray> roleNames() const override {
        return { { Qt::UserRole, "id" }, { Qt::UserRole + 1, "height" } };
    }
    QVariant data(const QModelIndex& index, int role) const override {
        return role == Qt::UserRole ? QVariant(QString::number(index.row())) : QVariant(40);
    }
    bool canFetchMore(const QModelIndex& parent) const override {
        return ! parent.isValid() && count < 20;
    }
    void fetchMore(const QModelIndex&) override {
        ++requests;
        if (! progress) {
            if (count) emit dataChanged(index(0), index(count - 1));
            return;
        }
        beginInsertRows({}, count, count + 1);
        count += 2;
        endInsertRows();
    }
};

class AsyncRows : public PagedRows {
    Q_OBJECT
public:
    bool loading = false;
    bool canFetchMore(const QModelIndex& parent) const override {
        return ! loading && PagedRows::canFetchMore(parent);
    }
    void fetchMore(const QModelIndex&) override {
        ++requests;
        loading = true;
    }
    void complete(bool append = true) {
        QTimer::singleShot(0, this, [this, append] {
            if (append) {
                beginInsertRows({}, count, count + 1);
                count += 2;
                endInsertRows();
            }
            // Keep loading true through row delivery, then announce readiness later.
            QTimer::singleShot(0, this, [this] {
                loading = false;
                emit readyChanged();
            });
        });
    }
    Q_SIGNAL void readyChanged();
};

class LazyListTest : public QObject {
    Q_OBJECT
    static QVariantList rows(int count) {
        QVariantList result;
        for (int i = 0; i < count; ++i)
            result.append(
                QVariantMap { { "id", QString::number(i) }, { "height", 40 + i % 3 * 10 } });
        return result;
    }
    static void frames(QQuickRenderControl& render, int count = 8) {
        for (int i = 0; i < count; ++i) {
            QCoreApplication::processEvents();
            render.polishItems();
        }
    }
private slots:
    void modelStructuralAnchors() {
        QQmlEngine engine;
        QQuickRenderControl render;
        QQuickWindow window(&render);
        QQmlComponent delegate(&engine);
        delegate.setData("import QtQuick\nimport Test 1.0\nItem { required property LazyRow row; implicitHeight: 48 }", QUrl());
        QVERIFY(delegate.isReady());
        MutableRows model;
        model.reset(1000);
        ItemModelSource source;
        source.setKeyRole("id");
        source.setModel(&model);
        LazyList view(window.contentItem());
        QQmlEngine::setContextForObject(&view, engine.rootContext());
        view.setSize({300, 200});
        view.setSource(&source);
        view.setDelegate(&delegate);
        frames(render);
        QVERIFY(view.positionAtKey("500", 0, 7));
        frames(render);
        const auto screenPosition = [&](const QString& key) {
            for (auto* host : view.contentItem()->childItems()) {
                for (auto* item : host->childItems()) {
                    auto* row = item->property("row").value<LazyRow*>();
                    if (row && row->active() && row->key() == key)
                        return item->mapToItem(&view, QPointF()).y();
                }
            }
            return std::numeric_limits<qreal>::quiet_NaN();
        };
        QCOMPARE(screenPosition("500"), -7.0);
        model.prepend(100);
        frames(render);
        QCOMPARE(view.count(), 1100);
        QCOMPARE(view.firstVisibleIndex(), 600);
        QCOMPARE(screenPosition("500"), -7.0);
        QVERIFY(model.move(600, 20));
        frames(render);
        QCOMPARE(view.firstVisibleIndex(), 20);
        QCOMPARE(source.indexOfKey("500"), 20);
        QCOMPARE(screenPosition("500"), -7.0);
        const auto successor = model.keys[21];
        const qreal successorPosition = screenPosition(successor);
        QCOMPARE(successorPosition, 41.0);
        model.remove(20);
        frames(render);
        QCOMPARE(source.indexOfKey("500"), -1);
        QCOMPARE(screenPosition(successor), successorPosition);
        model.reset(0);
        frames(render);
        QCOMPARE(view.contentY(), 0.0);
        QCOMPARE(view.liveCount(), 0);
        QCOMPARE(view.firstVisibleIndex(), -1);
        model.reset(20);
        frames(render);
        QCOMPARE(view.firstVisibleIndex(), 0);
        QCOMPARE(screenPosition("0"), 0.0);
        QVERIFY(view.liveCount() < 30);
        QVERIFY2(view.errorString().isEmpty(), qPrintable(view.errorString()));
    }
    void updatesDuringMotion_data() {
        QTest::addColumn<bool>("prepend");
        QTest::newRow("drag-prepend") << true;
        QTest::newRow("drag-delayed-height") << false;
    }
    void updatesDuringMotion() {
        QFETCH(bool, prepend);
        QQmlEngine engine;
        QQuickRenderControl render;
        QQuickWindow window(&render);
        QQmlComponent delegate(&engine);
        delegate.setData("import QtQuick\nimport Test 1.0\nItem { required property LazyRow row; "
                         "property real extraHeight: 0; implicitHeight: 48 + extraHeight }", QUrl());
        QVERIFY2(delegate.isReady(), qPrintable(delegate.errorString()));
        ListSnapshotSource source;
        source.setKeyRole("id");
        source.setItems(rows(1000));
        InputLazyList view(window.contentItem());
        QQmlEngine::setContextForObject(&view, engine.rootContext());
        view.setSize({300, 200});
        view.setCacheExtent(150);
        view.setSynchronousDrag(true);
        view.setSource(&source);
        view.setDelegate(&delegate);
        frames(render);
        QVERIFY(view.positionAtKey("500"));
        frames(render);
        const auto mouse = [&](QEvent::Type type, int y, ulong time) {
            const bool moving = type == QEvent::MouseMove;
            QMouseEvent event(type, QPointF(100, y), QPointF(100, y),
                              moving ? Qt::NoButton : Qt::LeftButton,
                              type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton,
                              Qt::NoModifier);
            event.setTimestamp(time);
            if (type == QEvent::MouseButtonPress) view.mousePressEvent(&event);
            else if (moving) view.mouseMoveEvent(&event);
            else view.mouseReleaseEvent(&event);
        };
        mouse(QEvent::MouseButtonPress, 180, 1000);
        mouse(QEvent::MouseMove, 130, 1050);
        frames(render);
        QVERIFY(view.isDragging());
        const auto itemFor = [&](const QString& key) -> QQuickItem* {
            for (auto* host : view.contentItem()->childItems()) {
                for (auto* item : host->childItems()) {
                    const auto* row = item->property("row").value<LazyRow*>();
                    if (row && row->key() == key && row->active()) return item;
                }
            }
            return nullptr;
        };
        const QString anchorKey = "501";
        auto* anchor = itemFor(anchorKey);
        QVERIFY(anchor);
        const qreal screenY = anchor->mapToItem(&view, QPointF()).y();
        const qreal offset = view.contentY();
        if (prepend) {
            auto data = source.items();
            for (int i = 0; i < 100; ++i)
                data.prepend(QVariantMap{{"id", QStringLiteral("new%1").arg(i)}});
            source.setItems(data);
        } else {
            auto* previous = itemFor("500");
            QVERIFY(previous);
            previous->setProperty("extraHeight", 120);
        }
        frames(render);
        QVERIFY(view.isDragging());
        QVERIFY(itemFor("501"));
        QCOMPARE(itemFor("501")->mapToItem(&view, QPointF()).y(), screenY);
        QCOMPARE(view.contentY(), offset + (prepend ? 4800 : 120));
        const qreal corrected = view.contentY();
        mouse(QEvent::MouseMove, 113, 1070);
        frames(render);
        QCOMPARE(view.contentY(), corrected + 17);
        QCOMPARE(itemFor("501")->mapToItem(&view, QPointF()).y(), screenY - 17);
        mouse(QEvent::MouseButtonRelease, 113, 1500);
        QVERIFY(! view.isDragging());
        QVERIFY(! view.isMoving());
        QVERIFY(view.errorString().isEmpty());
    }
    void windowDestruction() {
        QQmlEngine engine;
        QQuickRenderControl render;
        auto window = std::make_unique<QQuickWindow>(&render);
        QQmlComponent delegate(&engine);
        delegate.setData("import QtQuick\nimport Test 1.0\nItem { required property LazyRow row; "
                         "implicitHeight: 40 }", QUrl());
        ListSnapshotSource source;
        source.setKeyRole("id");
        source.setItems(rows(1000));
        QPointer<LazyList> view = new LazyList(window->contentItem());
        QQmlEngine::setContextForObject(view, engine.rootContext());
        view->setSize({300, 200});
        view->setSource(&source);
        view->setDelegate(&delegate);
        frames(render);
        QList<QPointer<QObject>> objects;
        const auto track = [&] {
            for (auto* host : view->contentItem()->childItems()) {
                objects.append(host);
                for (auto* item : host->childItems()) {
                    objects.append(item);
                    objects.append(item->property("row").value<LazyRow*>());
                }
            }
        };
        track();
        view->setContentY(2000);
        frames(render);
        track();
        QVERIFY(! objects.isEmpty());
        QVERIFY(view->positionAtKey("800", 0, 0, LazyList::Smooth));
        frames(render, 2);
        window.reset();
        QVERIFY(view.isNull());
        for (const auto& object : objects) QVERIFY(object.isNull());
        source.setItems(rows(10));
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::processEvents();
    }
    void delegateReentry_data() {
        QTest::addColumn<int>("event");
        QTest::addColumn<int>("mutation");
        const QStringList events {"create", "bind", "pool", "reuse"};
        for (int event = 0; event < events.size(); ++event)
            for (int mutation = 0; mutation < 3; ++mutation)
                QTest::newRow(qPrintable(QString("%1-%2").arg(events[event])
                                        .arg(mutation))) << event << mutation;
    }
    void delegateReentry() {
        QFETCH(int, event);
        QFETCH(int, mutation);
        QQmlEngine engine;
        QQuickRenderControl render;
        QQuickWindow window(&render);
        DelegateCallback hook;
        engine.rootContext()->setContextProperty("callback", &hook);
        engine.rootContext()->setContextProperty("callbackEvent", event);
        QQmlComponent delegate(&engine), replacementDelegate(&engine);
        delegate.setData(R"(
            import QtQuick
            import Test 1.0
            Item {
                required property LazyRow row
                implicitHeight: 40
                Component.onCompleted: { if (callbackEvent === 0) callback.fire() }
                Connections {
                    target: row
                    function onChanged() { if (callbackEvent === 1) callback.fire() }
                    function onPooled() { if (callbackEvent === 2) callback.fire() }
                    function onReused() { if (callbackEvent === 3) callback.fire() }
                }
            }
        )", QUrl());
        replacementDelegate.setData("import QtQuick\nimport Test 1.0\nItem { "
                                    "required property LazyRow row; implicitHeight: 80 }", QUrl());
        QVERIFY2(delegate.isReady(), qPrintable(delegate.errorString()));
        ListSnapshotSource source, replacement;
        source.setKeyRole("id");
        source.setItems(rows(100));
        replacement.setKeyRole("id");
        replacement.setItems({QVariantMap{{"id", "replacement"}, {"height", 80}}});
        QPointer<LazyList> view = new LazyList(window.contentItem());
        QQmlEngine::setContextForObject(view, engine.rootContext());
        view->setSize({300, 200});
        view->setSource(&source);
        view->setDelegate(&delegate);
        if (event > 0) frames(render);
        int calls = 0;
        hook.callback = [&] {
            ++calls;
            if (mutation == 0) view->setSource(&replacement);
            else if (mutation == 1) view->setDelegate(&replacementDelegate);
            else delete view.data();
        };
        if (event == 1) {
            auto changed = source.items();
            changed[0] = QVariantMap{{"id", "0"}, {"height", 90}};
            source.setItems(changed);
        }
        if (event >= 2) view->setContentY(2000);
        frames(render, 20);
        QCOMPARE(calls, 1);
        if (mutation == 2) {
            QVERIFY(view.isNull());
        } else {
            QVERIFY(view);
            QVERIFY2(view->errorString().isEmpty(), qPrintable(view->errorString()));
            QVERIFY(view->liveCount() > 0);
            if (mutation == 0) {
                QCOMPARE(view->source(), &replacement);
                QCOMPARE(view->count(), 1);
                QCOMPARE(view->liveCount(), 1);
                for (auto* host : view->contentItem()->childItems())
                    for (auto* item : host->childItems()) {
                        auto* row = item->property("row").value<LazyRow*>();
                        QVERIFY(row);
                        QCOMPARE(row->key(), QString("replacement"));
                    }
            } else {
                QCOMPARE(view->delegate(), &replacementDelegate);
                for (auto* host : view->contentItem()->childItems())
                    for (auto* item : host->childItems()) QCOMPARE(item->height(), 80.0);
            }
            delete view.data();
        }
    }
    void initTestCase() {
        qmlRegisterUncreatableType<ItemSource>("Test", 1, 0, "ItemSource", "source base");
        qmlRegisterUncreatableType<LazyRow>("Test", 1, 0, "LazyRow", "delegate row");
        qmlRegisterType<LazyList>("Test", 1, 0, "LazyList");
        qmlRegisterType<ListSnapshotSource>("Test", 1, 0, "ListSnapshotSource");
    }
    void qmlConstruction() {
        QQmlEngine          engine;
        QQuickRenderControl render;
        QQuickWindow        window(&render);
        QQmlComponent       component(&engine);
        component.setData(R"(
            import QtQuick
            import Test 1.0
            LazyList {
                width: 300; height: 200
                source: ListSnapshotSource {
                    keyRole: "id"
                    items: [{id: "a", label: "A"}, {id: "b", label: "B"}]
                }
                delegate: Item { required property LazyRow row; implicitHeight: 50 }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* view = qobject_cast<LazyList*>(object.get());
        QVERIFY(view);
        QSignalSpy positioned(view, &LazyList::positioningFinished);
        QVERIFY(view->positionAtKey("b"));
        view->setParentItem(window.contentItem());
        QVERIFY(view->positioning());
        frames(render);
        QCOMPARE(positioned.size(), 1);
        QVERIFY(positioned.first().at(1).toBool());
        QCOMPARE(view->liveCount(), 2);
        QCOMPARE(view->contentHeight(), 100.0);
        QVERIFY(view->errorString().isEmpty());
        view->setHeight(50);
        frames(render);
        QCOMPARE(view->firstVisibleIndex(), 0);
        QCOMPARE(view->lastVisibleIndex(), 0);
        view->setContentY(50);
        frames(render);
        QCOMPARE(view->firstVisibleIndex(), 1);
        QCOMPARE(view->lastVisibleIndex(), 1);
        view->setHeight(0);
        frames(render);
        QCOMPARE(view->firstVisibleIndex(), -1);
        QCOMPARE(view->lastVisibleIndex(), -1);
    }
    void virtualizedAndAnchored() {
        QQmlEngine          engine;
        QQuickRenderControl render;
        QQuickWindow        window(&render);
        QQmlComponent       delegate(&engine);
        delegate.setData(R"(
            import QtQuick
            import Test 1.0
            Item { required property LazyRow row; implicitHeight: row.value.height }
        )",
                         QUrl());
        QVERIFY2(delegate.isReady(), qPrintable(delegate.errorString()));
        ListSnapshotSource source;
        source.setKeyRole("id");
        source.setItems(rows(100000));
        LazyList view(window.contentItem());
        QQmlEngine::setContextForObject(&view, engine.rootContext());
        view.setSize({ 300, 200 });
        view.setCacheExtent(80);
        view.setSource(&source);
        view.setDelegate(&delegate);
        frames(render);
        QCOMPARE(view.count(), 100000);
        QCOMPARE(view.firstVisibleIndex(), 0);
        QVERIFY(view.liveCount() > 0 && view.liveCount() < 30);
        QVERIFY2(view.errorString().isEmpty(), qPrintable(view.errorString()));
        QVERIFY(view.positionAtKey("50000"));
        frames(render);
        QCOMPARE(view.firstVisibleIndex(), 50000);
        QVERIFY(view.liveCount() < 30);
        const int   first  = view.firstVisibleIndex();
        const qreal offset = view.contentY();
        const auto anchorPosition = [&] {
            for (auto* host : view.contentItem()->childItems()) {
                for (auto* item : host->childItems()) {
                    const auto* row = item->property("row").value<LazyRow*>();
                    if (row && row->active() && row->key() == "50000")
                        return item->mapToItem(&view, QPointF()).y();
                }
            }
            return std::numeric_limits<qreal>::quiet_NaN();
        };
        const qreal screenY = anchorPosition();
        QVERIFY(std::isfinite(screenY));
        auto        data   = source.items();
        for (int i = 0; i < 100; ++i)
            data.prepend(QVariantMap { { "id", QStringLiteral("new%1").arg(i) }, { "height", 40 } });
        source.setItems(data);
        frames(render);
        QCOMPARE(view.firstVisibleIndex(), first + 100);
        QCOMPARE(source.indexOfKey("50000"), view.firstVisibleIndex());
        QCOMPARE(anchorPosition(), screenY);
        QVERIFY(qAbs(view.contentY() - offset - 100 * view.estimatedItemExtent()) < 0.001);
        view.setSource(nullptr);
        frames(render);
        QCOMPARE(view.liveCount(), 0);
        QCOMPARE(view.contentHeight(), 0.0);
    }
    void dynamicDelegateMeasurement() {
        QQmlEngine engine;
        QQuickRenderControl render;
        QQuickWindow window(&render);
        QQmlComponent delegate(&engine);
        delegate.setData(R"(
            import QtQuick
            import Test 1.0
            Item {
                required property LazyRow row
                property real imageHeight: 0
                implicitHeight: label.implicitHeight + imageHeight
                Text {
                    id: label
                    width: parent.width
                    font.pixelSize: 16
                    wrapMode: Text.Wrap
                    text: "Row " + row.key + " " + "Variable length text. ".repeat(8)
                }
            }
        )", QUrl());
        QVERIFY2(delegate.isReady(), qPrintable(delegate.errorString()));
        ListSnapshotSource source;
        source.setKeyRole("id");
        source.setItems(rows(1000));
        LazyList view(window.contentItem());
        QQmlEngine::setContextForObject(&view, engine.rootContext());
        view.setSize({300, 200});
        view.setCacheExtent(500);
        view.setSource(&source);
        view.setDelegate(&delegate);
        frames(render);
        QVERIFY(view.positionAtKey("500", 0, 17));
        frames(render);
        const auto itemFor = [&](const QString& key) -> QQuickItem* {
            for (auto* host : view.contentItem()->childItems()) {
                for (auto* item : host->childItems()) {
                    const auto* row = item->property("row").value<LazyRow*>();
                    if (row && row->active() && row->key() == key) return item;
                }
            }
            return nullptr;
        };
        auto* anchor = itemFor("500");
        QPointer<QQuickItem> previous = itemFor("499");
        QVERIFY(anchor);
        QVERIFY(previous);
        const qreal screenY = anchor->mapToItem(&view, QPointF()).y();
        const qreal originalHeight = anchor->height();
        const qreal oldOffset = view.contentY();
        const auto revision = source.revision();
        QTimer::singleShot(0, previous, [previous] {
            if (previous) previous->setProperty("imageHeight", 120);
        });
        frames(render);
        QCOMPARE(source.revision(), revision);
        QVERIFY(itemFor("500"));
        QCOMPARE(itemFor("500")->mapToItem(&view, QPointF()).y(), screenY);
        QCOMPARE(view.contentY(), oldOffset + 120);
        QCOMPARE(view.firstVisibleIndex(), 500);

        view.setWidth(150);
        frames(render);
        anchor = itemFor("500");
        QVERIFY(anchor);
        QCOMPARE(anchor->width(), 150.0);
        QVERIFY(anchor->height() > originalHeight);
        QCOMPARE(anchor->mapToItem(&view, QPointF()).y(), screenY);
        QCOMPARE(view.firstVisibleIndex(), 500);
        auto* next = itemFor("501");
        QVERIFY(next);
        QCOMPARE(next->mapToItem(&view, QPointF()).y(), screenY + anchor->height());

        view.setWidth(300);
        frames(render);
        anchor = itemFor("500");
        QVERIFY(anchor);
        QCOMPARE(anchor->height(), originalHeight);
        QCOMPARE(anchor->mapToItem(&view, QPointF()).y(), screenY);
        QVERIFY(view.liveCount() < 40);
        QVERIFY2(view.errorString().isEmpty(), qPrintable(view.errorString()));
    }
    void invalidHeight_data() {
        QTest::addColumn<double>("extent");
        QTest::newRow("negative") << -1.0;
        QTest::newRow("infinite") << std::numeric_limits<double>::infinity();
    }
    void invalidHeight() {
        QFETCH(double, extent);
        QQmlEngine engine;
        QQuickRenderControl render;
        QQuickWindow window(&render);
        QQmlComponent delegate(&engine);
        delegate.setData("import QtQuick\nimport Test 1.0\nItem { required property LazyRow row; height: row.value.height }", QUrl());
        QVERIFY(delegate.isReady());
        ListSnapshotSource source;
        source.setKeyRole("id");
        source.setItems({QVariantMap {{"id", "invalid"}, {"height", extent}}});
        LazyList view(window.contentItem());
        QQmlEngine::setContextForObject(&view, engine.rootContext());
        view.setSize({300, 200});
        view.setSource(&source);
        view.setDelegate(&delegate);
        frames(render);
        QVERIFY(! view.errorString().isEmpty());
        QVERIFY(std::isfinite(view.contentHeight()));
        QSignalSpy layouts(&view, &LazyList::layoutChanged);
        frames(render, 20);
        QCOMPARE(layouts.count(), 0);
        QVERIFY(view.positionAtKey("invalid"));
        frames(render);
        QVERIFY(! view.positioning());
        auto* host = view.contentItem()->childItems().first();
        auto* item = host->childItems().first();
        item->setHeight(0);
        frames(render);
        QVERIFY2(view.errorString().isEmpty(), qPrintable(view.errorString()));
        QCOMPARE(view.contentHeight(), 0.0);
        item->setHeight(80);
        frames(render);
        QCOMPARE(view.contentHeight(), 80.0);
    }
    void zeroHeightBudget() {
        QQmlEngine          engine;
        QQuickRenderControl render;
        QQuickWindow        window(&render);
        QQmlComponent       delegate(&engine);
        delegate.setData(
            "import QtQuick\nimport Test 1.0\nItem { required property LazyRow row; height: 0 }",
            QUrl());
        ListSnapshotSource source;
        source.setKeyRole("id");
        source.setItems(rows(10000));
        LazyList view(window.contentItem());
        QQmlEngine::setContextForObject(&view, engine.rootContext());
        view.setSize({ 300, 200 });
        view.setSource(&source);
        view.setDelegate(&delegate);
        frames(render, 100);
        QVERIFY(view.liveCount() <= 256);
        QVERIFY(! view.errorString().isEmpty());
    }
    void zeroHeightRecovery() {
        QQmlEngine engine;
        QQuickRenderControl render;
        QQuickWindow window(&render);
        QQmlComponent delegate(&engine);
        delegate.setData("import QtQuick\nimport Test 1.0\nItem { required property LazyRow row; height: 0 }", QUrl());
        QVERIFY(delegate.isReady());
        ListSnapshotSource source;
        source.setKeyRole("id");
        source.setItems(rows(10000));
        LazyList view(window.contentItem());
        QQmlEngine::setContextForObject(&view, engine.rootContext());
        view.setSize({300, 200});
        view.setSource(&source);
        view.setDelegate(&delegate);
        frames(render, 100);
        QVERIFY(! view.errorString().isEmpty());
        QQuickItem* recovered = nullptr;
        QString key;
        for (auto* host : view.contentItem()->childItems()) {
            if (host->childItems().isEmpty()) continue;
            recovered = host->childItems().first();
            key = recovered->property("row").value<LazyRow*>()->key();
            break;
        }
        QVERIFY(recovered);
        const int limitedCount = view.liveCount();
        recovered->setHeight(10);
        recovered->setHeight(0);
        frames(render, 100);
        QCOMPARE(view.liveCount(), limitedCount);
        QVERIFY(! view.errorString().isEmpty());
        recovered->setHeight(600);
        frames(render);
        QVERIFY2(view.errorString().isEmpty(), qPrintable(view.errorString()));
        QCOMPARE(view.firstVisibleIndex(), source.indexOfKey(key));
        QCOMPARE(view.lastVisibleIndex(), source.indexOfKey(key));
        QCOMPARE(recovered->mapToItem(&view, QPointF()).y(), 0.0);
        QVERIFY(view.liveCount() < 30);
    }
    void paginationDemand() {
        QQmlEngine          engine;
        QQuickRenderControl render;
        QQuickWindow        window(&render);
        QQmlComponent       delegate(&engine);
        delegate.setData("import QtQuick\nimport Test 1.0\nItem { required property LazyRow row; "
                         "implicitHeight: row.value.height }",
                         QUrl());
        PagedRows       model;
        ItemModelSource source;
        source.setKeyRole("id");
        source.setModel(&model);
        LazyList view(window.contentItem());
        QQmlEngine::setContextForObject(&view, engine.rootContext());
        view.setSize({ 300, 200 });
        view.setCacheExtent(0);
        view.setSource(&source);
        view.setDelegate(&delegate);
        frames(render, 40);
        QCOMPARE(model.requests, 3);
        QCOMPARE(model.count, 6);
        QCOMPARE(view.contentHeight(), 240.0);
        view.setHeight(400);
        frames(render, 40);
        QCOMPARE(model.requests, 6);
        QCOMPARE(model.count, 12);
        model.progress = false;
        view.setHeight(800);
        frames(render, 40);
        QCOMPARE(model.requests, 7);
        frames(render, 40);
        QCOMPARE(model.requests, 7);
        PagedRows replacement;
        replacement.count = model.count;
        source.setModel(&replacement);
        frames(render, 60);
        QCOMPARE(replacement.count, 20);
        QCOMPARE(replacement.requests, 4);
    }
    void paginationAfterMeasurement() {
        QQmlEngine engine;
        QQuickRenderControl render;
        QQuickWindow window(&render);
        engine.rootContext()->setContextProperty("rowHeight", 40);
        QQmlComponent delegate(&engine);
        delegate.setData("import QtQuick\nimport Test 1.0\nItem { required property LazyRow row; implicitHeight: rowHeight }", QUrl());
        QVERIFY(delegate.isReady());
        PagedRows model;
        ItemModelSource source;
        source.setKeyRole("id");
        source.setModel(&model);
        LazyList view(window.contentItem());
        QQmlEngine::setContextForObject(&view, engine.rootContext());
        view.setSize({300, 200});
        view.setCacheExtent(0);
        view.setSource(&source);
        view.setDelegate(&delegate);
        frames(render, 40);
        QCOMPARE(model.count, 6);
        engine.rootContext()->setContextProperty("rowHeight", 20);
        frames(render, 60);
        QCOMPARE(model.count, 12);
        QCOMPARE(model.requests, 6);
        QVERIFY(view.contentHeight() >= 240.0);
        QVERIFY(view.estimatedContentHeight());
        frames(render, 40);
        QCOMPARE(model.requests, 6);
    }
    void asynchronousPagination() {
        QQmlEngine engine;
        QQuickRenderControl render;
        QQuickWindow window(&render);
        QQmlComponent delegate(&engine);
        delegate.setData("import QtQuick\nimport Test 1.0\nItem { required property LazyRow row; "
                         "implicitHeight: 40 }", QUrl());
        AsyncRows model;
        ItemModelSource source;
        source.setKeyRole("id");
        source.setModel(&model);
        const auto readiness = connect(&model, &AsyncRows::readyChanged, &source,
                                       &ItemModelSource::notifyFetchStateChanged);
        LazyList view(window.contentItem());
        QQmlEngine::setContextForObject(&view, engine.rootContext());
        view.setSize({300, 200});
        view.setCacheExtent(0);
        view.setSource(&source);
        view.setDelegate(&delegate);
        frames(render, 20);
        QCOMPARE(model.requests, 1);
        for (int i = 0; i < 3; ++i) {
            frames(render, 20);
            QCOMPARE(model.requests, i + 1);
            model.complete();
            frames(render, 20);
        }
        QCOMPARE(model.requests, 3);
        QCOMPARE(view.count(), 6);
        view.setHeight(400);
        frames(render);
        QCOMPARE(model.requests, 4);
        disconnect(readiness);
        AsyncRows replacement;
        source.setModel(&replacement);
        connect(&replacement, &AsyncRows::readyChanged, &source,
                &ItemModelSource::notifyFetchStateChanged);
        frames(render);
        QCOMPARE(replacement.requests, 1);
        model.complete();
        frames(render, 20);
        QCOMPARE(view.count(), 0);
        QCOMPARE(replacement.requests, 1);
        replacement.complete(false);
        frames(render, 20);
        QCOMPARE(replacement.requests, 2);
        frames(render, 20);
        QCOMPARE(replacement.requests, 2);
        replacement.complete();
        frames(render, 20);
        QCOMPARE(view.count(), 2);
        QCOMPARE(replacement.requests, 3);
    }
    void measuredPositioning() {
        QQmlEngine          engine;
        QQuickRenderControl render;
        QQuickWindow        window(&render);
        QQmlComponent       delegate(&engine);
        delegate.setData("import QtQuick\nimport Test 1.0\nItem { required property LazyRow row; "
                         "implicitHeight: row.value.height }",
                         QUrl());
        ListSnapshotSource source;
        source.setKeyRole("id");
        source.setItems(rows(10000));
        LazyList view(window.contentItem());
        QQmlEngine::setContextForObject(&view, engine.rootContext());
        view.setSize({ 300, 200 });
        view.setSource(&source);
        view.setDelegate(&delegate);
        QSignalSpy finished(&view, &LazyList::positioningFinished);
        QVERIFY(view.positionAtIndex(5000, 0.5));
        QVERIFY(view.positioning());
        auto data = source.items();
        data.prepend(QVariantMap { { "id", "new" }, { "height", 80 } });
        source.setItems(data);
        frames(render, 15);
        QVERIFY(! view.positioning());
        QCOMPARE(finished.size(), 1);
        QCOMPARE(finished.last().at(0).toString(), QStringLiteral("5000"));
        QVERIFY(finished.last().at(1).toBool());
        bool found = false;
        for (auto* host : view.contentItem()->childItems()) {
            for (auto* item : host->childItems()) {
                auto* row = item->property("row").value<LazyRow*>();
                if (! row || row->key() != "5000") continue;
                found = true;
                QCOMPARE(item->mapToItem(&view, QPointF()).y(), (200 - item->height()) / 2);
            }
        }
        QVERIFY(found);
        QVERIFY(view.liveCount() < 30);
        QVERIFY(view.positionAtKey("6000"));
        QVERIFY(view.positionAtKey("7000"));
        frames(render, 15);
        QCOMPARE(finished.last().at(0).toString(), QStringLiteral("7000"));
        QVERIFY(finished.last().at(1).toBool());
        QVERIFY(view.positionAtKey("8000"));
        view.consumeScroll({ 0, 10 }, Flickable::ScrollInput::Direct);
        QVERIFY(! view.positioning());
        QVERIFY(! finished.last().at(1).toBool());
        QVERIFY(view.positionAtKey("9000"));
        data = source.items();
        data.removeAt(source.indexOfKey("9000"));
        source.setItems(data);
        frames(render, 15);
        QVERIFY(! view.positioning());
        QVERIFY(! finished.last().at(1).toBool());
        QVERIFY(! view.positionAtKey("missing"));
    }
    void positioningLifecycle_data() {
        QTest::addColumn<int>("change");
        QTest::newRow("hidden") << 0;
        QTest::newRow("disabled") << 1;
        QTest::newRow("noninteractive") << 2;
        QTest::newRow("detached") << 3;
        QTest::newRow("window-hidden") << 4;
        QTest::newRow("window-replaced") << 5;
    }
    void positioningDependencies_data() {
        QTest::addColumn<bool>("sourceChange");
        QTest::addColumn<bool>("destroy");
        QTest::newRow("clear-source") << true << false;
        QTest::newRow("destroy-source") << true << true;
        QTest::newRow("clear-delegate") << false << false;
        QTest::newRow("destroy-delegate") << false << true;
    }
    void dependencyCancellationReentry() {
        ListSnapshotSource original, requested, replacement;
        for (auto* source : {&original, &requested, &replacement}) {
            source->setKeyRole("id");
            source->setItems(rows(2));
        }
        LazyList view;
        view.setSource(&original);
        QVERIFY(view.positionAtKey("1"));
        connect(&view, &LazyList::positioningFinished, &view,
                [&](const QString&, bool success) {
            QVERIFY(! success);
            QCOMPARE(view.source(), &requested);
            view.setSource(&replacement);
        });
        view.setSource(&requested);
        QCOMPARE(view.source(), &replacement);
        QVERIFY(! view.positioning());

        QPointer<LazyList> disposable = new LazyList;
        disposable->setSource(&original);
        QVERIFY(disposable->positionAtKey("1"));
        connect(disposable, &LazyList::positioningFinished, disposable,
                [disposable] { delete disposable.data(); });
        disposable->setSource(nullptr);
        QVERIFY(disposable.isNull());
    }
    void positioningDependencies() {
        QFETCH(bool, sourceChange);
        QFETCH(bool, destroy);
        QQmlEngine engine;
        QQuickRenderControl render;
        QQuickWindow window(&render);
        auto delegate = std::make_unique<QQmlComponent>(&engine);
        delegate->setData("import QtQuick\nimport Test 1.0\nItem { required property LazyRow row; "
                          "implicitHeight: 40 }", QUrl());
        auto source = std::make_unique<ListSnapshotSource>();
        source->setKeyRole("id");
        source->setItems(rows(1000));
        LazyList view(window.contentItem());
        QQmlEngine::setContextForObject(&view, engine.rootContext());
        view.setSize({300, 200});
        view.setSource(source.get());
        view.setDelegate(delegate.get());
        frames(render);
        QSignalSpy finished(&view, &LazyList::positioningFinished);
        QVERIFY(view.positionAtKey("800", 0, 0, LazyList::Smooth));
        frames(render, 2);
        QVERIFY(view.isMoving());
        if (sourceChange) {
            if (destroy) source.reset();
            else view.setSource(nullptr);
        } else {
            if (destroy) delegate.reset();
            else view.setDelegate(nullptr);
        }
        QVERIFY(! view.positioning());
        QVERIFY(! view.isMoving());
        QCOMPARE(finished.size(), 1);
        QVERIFY(! finished.first().at(1).toBool());
        frames(render);
        QCOMPARE(view.liveCount(), 0);
        QCOMPARE(finished.size(), 1);
    }
    void positioningLifecycle() {
        QFETCH(int, change);
        QQmlEngine engine;
        QQuickRenderControl render;
        QQuickWindow window(&render);
        QQuickRenderControl otherRender;
        QQuickWindow otherWindow(&otherRender);
        QQmlComponent delegate(&engine);
        delegate.setData("import QtQuick\nimport Test 1.0\nItem { required property LazyRow row; "
                         "implicitHeight: 40 }", QUrl());
        ListSnapshotSource source;
        source.setKeyRole("id");
        source.setItems(rows(1000));
        LazyList view(window.contentItem());
        QQmlEngine::setContextForObject(&view, engine.rootContext());
        view.setSize({300, 200});
        view.setSource(&source);
        view.setDelegate(&delegate);
        if (change == 4) window.setVisible(true);
        frames(render);
        QSignalSpy finished(&view, &LazyList::positioningFinished);
        QVERIFY(view.positionAtKey("800", 0, 0, LazyList::Smooth));
        frames(render, 2);
        QVERIFY(view.positioning());
        QVERIFY(view.isMoving());
        switch (change) {
        case 0: view.setVisible(false); break;
        case 1: view.setEnabled(false); break;
        case 2: view.setInteractive(false); break;
        case 3: view.setParentItem(nullptr); break;
        case 4: window.setVisible(false); break;
        case 5: view.setParentItem(otherWindow.contentItem()); break;
        }
        QVERIFY(! view.positioning());
        QVERIFY(! view.isMoving());
        QCOMPARE(finished.size(), 1);
        QCOMPARE(finished.first().at(0).toString(), QString("800"));
        QVERIFY(! finished.first().at(1).toBool());
        const auto stopped = view.contentY();
        view.setVisible(true);
        view.setEnabled(true);
        view.setInteractive(true);
        view.setParentItem(window.contentItem());
        frames(render);
        QCOMPARE(view.contentY(), stopped);
        QCOMPARE(finished.size(), 1);
    }
};

QTEST_MAIN(LazyListTest)
#include "lazy_list.moc"
