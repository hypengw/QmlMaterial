#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QtTest>
#include <memory>

class PageLayoutTest : public QObject {
    Q_OBJECT

private:
    QQmlEngine   m_engine;
    QQuickWindow m_window;

    std::unique_ptr<QObject> create(const QByteArray& type) {
        QQmlComponent component(&m_engine);
        component.setData("import Qcm.Material\n" + type + " {}", QUrl());
        return std::unique_ptr<QObject>(component.create());
    }

    void inherit(QObject* child, QObject* parent) {
        QVERIFY(child->setProperty("inherit", QVariant::fromValue(parent)));
    }

    void settle(QQuickItem* item) {
        for (int i = 0; i < 10; ++i) {
            item->ensurePolished();
            for (auto* child : item->childItems()) child->ensurePolished();
            QCoreApplication::processEvents();
        }
    }

private Q_SLOTS:
    void initTestCase() {
        m_engine.addImportPath(QCoreApplication::applicationDirPath() + "/../qml_modules");
        m_window.resize(800, 600);
        m_window.create();
    }

    void inheritance_data() {
        QTest::addColumn<QByteArray>("property");
        QTest::addColumn<QVariant>("value");
        QTest::newRow("left") << QByteArray("leftMargin") << QVariant(4);
        QTest::newRow("right") << QByteArray("rightMargin") << QVariant(12);
        QTest::newRow("top") << QByteArray("topMargin") << QVariant(16);
        QTest::newRow("bottom") << QByteArray("bottomMargin") << QVariant(20);
        QTest::newRow("radius") << QByteArray("radius") << QVariant(24);
        QTest::newRow("background-radius") << QByteArray("backgroundRadius") << QVariant(28);
        QTest::newRow("header-type") << QByteArray("headerType") << QVariant(3);
        QTest::newRow("header") << QByteArray("showHeader") << QVariant(true);
        QTest::newRow("background") << QByteArray("showBackground") << QVariant(true);
        QTest::newRow("opacity") << QByteArray("headerBackgroundOpacity") << QVariant(0.5);
    }

    void inheritance() {
        QFETCH(QByteArray, property);
        QFETCH(QVariant, value);
        auto parent     = create("PageContext");
        auto child      = create("PageContext");
        auto grandchild = create("PageContext");
        QVERIFY(parent && child && grandchild);
        inherit(grandchild.get(), child.get());
        inherit(child.get(), parent.get());
        const auto prop =
            child->metaObject()->property(child->metaObject()->indexOfProperty(property));
        QSignalSpy     spy(grandchild.get(), prop.notifySignal());
        const QVariant fallback = child->property(property);
        QVERIFY(parent->setProperty(property, value));
        QCOMPARE(grandchild->property(property), value);
        QCOMPARE(spy.count(), 1);

        QVERIFY(child->setProperty(property, fallback));
        spy.clear();
        QVERIFY(parent->setProperty(property, fallback));
        QCOMPARE(spy.count(), 0);
        QVERIFY(parent->setProperty(property, value));
        QCOMPARE(spy.count(), 0);
        QVERIFY(prop.reset(child.get()));
        QCOMPARE(grandchild->property(property), value);
        QCOMPARE(spy.count(), 1);

        auto replacement = create("PageContext");
        inherit(child.get(), replacement.get());
        QCOMPARE(grandchild->property(property), fallback);
        spy.clear();
        QVERIFY(parent->setProperty(property, fallback));
        QCOMPARE(spy.count(), 0);
        inherit(child.get(), parent.get());
        QVERIFY(parent->setProperty(property, value));
        parent.reset();
        QCOMPARE(child->property("inherit").value<QObject*>(), nullptr);
        QCOMPARE(grandchild->property(property), fallback);
        inherit(replacement.get(), grandchild.get());
        inherit(child.get(), replacement.get());
        QCOMPARE(child->property("inherit").value<QObject*>(), nullptr);
    }

    void actionReset() {
        auto parent = create("PageContext");
        auto child  = create("PageContext");
        QVERIFY(parent && child);
        QObject action;
        parent->setProperty("leadingAction", QVariant::fromValue(&action));
        inherit(child.get(), parent.get());
        QCOMPARE(child->property("leadingAction").value<QObject*>(), &action);
        child->setProperty("leadingAction", QVariant::fromValue<QObject*>(nullptr));
        QCOMPARE(child->property("leadingAction").value<QObject*>(), nullptr);
        QVERIFY(QMetaObject::invokeMethod(child.get(), "resetLeadingAction"));
        QCOMPARE(child->property("leadingAction").value<QObject*>(), &action);
    }

    void widthInputs() {
        auto provider = create("WidthProvider");
        QVERIFY(provider);
        provider->setProperty("total", 500);
        QCOMPARE(provider->property("column").toInt(), 3);
        provider->setProperty("minimum", 200);
        QCOMPARE(provider->property("column").toInt(), 2);
        QCOMPARE(provider->property("width").toReal(), 250.0);
        provider->setProperty("spacing", 12);
        QCOMPARE(provider->property("width").toReal(), 244.0);
        provider->setProperty("leftMargin", 4);
        QCOMPARE(provider->property("width").toReal(), 242.0);
        provider->setProperty("rightMargin", 12);
        QCOMPARE(provider->property("width").toReal(), 236.0);
        provider->setProperty("total", 10);
        QCOMPARE(provider->property("width").toReal(), 0.0);
        QCOMPARE(provider->property("column").toInt(), 1);
        provider->setProperty("total", 500);
        QCOMPARE(provider->property("width").toReal(), 236.0);
        provider->setProperty("minimum", 0);
        QCOMPARE(provider->property("width").toReal(), 0.0);
        QCOMPARE(provider->property("column").toInt(), 1);
    }

    void pageViews_data() {
        QTest::addColumn<QString>("type");
        QTest::newRow("list") << QString("PageVerticalListView");
        QTest::newRow("flickable") << QString("PageVerticalFlickable");
        QTest::newRow("horizontal") << QString("PageHorizontalListView");
    }

    void pageViews() {
        QFETCH(QString, type);
        QQmlComponent component(&m_engine);
        component.setData(QString(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                width: 500; height: 300
                property alias context: ctx
                MD.MProp.page: ctx
                MD.PageContext { id: ctx; leftMargin: 4; rightMargin: 12; topMargin: 16; bottomMargin: 20 }
                MD.%1 {
                    id: view
                    objectName: "view"
                    anchors.fill: parent
                    extraBottomMargin: 8
                }
            }
        )")
                              .arg(type)
                              .toUtf8(),
                          QUrl("qrc:/tests/page-views.qml"));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(m_window.contentItem());
        auto* view = root->findChild<QQuickItem*>("view");
        auto* ctx  = qvariant_cast<QObject*>(root->property("context"));
        QVERIFY(view);
        QVERIFY(ctx);
        settle(root);
        QCOMPARE(view->width(), 500.0);
        QCOMPARE(view->property("leftMargin").toReal(), 4.0);
        QCOMPARE(view->property("rightMargin").toReal(), 12.0);
        const bool horizontal = type == "PageHorizontalListView";
        QCOMPARE(view->property("topMargin").toReal(), horizontal ? 0.0 : 16.0);
        QCOMPARE(view->property("bottomMargin").toReal(), horizontal ? 8.0 : 28.0);
        ctx->setProperty("rightMargin", 40);
        QCOMPARE(view->property("rightMargin").toReal(), 40.0);
        QVERIFY(view->setProperty("pageMarginEdges", int(Qt::TopEdge | Qt::BottomEdge)));
        QCOMPARE(view->property("leftMargin").toReal(), 0.0);
        QCOMPARE(view->property("rightMargin").toReal(), 0.0);
        QCOMPARE(view->property("bottomMargin").toReal(), 28.0);
        QCOMPARE(view->width(), 500.0);
    }

    void horizontalCards() {
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                id: root
                width: 500; height: 300
                property alias rightMargin: ctx.rightMargin
                MD.MProp.page: MD.PageContext { id: ctx; leftMargin: 4; rightMargin: 12 }
                function aligned() {
                    const last = view.itemAtIndex(sizing.column - 1);
                    const next = view.itemAtIndex(sizing.column);
                    return last !== null && next !== null && view.clip
                        && Math.abs(last.mapToItem(root, last.width, 0).x - (width - ctx.rightMargin)) < 0.01
                        && next.mapToItem(root, 0, 0).x >= width - 0.01;
                }
                MD.PageHorizontalListView {
                    id: view
                    anchors.fill: parent
                    spacing: Math.max(12, rightMargin)
                    cacheBuffer: 1000
                    model: 20
                    delegate: Item { width: sizing.width; height: 100 }
                    MD.WidthProvider {
                        id: sizing
                        total: view.width
                        minimum: 160
                        spacing: view.spacing
                        leftMargin: view.leftMargin
                        rightMargin: view.rightMargin
                    }
                }
            }
        )",
                          QUrl("qrc:/tests/horizontal-cards.qml"));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(m_window.contentItem());
        for (const auto width : { 500, 700, 375 }) {
            root->setWidth(width);
            for (const auto margin : { 12, 20 }) {
                root->setProperty("rightMargin", margin);
                settle(root);
                QVariant aligned;
                QVERIFY(
                    QMetaObject::invokeMethod(root, "aligned", Q_RETURN_ARG(QVariant, aligned)));
                QVERIFY(aligned.toBool());
            }
        }
    }

    void contentGeometry() {
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                width: 500; height: 400
                function itemInsets() {
                    const bg = view.currentItem.background;
                    return [bg.mapToItem(pane, 0, 0).x,
                            pane.width - bg.mapToItem(pane, bg.width, 0).x];
                }
                MD.VerticalListView {
                    id: view
                    objectName: "view"
                    anchors.fill: parent
                    leftMargin: 4; rightMargin: 12
                    topMargin: 16; bottomMargin: 20
                    busy: true
                    model: 20
                    delegate: MD.ListItem {
                        width: ListView.view.contentWidth; height: 40
                        leftMargin: 16; rightMargin: 16
                    }
                }
                MD.FlickablePane {
                    id: pane
                    objectName: "pane"
                    view: view
                    excludeBegin: 40
                    excludeEnd: 24
                }
            }
        )",
                          QUrl("qrc:/tests/content-geometry.qml"));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(m_window.contentItem());
        settle(root);
        auto* view   = root->findChild<QQuickItem*>("view");
        auto* pane   = root->findChild<QQuickItem*>("pane");
        auto* footer = qvariant_cast<QQuickItem*>(view->property("footerItem"));
        QVERIFY(footer);
        QCOMPARE(footer->width(), 484.0);
        QCOMPARE(pane->width(), 500.0);
        QCOMPARE(pane->height(), pane->property("contentHeight").toReal() - 40 - 24 + 36);
        QVERIFY(pane->setProperty("includeHorizontalMargins", false));
        QCOMPARE(pane->x(), 4.0);
        QCOMPARE(pane->width(), 484.0);
        view->setProperty("rightMargin", 24);
        settle(root);
        QCOMPARE(pane->width(), 472.0);
        QCOMPARE(footer->width(), 472.0);
        view->setProperty("contentWidth", 800);
        settle(root);
        for (const auto contentX : {-4, 0, 12}) {
            view->setProperty("contentX", contentX);
            QCOMPARE(view->property("contentX").toReal(), qreal(contentX));
            QVariant insets;
            QVERIFY(QMetaObject::invokeMethod(root, "itemInsets", Q_RETURN_ARG(QVariant, insets)));
            const auto values = insets.value<QJSValue>().toVariant().toList();
            QCOMPARE(values.size(), 2);
            QCOMPARE(values[0].toReal(), 16.0);
            QCOMPARE(values[1].toReal(), 16.0);
        }
    }

    void tableGeometry() {
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qt.labs.qmlmodels
            import Qcm.Material as MD
            Item {
                id: root
                width: 500; height: 400
                function aligned() {
                    const cell = table.itemAtCell(Qt.point(0, 0));
                    const heading = header.itemAtCell(Qt.point(0, 0));
                    return cell !== null && heading !== null
                        && Math.abs(cell.mapToItem(root, 0, 0).x - heading.mapToItem(root, 0, 0).x) < 0.01;
                }
                MD.TableView {
                    id: table
                    objectName: "table"
                    width: 500; height: 300
                    leftMargin: 4; rightMargin: 12; topMargin: 16; bottomMargin: 20
                    model: TableModel {
                        TableModelColumn { display: "name" }
                        TableModelColumn { display: "value" }
                        rows: [{name: "one", value: "two"}]
                    }
                    columnWidthProvider: column => viewportContentWidth / 2
                    delegate: Item { implicitWidth: 100; implicitHeight: 40 }
                }
                MD.HorizontalHeaderView {
                    id: header
                    objectName: "header"
                    syncView: table
                    followSyncViewMargins: true
                    model: ["Name", "Value"]
                    delegate: Item { implicitWidth: 100; implicitHeight: 32 }
                }
            }
        )",
                          QUrl("qrc:/tests/table-geometry.qml"));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(m_window.contentItem());
        settle(root);
        auto* table  = root->findChild<QQuickItem*>("table");
        auto* header = root->findChild<QQuickItem*>("header");
        QCOMPARE(table->property("viewportContentWidth").toReal(), 484.0);
        QCOMPARE(table->property("viewportContentHeight").toReal(), 264.0);
        QCOMPARE(header->x(), 0.0);
        QCOMPARE(header->width(), 500.0);
        auto* outline = header->findChild<QQuickItem*>("headerOutline");
        QVERIFY(outline);
        QCOMPARE(outline->x(), 4.0);
        QCOMPARE(outline->width(), 484.0);
        QVariant aligned;
        QVERIFY(QMetaObject::invokeMethod(root, "aligned", Q_RETURN_ARG(QVariant, aligned)));
        QVERIFY(aligned.toBool());
        table->setProperty("rightMargin", 24);
        settle(root);
        QCOMPARE(outline->width(), 472.0);
        QVERIFY(QMetaObject::invokeMethod(root, "aligned", Q_RETURN_ARG(QVariant, aligned)));
        QVERIFY(aligned.toBool());
        header->setProperty("followSyncViewMargins", false);
        QCOMPARE(header->width(), 500.0);
        QCOMPARE(header->x(), 0.0);
        QCOMPARE(outline->width(), 500.0);
    }
};

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    PageLayoutTest  test;
    return QTest::qExec(&test, argc, argv);
}

#include "page_layout.moc"
