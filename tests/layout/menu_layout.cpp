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

#include "qml_material/control/icon_spec.hpp"
#include "qml_material/layout/layout_container.hpp"

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

QQuickItem* itemWithText(QQuickItem* root, const QString& text) {
    for (auto* child : root->childItems()) {
        if (auto* match = itemWithText(child, text)) {
            return match;
        }
    }
    return root->property("text").toString() == text ? root : nullptr;
}

QQuickItem* itemWithIconView(QQuickItem* root) {
    if (auto* icon = root->property("icon").value<qml_material::IconSpec*>();
        icon && ! icon->isEmpty()) {
        return root;
    }
    for (auto* child : root->childItems()) {
        if (auto* match = itemWithIconView(child)) return match;
    }
    return nullptr;
}

qml_material::Row* layoutRow(QQuickItem* root) {
    if (auto* row = qobject_cast<qml_material::Row*>(root)) {
        return row;
    }
    for (auto* child : root->childItems()) {
        if (auto* row = layoutRow(child)) {
            return row;
        }
    }
    return nullptr;
}
} // namespace

class MenuLayoutTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        m_engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        m_window.setGeometry(0, 0, 800, 600);
        m_window.create();
    }

    void menuItemBusyReplacesIcon() {
        const auto source = QByteArrayLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            MD.ActionMenuItem {
                text: "Syncing"
                action: MD.Action {
                    icon.name: MD.Token.icon.sync
                    busy: MD.Enum.Progress
                    progress: 0.5
                }
            }
        )");

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/busy-menu-item.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* menuItem = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(menuItem);
        menuItem->setParentItem(m_window.contentItem());
        settle(menuItem);

        auto* content = qvariant_cast<QQuickItem*>(menuItem->property("contentItem"));
        auto* row     = layoutRow(content);
        QVERIFY(row);
        QVERIFY(! row->childItems().empty());
        auto* leading = row->childItems().front();

        QQuickItem* icon   = itemWithIconView(leading);
        QQuickItem* loader = nullptr;
        for (auto* child : leading->childItems()) {
            if (child->property("active").isValid()) {
                loader = child;
            }
        }
        QVERIFY(icon);
        QVERIFY(loader);
        QVERIFY(! icon->isVisible());
        QVERIFY(loader->isVisible());
        QVERIFY(loader->property("active").toBool());
    }

    void menuConstrainsLongItem() {
        const QString text   = QStringLiteral("long long long long long long long long long");
        const auto    source = QStringLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            Item {
                width: 500
                height: 300

                MD.Menu {
                    objectName: "menu"
                    parent: parent
                    width: 240

                    MD.MenuItem {
                        objectName: "longItem"
                        text: "%1"
                        icon.name: MD.Token.icon.description
                    }
                }
            }
        )")
                                   .arg(text)
                                   .toUtf8();

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/long-menu-item.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(root);
        root->setParentItem(m_window.contentItem());

        auto* menu = root->findChild<QObject*>(QStringLiteral("menu"));
        QVERIFY(menu);
        QVERIFY(QMetaObject::invokeMethod(menu, "open"));
        QCoreApplication::processEvents();

        auto* menuItem = root->findChild<QQuickItem*>(QStringLiteral("longItem"));
        QVERIFY(menuItem);
        settle(menuItem);
        QVERIFY(menuItem->width() > 0);
        QVERIFY(menuItem->width() <= 240.0);

        auto* content = qvariant_cast<QQuickItem*>(menuItem->property("contentItem"));
        QVERIFY(content);
        auto* label = itemWithText(content, text);
        QVERIFY(label);
        QVERIFY(label->implicitWidth() > label->width());
        QVERIFY(label->property("truncated").toBool());

        QVERIFY(QMetaObject::invokeMethod(menu, "close"));
    }

    void menuContentDelegateReceivesWidth() {
        const QString text = QStringLiteral(
            "long long long long long long long long long long long long long long long");
        const auto source = QStringLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            Item {
                width: 500
                height: 300

                MD.Menu {
                    objectName: "menu"
                    parent: parent
                    width: 240
                    model: ["%1"]
                    contentDelegate: MD.MenuItem {
                        objectName: "contentDelegateItem"
                        required property string modelData
                        text: modelData
                    }
                }
            }
        )")
                                .arg(text)
                                .toUtf8();

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/menu-content-delegate.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(root);
        root->setParentItem(m_window.contentItem());

        auto* menu = root->findChild<QObject*>(QStringLiteral("menu"));
        QVERIFY(menu);
        QVERIFY(QMetaObject::invokeMethod(menu, "open"));
        QCoreApplication::processEvents();

        QQuickItem* delegate = nullptr;
        QTRY_VERIFY_WITH_TIMEOUT(
            QMetaObject::invokeMethod(
                menu, "itemAt", Q_RETURN_ARG(QQuickItem*, delegate), Q_ARG(int, 0)) &&
                delegate,
            3000);
        settle(delegate);
        QCOMPARE(menu->property("count").toInt(), 1);
        QCOMPARE(menu->property("implicitContentWidth").toReal(), delegate->implicitWidth());
        QCOMPARE(delegate->width(), menu->property("availableWidth").toReal());

        QVERIFY(menu->setProperty("width", 180.0));
        settle(delegate);
        QCOMPARE(delegate->width(), menu->property("availableWidth").toReal());

        auto* label = itemWithText(delegate, text);
        QVERIFY(label);
        QVERIFY(label->implicitWidth() > label->width());
        QVERIFY(label->property("truncated").toBool());

        QVERIFY(QMetaObject::invokeMethod(menu, "close"));
    }

    void actionMenuConstrainsLongItem() {
        const QString text = QStringLiteral(
            "long long long long long long long long long long long long long long long");
        const auto source = QStringLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            Item {
                id: testRoot
                width: 500
                height: 300

                property MD.Action longAction: MD.Action {
                    text: "%1"
                    icon.name: MD.Token.icon.description
                }

                MD.ActionMenu {
                    objectName: "menu"
                    parent: testRoot
                    actions: [testRoot.longAction]
                    itemDelegate: MD.ActionMenuItem { objectName: "actionItem" }
                }
            }
        )")
                                .arg(text)
                                .toUtf8();

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/long-action-menu-item.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(root);
        root->setParentItem(m_window.contentItem());

        auto* menu = root->findChild<QObject*>(QStringLiteral("menu"));
        QVERIFY(menu);
        QVERIFY(QMetaObject::invokeMethod(menu, "open"));
        QCoreApplication::processEvents();

        auto* actionItem = root->findChild<QQuickItem*>(QStringLiteral("actionItem"));
        QVERIFY(actionItem);
        settle(actionItem);

        QCOMPARE(menu->property("maximumWidth").toReal(), 280.0);
        QCOMPARE(menu->property("implicitContentWidth").toReal(), actionItem->implicitWidth());
        QCOMPARE(menu->property("implicitWidth").toReal(), 280.0);
        QCOMPARE(actionItem->width(), menu->property("availableWidth").toReal());

        auto* label = itemWithText(actionItem, text);
        QVERIFY(label);
        QVERIFY(label->implicitWidth() > label->width());
        QVERIFY(label->property("truncated").toBool());

        QVERIFY(QMetaObject::invokeMethod(menu, "close"));
    }

private:
    QQmlEngine   m_engine;
    QQuickWindow m_window;
};

int run_menu_layout(int argc, char** argv) {
    MenuLayoutTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "menu_layout.moc"
