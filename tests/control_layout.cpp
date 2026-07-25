#include <QCoreApplication>
#include <QFont>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QtTest>
#include <memory>

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

class ControlLayoutTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void constrainedTextElides_data() {
        QTest::addColumn<QString>("type");
        QTest::addColumn<QString>("setup");

        QTest::newRow("button") << QStringLiteral("Button")
                                << QStringLiteral("icon.name: MD.Token.icon.add");
        QTest::newRow("segmented-button")
            << QStringLiteral("SegmentedButton") << QStringLiteral("icon.name: MD.Token.icon.add");
        QTest::newRow("tab-button")
            << QStringLiteral("TabButton") << QStringLiteral("icon.name: MD.Token.icon.add");
        QTest::newRow("menu-item")
            << QStringLiteral("MenuItem") << QStringLiteral("icon.name: MD.Token.icon.add");
        QTest::newRow("assist") << QStringLiteral("AssistChip")
                                << QStringLiteral("icon.name: MD.Token.icon.search");
        QTest::newRow("filter") << QStringLiteral("FilterChip")
                                << QStringLiteral("checked: true; icon.name: MD.Token.icon.close");
        QTest::newRow("input")
            << QStringLiteral("InputChip")
            << QStringLiteral("leadingItem: Item { implicitWidth: 18; implicitHeight: 18 }\n"
                              "icon.name: MD.Token.icon.close");
        QTest::newRow("suggestion") << QStringLiteral("SuggestionChip")
                                    << QStringLiteral("icon.name: MD.Token.icon.search");
        QTest::newRow("embed") << QStringLiteral("EmbedChip")
                               << QStringLiteral(
                                      "icon.name: MD.Token.icon.search; trailingIconName: "
                                      "MD.Token.icon.close");
    }

    void constrainedTextElides() {
        QFETCH(QString, type);
        QFETCH(QString, setup);

        const QString text = QStringLiteral("A control label that is much wider than its control");
        const auto    source = QStringLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            MD.%1 {
                width: 112
                text: "%2"
                %3
            }
        )")
                                   .arg(type, text, setup)
                                   .toUtf8();

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/control-layout.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* control = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(control);
        control->setParentItem(m_window.contentItem());
        settle(control);

        auto* content = qvariant_cast<QQuickItem*>(control->property("contentItem"));
        QVERIFY(content);
        auto* row = layoutRow(content);
        QVERIFY(row);

        auto* label = itemWithText(content, text);
        QVERIFY(label);
        QVERIFY(control->implicitWidth() > control->width());
        QVERIFY(label->implicitWidth() > label->width());
        QCOMPARE(label->property("elide").toInt(), int(Qt::ElideRight));
        QCOMPARE(label->property("wrapMode").toInt(), 0);
        QVERIFY(label->property("truncated").isValid());
        QVERIFY(label->property("truncated").toBool());

        if (type == QStringLiteral("MenuItem")) {
            auto* arrow = qvariant_cast<QQuickItem*>(control->property("arrow"));
            QVERIFY(arrow);
            QCOMPARE(arrow->parentItem(), row);
            QVERIFY(! row->childItems().empty());
            QCOMPARE(row->childItems().back(), arrow);
        }
    }

    void iconStyles_data() {
        QTest::addColumn<QString>("type");
        QTest::addColumn<QString>("style");
        QTest::addColumn<bool>("iconVisible");
        QTest::addColumn<bool>("labelVisible");

        QTest::newRow("button-icon-and-text")
            << QStringLiteral("Button") << QStringLiteral("IconAndText") << true << true;
        QTest::newRow("button-icon-only")
            << QStringLiteral("Button") << QStringLiteral("IconOnly") << true << false;
        QTest::newRow("button-text-only")
            << QStringLiteral("Button") << QStringLiteral("TextOnly") << false << true;
        QTest::newRow("tab-button-icon-and-text")
            << QStringLiteral("TabButton") << QStringLiteral("IconAndText") << true << true;
        QTest::newRow("tab-button-icon-only")
            << QStringLiteral("TabButton") << QStringLiteral("IconOnly") << true << false;
        QTest::newRow("tab-button-text-only")
            << QStringLiteral("TabButton") << QStringLiteral("TextOnly") << false << true;
    }

    void iconStyles() {
        QFETCH(QString, type);
        QFETCH(QString, style);
        QFETCH(bool, iconVisible);
        QFETCH(bool, labelVisible);

        const QString text   = QStringLiteral("Button label");
        const auto    source = QStringLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            MD.%1 {
                text: "%2"
                icon.name: MD.Token.icon.add
                iconStyle: MD.Enum.%3
            }
        )")
                                   .arg(type, text, style)
                                   .toUtf8();

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/button-icon-style.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* control = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(control);
        control->setParentItem(m_window.contentItem());
        settle(control);

        auto* content = qvariant_cast<QQuickItem*>(control->property("contentItem"));
        QVERIFY(content);
        auto* row = layoutRow(content);
        QVERIFY(row);

        auto* label = itemWithText(row, text);
        QVERIFY(label);
        QCOMPARE(label->isVisible(), labelVisible);

        QQuickItem* icon = nullptr;
        for (auto* child : row->childItems()) {
            if (! child->property("name").toString().isEmpty()) {
                icon = child;
                break;
            }
        }
        QVERIFY(icon);
        QCOMPARE(icon->isVisible(), iconVisible);
    }

    void expandedCenteredControlsKeepNaturalContent_data() {
        QTest::addColumn<QString>("type");

        QTest::newRow("button") << QStringLiteral("Button");
        QTest::newRow("segmented-button") << QStringLiteral("SegmentedButton");
        QTest::newRow("tab-button") << QStringLiteral("TabButton");
    }

    void expandedCenteredControlsKeepNaturalContent() {
        QFETCH(QString, type);

        const QString text   = QStringLiteral("Button label");
        const auto    source = QStringLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            MD.%1 {
                width: 300
                text: "%2"
                icon.name: MD.Token.icon.add
            }
        )")
                                   .arg(type, text)
                                   .toUtf8();

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/expanded-button.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* control = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(control);
        control->setParentItem(m_window.contentItem());
        settle(control);

        auto* content = qvariant_cast<QQuickItem*>(control->property("contentItem"));
        QVERIFY(content);
        QVERIFY(qobject_cast<qml_material::Box*>(content));
        auto* row = layoutRow(content);
        QVERIFY(row);

        QCOMPARE(row->width(), row->implicitWidth());
        QCOMPARE(row->x() + row->width() / 2, content->width() / 2);
        auto* label = itemWithText(row, text);
        QVERIFY(label);
        QVERIFY(! label->property("truncated").toBool());
    }

    void menuItemBusyReplacesIcon() {
        const auto source = QByteArrayLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            MD.MenuItem {
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

        QQuickItem* icon   = nullptr;
        QQuickItem* loader = nullptr;
        for (auto* child : leading->childItems()) {
            if (! child->property("name").toString().isEmpty()) {
                icon = child;
            }
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
        QVERIFY(QMetaObject::invokeMethod(
            menu, "itemAt", Q_RETURN_ARG(QQuickItem*, delegate), Q_ARG(int, 0)));
        QVERIFY(delegate);
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
                    itemDelegate: MD.MenuItem { objectName: "actionItem" }
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

    void embedChipIconsRecoverAfterAncestorVisibilityChange() {
        const auto source = QByteArrayLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            Item {
                width: 300
                height: 100
                property string leadingIconName: MD.Token.icon.description
                property string trailingIconName: MD.Token.icon.close

                Item {
                    objectName: "page"

                    MD.EmbedChip {
                        objectName: "chip"
                        text: "Embed"
                        icon.name: parent.parent.leadingIconName
                        trailingIconName: parent.parent.trailingIconName
                    }
                }
            }
        )");

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/embed-chip-visibility.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(root);
        root->setParentItem(m_window.contentItem());

        auto* page = root->findChild<QQuickItem*>(QStringLiteral("page"));
        auto* chip = root->findChild<QQuickItem*>(QStringLiteral("chip"));
        QVERIFY(page);
        QVERIFY(chip);
        settle(chip);

        auto* row = layoutRow(qvariant_cast<QQuickItem*>(chip->property("contentItem")));
        QVERIFY(row);
        QVERIFY(row->childItems().size() >= 3);
        auto* leadingLoader  = row->childItems().front();
        auto* trailingLoader = row->childItems().back();
        auto* leadingIcon    = qvariant_cast<QQuickItem*>(leadingLoader->property("item"));
        auto* trailingIcon   = qvariant_cast<QQuickItem*>(trailingLoader->property("item"));
        QVERIFY(leadingIcon);
        QVERIFY(trailingIcon);
        QVERIFY(leadingLoader->isVisible());
        QVERIFY(trailingLoader->isVisible());
        QVERIFY(leadingIcon->isVisible());
        QVERIFY(trailingIcon->isVisible());
        const qreal full_width    = row->implicitWidth();
        const auto  leading_name  = root->property("leadingIconName");
        const auto  trailing_name = root->property("trailingIconName");

        page->setVisible(false);
        QCoreApplication::processEvents();
        QVERIFY(! leadingLoader->isVisible());
        QVERIFY(! trailingLoader->isVisible());
        QVERIFY(! leadingIcon->isVisible());
        QVERIFY(! trailingIcon->isVisible());

        page->setVisible(true);
        settle(chip);
        QVERIFY(leadingLoader->isVisible());
        QVERIFY(trailingLoader->isVisible());
        QVERIFY(leadingIcon->isVisible());
        QVERIFY(trailingIcon->isVisible());
        QCOMPARE(row->implicitWidth(), full_width);

        page->setVisible(false);
        QCoreApplication::processEvents();
        QVERIFY(root->setProperty("leadingIconName", QString()));
        QVERIFY(root->setProperty("trailingIconName", QString()));
        page->setVisible(true);
        settle(chip);
        QVERIFY(leadingLoader->isVisible());
        QVERIFY(trailingLoader->isVisible());
        QVERIFY(! leadingIcon->isVisible());
        QVERIFY(! trailingIcon->isVisible());
        QCOMPARE(row->implicitWidth(), row->childItems().at(1)->implicitWidth());

        QVERIFY(root->setProperty("leadingIconName", leading_name));
        QVERIFY(root->setProperty("trailingIconName", trailing_name));
        settle(chip);
        QCOMPARE(row->implicitWidth(), full_width);
    }

    void embedChipCustomVisibilityControlsSpacing() {
        const auto source = QByteArrayLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            Item {
                id: testRoot
                width: 300
                height: 100
                property bool trailingVisible: true

                Component {
                    id: customTrailing
                    Item {
                        implicitWidth: 24
                        implicitHeight: 18
                        visible: testRoot.trailingVisible
                    }
                }

                MD.EmbedChip {
                    objectName: "chip"
                    text: "Embed"
                    trailing: customTrailing
                }
            }
        )");

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/embed-chip-custom.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(root);
        root->setParentItem(m_window.contentItem());
        auto* chip = root->findChild<QQuickItem*>(QStringLiteral("chip"));
        QVERIFY(chip);
        settle(chip);

        auto* row = layoutRow(qvariant_cast<QQuickItem*>(chip->property("contentItem")));
        QVERIFY(row);
        auto* trailing_loader = row->childItems().back();
        auto* trailing_item   = qvariant_cast<QQuickItem*>(trailing_loader->property("item"));
        QVERIFY(trailing_item);
        const qreal visible_width = row->implicitWidth();

        QVERIFY(root->setProperty("trailingVisible", false));
        settle(chip);
        QVERIFY(! trailing_item->isVisible());
        QCOMPARE(row->implicitWidth(), visible_width - trailing_loader->implicitWidth() - 4.0);

        QVERIFY(root->setProperty("trailingVisible", true));
        settle(chip);
        QVERIFY(trailing_item->isVisible());
        QCOMPARE(row->implicitWidth(), visible_width);
    }

    void textFieldTypography_data() {
        QTest::addColumn<QString>("type");

        QTest::newRow("filled") << QStringLiteral("TextFieldFilled");
        QTest::newRow("outlined") << QStringLiteral("TextFieldOutlined");
    }

    void textFieldTypography() {
        QFETCH(QString, type);

        const auto source = QStringLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            MD.TextField {
                type: MD.Enum.%1
                text: "Input"
                placeholderText: "Label"
                property real expectedSize: MD.Token.typescale.body_large.size
                property real titleSize: MD.Token.typescale.title_large.size
            }
        )")
                                .arg(type)
                                .toUtf8();

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/text-field-typography.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* textField = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(textField);
        textField->setParentItem(m_window.contentItem());
        settle(textField);

        const int  expectedSize = qRound(textField->property("expectedSize").toReal());
        const int  titleSize    = qRound(textField->property("titleSize").toReal());
        const auto inputFont    = qvariant_cast<QFont>(textField->property("font"));
        QCOMPARE(inputFont.pixelSize(), expectedSize);
        QVERIFY(inputFont.pixelSize() < titleSize);

        auto* placeholder = itemWithText(textField, QStringLiteral("Label"));
        QVERIFY(placeholder);
        const auto placeholderFont = qvariant_cast<QFont>(placeholder->property("font"));
        QCOMPARE(placeholderFont.pixelSize(), expectedSize);
    }

    void snakeBarActionElides() {
        const QString text   = QStringLiteral("A very long snackbar action label");
        const auto    source = QStringLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            MD.SnakeBar {
                width: 180
                text: ""
                action: MD.Action { text: "%1" }
            }
        )")
                                   .arg(text)
                                   .toUtf8();

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/snackbar-action.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* snakeBar = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(snakeBar);
        snakeBar->setParentItem(m_window.contentItem());
        settle(snakeBar);

        auto* actionControl = qvariant_cast<QQuickItem*>(snakeBar->property("actionControl"));
        QVERIFY(actionControl);
        actionControl->setParentItem(m_window.contentItem());
        actionControl->setWidth(80);
        settle(actionControl);
        auto* content = qvariant_cast<QQuickItem*>(actionControl->property("contentItem"));
        QVERIFY(content);
        QVERIFY(qobject_cast<qml_material::Box*>(content));
        auto* label = itemWithText(content, text);
        QVERIFY(label);
        QVERIFY(label->implicitWidth() > label->width());
        QVERIFY(label->property("truncated").toBool());
    }

    void snakeBarStandardGeometry_data() {
        QTest::addColumn<QString>("sourceText");
        QTest::addColumn<QString>("text");
        QTest::addColumn<bool>("hasAction");
        QTest::addColumn<bool>("actionOnNewLine");
        QTest::addColumn<qreal>("expectedHeight");

        QTest::newRow("single-line")
            << QStringLiteral("Single-line snackbar") << QStringLiteral("Single-line snackbar")
            << false << false << 48.0;
        QTest::newRow("single-line-action")
            << QStringLiteral("Single-line snackbar with action")
            << QStringLiteral("Single-line snackbar with action") << true << false << 48.0;
        QTest::newRow("two-line") << QStringLiteral("Two-line snackbar\\nwithout action")
                                  << QStringLiteral("Two-line snackbar\nwithout action") << false
                                  << false << 68.0;
        QTest::newRow("two-line-action")
            << QStringLiteral("Two-line snackbar\\nwith action")
            << QStringLiteral("Two-line snackbar\nwith action") << true << false << 68.0;
        QTest::newRow("two-line-longer-action")
            << QStringLiteral("Two-line snackbar\\nwith longer action")
            << QStringLiteral("Two-line snackbar\nwith longer action") << true << true << 112.0;
    }

    void snakeBarStandardGeometry() {
        QFETCH(QString, sourceText);
        QFETCH(QString, text);
        QFETCH(bool, hasAction);
        QFETCH(bool, actionOnNewLine);
        QFETCH(qreal, expectedHeight);

        const auto source =
            QStringLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            MD.SnakeBar {
                width: 344
                text: "%1"
                actionOnNewLine: %2
                action: %3
            }
        )")
                .arg(sourceText,
                     actionOnNewLine ? QStringLiteral("true") : QStringLiteral("false"),
                     hasAction ? QStringLiteral("MD.Action { text: \"Action\" }")
                               : QStringLiteral("null"))
                .toUtf8();

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/snackbar-standard.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* snakeBar = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(snakeBar);
        snakeBar->setParentItem(m_window.contentItem());
        settle(snakeBar, 12);

        QCOMPARE(snakeBar->implicitHeight(), expectedHeight);
        auto* label = itemWithText(snakeBar, text);
        QVERIFY(label);

        if (! hasAction) {
            return;
        }

        auto* actionControl = qvariant_cast<QQuickItem*>(snakeBar->property("actionControl"));
        QVERIFY(actionControl);
        const QRectF labelRect(label->mapToItem(snakeBar, QPointF {}),
                               QSizeF(label->width(), label->height()));
        const QRectF actionRect(actionControl->mapToItem(snakeBar, QPointF {}),
                                QSizeF(actionControl->width(), actionControl->height()));
        if (actionOnNewLine) {
            QVERIFY(actionRect.top() >= labelRect.bottom());
        } else {
            QVERIFY(actionRect.top() < labelRect.bottom());
            QVERIFY(actionRect.bottom() > labelRect.top());
        }
    }

    void snakeViewLongTextWrapsWithoutClipping() {
        const QString text = QStringLiteral(
            "Import completed with additional details. 128 records were processed, 124 were "
            "added, three duplicates were skipped, and one record needs manual review before it "
            "can be published.");
        const auto source = QStringLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            MD.SnakeView {
                width: 500
                height: 300
                property MD.Action inlineAction: MD.Action { text: "Action" }
                Component.onCompleted: show("%1", 60000, 0, inlineAction, 420)
            }
        )")
                                .arg(text)
                                .toUtf8();

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/snackbar-details.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* snakeView = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(snakeView);
        snakeView->setParentItem(m_window.contentItem());
        settle(snakeView, 12);

        auto* label = itemWithText(snakeView, text);
        QVERIFY(label);
        QQuickItem* snakeBar = label;
        while (snakeBar && ! snakeBar->property("actionControl").isValid()) {
            snakeBar = snakeBar->parentItem();
        }
        QVERIFY(snakeBar);
        QCOMPARE(snakeBar->width(), 420.0);
        QCOMPARE(snakeBar->property("actionOnNewLine").toBool(), false);
        QCOMPARE(snakeBar->property("showClose").toBool(), false);
        QVERIFY(label->property("lineCount").toInt() > 2);

        auto* actionControl = qvariant_cast<QQuickItem*>(snakeBar->property("actionControl"));
        QVERIFY(actionControl);
        QVERIFY(actionControl->isVisible());

        const QRectF labelRect(label->mapToItem(snakeBar, QPointF {}),
                               QSizeF(label->width(), label->height()));
        const QRectF actionRect(actionControl->mapToItem(snakeBar, QPointF {}),
                                QSizeF(actionControl->width(), actionControl->height()));
        QVERIFY(labelRect.top() >= 13.0);
        QVERIFY(labelRect.bottom() <= snakeBar->height() - 13.0);
        QVERIFY(snakeBar->height() >= label->implicitHeight() + 28.0);
        QVERIFY(actionRect.top() < labelRect.bottom());
        QVERIFY(actionRect.bottom() > labelRect.top());

        auto* delegate = snakeBar->parentItem();
        QVERIFY(delegate);
        QTRY_COMPARE_WITH_TIMEOUT(delegate->height(), snakeBar->height(), 500);
        QVERIFY(! delegate->clip());
    }

    void snakeViewPropagatesActionPlacement() {
        const QString text   = QStringLiteral("Two-line snackbar\nwith longer action");
        const auto    source = QStringLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            MD.SnakeView {
                width: 500
                height: 300
                property MD.Action longerAction: MD.Action { text: "Longer action" }
                Component.onCompleted: show("Two-line snackbar\nwith longer action",
                                            60000, 0, longerAction, 344, true)
            }
        )")
                                   .toUtf8();

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/snackbar-action-placement.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* snakeView = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(snakeView);
        snakeView->setParentItem(m_window.contentItem());
        settle(snakeView, 12);

        auto* label = itemWithText(snakeView, text);
        QVERIFY(label);
        QQuickItem* snakeBar = label;
        while (snakeBar && ! snakeBar->property("actionControl").isValid()) {
            snakeBar = snakeBar->parentItem();
        }
        QVERIFY(snakeBar);
        QCOMPARE(snakeBar->property("actionOnNewLine").toBool(), true);
        QCOMPARE(snakeBar->implicitHeight(), 112.0);
    }

private:
    QQmlEngine   m_engine;
    QQuickWindow m_window;
};

QTEST_MAIN(ControlLayoutTest)

#include "control_layout.moc"
