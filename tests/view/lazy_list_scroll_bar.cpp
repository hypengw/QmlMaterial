#include <QCoreApplication>
#include <QColor>
#include <QFont>
#include <QFontDatabase>
#include <QFile>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlProperty>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QStandardItemModel>
#include <QtTest>
#include <algorithm>
#include <memory>

#include "qml_material/view/lazy_list.hpp"
#include "qml_material/control/scroll_bar.hpp"
#include "qml_material/model/item_source.hpp"

namespace
{

void settle(QQuickItem* item, int rounds = 8) {
    for (int round = 0; round < rounds; ++round) {
        item->ensurePolished();
        for (auto* child : item->childItems()) {
            settle(child, 1);
        }
        QCoreApplication::processEvents();
    }
}
} // namespace

class LazyListScrollBarTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        m_engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        m_window.setGeometry(0, 0, 800, 600);
        m_window.create();
    }

    void lazyListScrollBarEstimates() {
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.LazyList {
                width: 300; height: 100
                estimatedItemExtent: 80
                cacheExtent: 0
                source: MD.ListSnapshotSource {
                    keyRole: "key"
                    Component.onCompleted: {
                        let rows = [];
                        for (let i = 0; i < 20; ++i) rows.push({key: String(i)});
                        items = rows;
                    }
                }
                delegate: Item { required property MD.LazyRow row; implicitHeight: 40 }
                MD.ScrollBar.vertical: MD.ScrollBar { objectName: "bar" }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* list = qobject_cast<qml_material::LazyList*>(object.get());
        QVERIFY(list);
        list->setParentItem(m_window.contentItem());
        settle(list);
        auto* bar = list->findChild<qml_material::ScrollBar*>("bar");
        QVERIFY(bar);
        QVERIFY(list->estimatedContentHeight());
        QVERIFY(list->contentHeight() > 800);
        for (int index = 0; index < 20; ++index) {
            QVERIFY(list->positionAtIndex(index));
            settle(list);
            QCOMPARE(bar->property("size").toReal(), 100.0 / list->contentHeight());
            QCOMPARE(bar->property("position").toReal(), list->visibleArea()->yPosition());
        }
        QVERIFY(! list->estimatedContentHeight());
        QCOMPARE(list->contentHeight(), 800.0);
        qml_material::ListSnapshotSource replacement;
        replacement.setKeyRole("key");
        replacement.setItems({ QVariantMap { { "key", "replacement" } } });
        list->setSource(&replacement);
        settle(list);
        QCOMPARE(list->contentHeight(), 40.0);
        QCOMPARE(list->contentY(), 0.0);
        QCOMPARE(bar->property("size").toReal(), 1.0);
        QCOMPARE(bar->property("position").toReal(), 0.0);
        list->setSource(nullptr);
        settle(list);
        QCOMPARE(list->count(), 0);
        QCOMPARE(bar->property("position").toReal(), 0.0);
        list->setParentItem(nullptr);
    }

    void lazyListScrollBarDrag() {
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.LazyList {
                width: 300; height: 300
                estimatedItemExtent: 40
                cacheExtent: 0
                source: MD.ListSnapshotSource {
                    keyRole: "key"
                    Component.onCompleted: {
                        let rows = [];
                        for (let i = 0; i < 100; ++i) rows.push({key: String(i)});
                        items = rows;
                    }
                }
                delegate: Item { required property MD.LazyRow row; implicitHeight: 40 }
                MD.ScrollBar.vertical: MD.ScrollBar {
                    objectName: "bar"
                    width: 16
                    interactive: true
                    policy: MD.ScrollBarBase.AlwaysOn
                }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* list = qobject_cast<qml_material::LazyList*>(object.get());
        QVERIFY(list);
        QQuickWindow window;
        window.setGeometry(0, 0, 300, 300);
        list->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        settle(list);
        auto* bar = list->findChild<qml_material::ScrollBar*>("bar");
        QVERIFY(bar);
        const auto start = bar->mapToScene(QPointF(8, 8)).toPoint();
        const auto end   = bar->mapToScene(QPointF(8, 180)).toPoint();
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, start);
        QTRY_VERIFY(bar->interactionValues().dragged);
        QTest::mouseMove(&window, end);
        settle(list);
        QVERIFY(bar->interactionValues().dragged);
        QCOMPARE(window.mouseGrabberItem(), bar);
        QVERIFY(! list->isDragging());
        QVERIFY(list->contentY() > 1500);
        QCOMPARE(bar->property("position").toReal(), list->visibleArea()->yPosition());
        auto* source = qobject_cast<qml_material::ListSnapshotSource*>(list->source());
        QVERIFY(source);
        source->setItems(source->items().mid(0, 3));
        settle(list);
        QCOMPARE(list->contentY(), 0.0);
        QCOMPARE(bar->property("size").toReal(), 1.0);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, end);
        QTRY_VERIFY(! bar->interactionValues().dragged);
        settle(list);
        QCOMPARE(list->contentY(), 0.0);
        QCOMPARE(bar->property("position").toReal(), 0.0);
        QVERIFY(! list->isDragging());
        list->setParentItem(nullptr);
    }

private:
    QQmlEngine   m_engine;
    QQuickWindow m_window;
};

int run_lazy_list_scroll_bar(int argc, char** argv) {
    LazyListScrollBarTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "lazy_list_scroll_bar.moc"
