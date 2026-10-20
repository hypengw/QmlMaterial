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

#include "qml_material/control/tool_tip.hpp"
#include "qml_material/core/enum.hpp"
#include "qml_material/control/tab_bar.hpp"
#include "qml_material/control/popup.hpp"
#include "qml_material/control/panel.hpp"
#include "qml_material/control/dialog.hpp"
#include "qml_material/control/color_picker_button.hpp"
#include "qml_material/style/material_state.hpp"
#include "qml_material/style/list_item_state.hpp"
#include "qml_material/style/text_field_state.hpp"
#include "qml_material/style/slider_state.hpp"
#include "qml_material/style/drag_handle_state.hpp"
#include "qml_material/control/text_field.hpp"
#include "qml_material/control/slider.hpp"
#include "qml_material/control/scroll_bar.hpp"
#include "qml_material/control/item_delegate.hpp"
#include "qml_material/style/theme.hpp"
#include "qml_material/control/icon_spec.hpp"
#include "qml_material/control/button_control.hpp"
#include "qml_material/control/selection_button.hpp"
#include "qml_material/layout/layout_container.hpp"
#include "qml_material/view/lazy_list.hpp"
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

QQuickItem* itemWithText(QQuickItem* root, const QString& text) {
    for (auto* child : root->childItems()) {
        if (auto* match = itemWithText(child, text)) {
            return match;
        }
    }
    return root->property("text").toString() == text ? root : nullptr;
}

QQuickItem* itemWithImplicitSizeAndColor(QQuickItem* root, const QSizeF& size,
                                         const QColor& color) {
    if (qFuzzyCompare(root->implicitWidth(), size.width()) &&
        qFuzzyCompare(root->implicitHeight(), size.height()) &&
        root->property("color").value<QColor>() == color) {
        return root;
    }
    for (auto* child : root->childItems()) {
        if (auto* match = itemWithImplicitSizeAndColor(child, size, color)) {
            return match;
        }
    }
    return nullptr;
}

QQuickItem* itemWithLeadingItem(QQuickItem* root, QQuickItem* leadingItem) {
    if (qvariant_cast<QQuickItem*>(root->property("leadingItem")) == leadingItem) {
        return root;
    }
    for (auto* child : root->childItems()) {
        if (auto* match = itemWithLeadingItem(child, leadingItem)) {
            return match;
        }
    }
    return nullptr;
}

QQuickItem* itemWithIcon(QQuickItem* root) {
    if (! root->property("name").toString().isEmpty()) {
        return root;
    }
    for (auto* child : root->childItems()) {
        if (auto* match = itemWithIcon(child)) {
            return match;
        }
    }
    return nullptr;
}

qml_material::ToolTipAttached* attachedToolTip(QObject* target) {
    return static_cast<qml_material::ToolTipAttached*>(
        qmlAttachedPropertiesObject<qml_material::ToolTip>(target, false));
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

#include "qml_material/control/application_window.hpp"

class ControlLayoutTest : public QObject {
    Q_OBJECT

private slots:
    void scrollViewportAttachment_data() {
        QTest::addColumn<QByteArray>("type");
        QTest::newRow("qt") << QByteArray("Flickable");
        QTest::newRow("owned") << QByteArray("MD.Scrollable");
    }

    void scrollViewportAttachment() {
        QFETCH(QByteArray, type);
        QQmlComponent component(&m_engine);
        component.setData("import QtQuick\nimport Qcm.Material as MD\n" + type + R"(
            {
                width: 200; height: 100
                contentWidth: 800; contentHeight: 1000
                MD.ScrollBar.vertical: MD.ScrollBar { objectName: "vertical" }
                MD.ScrollIndicator.horizontal: MD.ScrollIndicator { objectName: "horizontal" }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        auto* bar       = root->findChild<qml_material::ScrollBar*>("vertical");
        auto* indicator = root->findChild<QQuickItem*>("horizontal");
        QVERIFY(bar);
        QVERIFY(indicator);
        QCOMPARE(bar->property("size").toReal(), 0.1);
        QCOMPARE(indicator->property("size").toReal(), 0.25);
        root->setProperty("contentY", 300);
        QCOMPARE(bar->property("position").toReal(), 0.3);
        bar->setPosition(.5);
        QCOMPARE(root->property("contentY").toReal(), 300.0);
        bar->requestPosition(0.6);
        QCOMPARE(root->property("contentY").toReal(), 600.0);
        root->setProperty("contentX", 200);
        QCOMPARE(indicator->property("position").toReal(), 0.25);
        root->setProperty("height", 200);
        QCOMPARE(bar->height(), 200.0);
        QCOMPARE(bar->property("size").toReal(), 0.2);
        root->setProperty("topMargin", 20);
        root->setProperty("bottomMargin", 30);
        QCOMPARE(bar->property("size").toReal(), 200.0 / 1050.0);
        bar->requestPosition(0.4);
        QCOMPARE(root->property("contentY").toReal(), 400.0);
    }

    void initTestCase() {
        m_engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        m_window.setGeometry(0, 0, 800, 600);
        m_window.create();
    }

    void inputChipSelectionAndEdit() {
        QQuickWindow window;
        window.resize(400, 200);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        window.requestActivate();
        QTRY_VERIFY(window.isActive());
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.InputChip {
                id: chip
                text: "Rainbow"
                property int requests: 0
                onSelectionRequested: ++requests
                editDelegate: TextInput {}
            }
        )",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        auto*                    chip = qobject_cast<qml_material::SelectionButton*>(object.get());
        QVERIFY(chip);
        chip->setParentItem(window.contentItem());
        settle(chip);
        QTest::mouseClick(
            &window, Qt::LeftButton, Qt::NoModifier, QPoint(chip->width() / 2, chip->height() / 2));
        QCOMPARE(chip->property("requests").toInt(), 1);
        QVERIFY(! chip->hasActiveFocus());
        chip->click();
        QCOMPARE(chip->property("requests").toInt(), 2);
        chip->forceActiveFocus(Qt::TabFocusReason);
        QTest::keyClick(&window, Qt::Key_Space);
        QCOMPARE(chip->property("requests").toInt(), 3);
        chip->setProperty("edit", true);
        settle(chip);
        QVERIFY(chip->contentItem()->hasActiveFocus());
    }

    void tabBarLayoutModes() {
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.TabBar {
                width: 500
                spacing: 0
                edgePadding: 20
                MD.TabButton { implicitWidth: 120; text: "First" }
                MD.TabButton { implicitWidth: 160; text: "Second" }
            }
        )",
                          QUrl("qrc:/tests/tab-modes.qml"));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* bar = qobject_cast<qml_material::TabBar*>(object.get());
        QVERIFY(bar);
        bar->setParentItem(m_window.contentItem());
        settle(bar);
        auto* first  = bar->itemAt(0);
        auto* second = bar->itemAt(1);
        QVERIFY(first && second);
        QCOMPARE(first->width(), 250);
        QCOMPARE(second->width(), 250);
        bar->setMode(qml_material::TabBar::Scrollable);
        settle(bar);
        QCOMPARE(first->width(), 120);
        QCOMPARE(second->width(), 160);
        QCOMPARE(first->x(), 20);
        QCOMPARE(bar->contentHost()->width(), 320);
        bar->setMode(qml_material::TabBar::Auto);
        settle(bar);
        QCOMPARE(first->width(), 160);
        QCOMPARE(second->width(), 160);
        QCOMPARE(first->x(), 90);
        QCOMPARE(bar->contentHost()->width(), 500);
        bar->setWidth(250);
        settle(bar);
        QCOMPARE(first->width(), 120);
        QCOMPARE(second->width(), 160);
        QCOMPARE(first->x(), 20);
        QCOMPARE(bar->contentHost()->width(), 320);
        bar->setLayoutDirection(Qt::RightToLeft);
        settle(bar);
        QCOMPARE(first->x(), 180);
        QCOMPARE(second->x(), 20);
        first->setFillWidth(false);
        first->setWidth(70);
        bar->setMode(qml_material::TabBar::Fixed);
        settle(bar);
        QCOMPARE(first->width(), 70);
        QCOMPARE(second->width(), 180);
        bar->setMode(qml_material::TabBar::Scrollable);
        bar->setMinimumTabWidth(180);
        settle(bar);
        QCOMPARE(first->width(), 70);
        QCOMPARE(second->width(), 180);
        QCOMPARE(bar->contentHost()->width(), 290);
    }

    void tabButtonStackedContent() {
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.TabButton {
                width: 120
                text: "Label"
                icon.name: MD.Token.icon.add
            }
        )",
                          QUrl("qrc:/tests/tab-stacked.qml"));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* button = qobject_cast<qml_material::TabButton*>(object.get());
        QVERIFY(button);
        button->setParentItem(m_window.contentItem());
        settle(button);
        QCOMPARE(button->implicitHeight(), 48);
        const qreal inlineWidth = button->implicitWidth();
        button->setProperty("inlineLabel", false);
        settle(button);
        QCOMPARE(button->implicitHeight(), 72);
        QVERIFY(button->implicitWidth() < inlineWidth);
        auto* label = itemWithText(button->contentItem(), QStringLiteral("Label"));
        QVERIFY(label);
        QVERIFY(qobject_cast<qml_material::Column*>(label->parentItem()));
        QVERIFY(label->width() > 0);
        button->setProperty("inlineLabel", true);
        settle(button);
        QCOMPARE(button->implicitHeight(), 48);
        QVERIFY(qobject_cast<qml_material::Row*>(label->parentItem()));
        button->setProperty("inlineLabel", false);
        button->setProperty("iconStyle", int(qml_material::Enum::IconLabelStyle::TextOnly));
        settle(button);
        QCOMPARE(button->implicitHeight(), 48);
    }

    void tabBarMotionGeometry_data() {
        QTest::addColumn<bool>("secondary");
        QTest::addColumn<bool>("mirrored");
        QTest::newRow("primary") << false << false;
        QTest::newRow("secondary") << true << false;
        QTest::newRow("primary-rtl") << false << true;
        QTest::newRow("secondary-rtl") << true << true;
    }

    void tabBarMotionGeometry() {
        QFETCH(bool, secondary);
        QFETCH(bool, mirrored);
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.TabBar {
                width: 160
                currentIndex: 1
                MD.TabButton { width: 80; fillWidth: false; text: "A" }
                MD.TabButton { width: 120; fillWidth: false; text: "Second" }
                MD.TabButton { width: 100; fillWidth: false; text: "Third" }
            }
        )",
                          QUrl("qrc:/tests/tab-motion.qml"));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* bar = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(bar);
        if (secondary) bar->setProperty("type", int(qml_material::Enum::TabType::SecondaryTab));
        QVERIFY(bar->setProperty("layoutDirection", mirrored ? Qt::RightToLeft : Qt::LeftToRight));
        bar->setParentItem(m_window.contentItem());
        settle(bar);
        QCOMPARE(bar->property("mirrored").toBool(), mirrored);
        auto* view = bar->property("contentItem").value<QQuickItem*>();
        QVERIFY(view);
        auto* scrollingContent = view->property("contentItem").value<QQuickItem*>();
        QVERIFY(scrollingContent);
        QQuickItem* indicator = nullptr;
        for (auto* child : scrollingContent->childItems())
            if (child->z() == 2) indicator = child;
        QVERIFY(indicator);
        QObject* scrollAnimation = nullptr;
        QVERIFY(bar->clip());
        QVERIFY(! indicator->clip());
        QCOMPARE(indicator->property("topLeftRadius").toReal(), secondary ? 0.0 : 3.0);
        QCOMPARE(indicator->property("bottomLeftRadius").toReal(), 0.0);
        bar->setProperty("position", int(qml_material::TabBar::Footer));
        QCOMPARE(indicator->y(), 0.0);
        QCOMPARE(indicator->property("topLeftRadius").toReal(), 0.0);
        QCOMPARE(indicator->property("bottomLeftRadius").toReal(), secondary ? 0.0 : 3.0);
        bar->setProperty("position", int(qml_material::TabBar::Header));
        for (auto* child : view->findChildren<QObject*>())
            if (child->property("property").toString() == QStringLiteral("contentX"))
                scrollAnimation = child;
        QVERIFY(scrollAnimation);
        QVERIFY(! scrollAnimation->property("running").toBool());
        QVERIFY(view->clip());
        QCOMPARE(QQmlProperty(bar, "layer.enabled").read().toBool(), false);
        bar->setProperty("radius", 12);
        QCOMPARE(QQmlProperty(bar, "layer.enabled").read().toBool(), false);
        bar->setProperty("radius", 0);
        bar->setProperty("currentIndex", 0);
        settle(bar);
        scrollAnimation->setProperty("running", true);
        QVERIFY(QMetaObject::invokeMethod(view, "movementStarted"));
        QVERIFY(! scrollAnimation->property("running").toBool());
        bar->setProperty("currentIndex", 2);
        settle(bar);
        for (int index : { 1, 2, 0, 1 }) {
            bar->setProperty("currentIndex", index);
            settle(bar);
            auto* item = bar->property("currentItem").value<QQuickItem*>();
            QVERIFY(item);
            const qreal expectedWidth =
                secondary ? item->width()
                          : std::min(item->width(),
                                     std::max(qreal(24),
                                              item->property("implicitContentWidth").toReal()));
            QTRY_VERIFY(qAbs(indicator->width() - expectedWidth) < 0.01);
            QTRY_VERIFY(qAbs(indicator->x() - (item->x() + (item->width() - expectedWidth) / 2)) <
                        0.01);
            const qreal scrollX = std::clamp(
                item->x() + item->width() / 2 - view->width() / 2,
                qreal(0),
                std::max(qreal(0), view->property("contentWidth").toReal() - view->width()));
            QTRY_VERIFY(qAbs(view->property("contentX").toReal() - scrollX) < 0.01);
        }
        bar->setWidth(400);
        settle(bar);
        QTRY_VERIFY(qAbs(view->property("contentX").toReal()) < 0.01);
        bar->setProperty("currentIndex", -1);
        settle(bar);
        QVERIFY(! indicator->isVisible());
        bar->setProperty("currentIndex", 2);
        settle(bar);
        QVERIFY(indicator->isVisible());
    }

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
                                << QStringLiteral("selected: true; icon.name: MD.Token.icon.close");
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

    void railItemIconOnly() {
        const auto source = QByteArrayLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            Item {
                width: 360
                height: 160
                property int iconAndTextStyle: MD.Enum.IconAndText
                property int iconOnlyStyle: MD.Enum.IconOnly
                property MD.Action railAction: MD.Action {
                    text: "Settings"
                    tooltip: "Configure application"
                    icon.name: MD.Token.icon.settings
                }

                MD.RailItem {
                    objectName: "collapsed"
                    width: 96
                    text: parent.railAction.text
                    icon.actionIcon: parent.railAction.icon
                    MD.ToolTip.text: parent.railAction.tooltip || text
                    iconStyle: MD.Enum.IconOnly
                }
                MD.RailItem {
                    objectName: "expanded"
                    x: 120
                    expand: true
                    text: parent.railAction.text
                    icon.actionIcon: parent.railAction.icon
                    MD.ToolTip.text: parent.railAction.tooltip || text
                    iconStyle: MD.Enum.IconOnly
                }
            }
        )");

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/rail-item-icon-only.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(root);
        root->setParentItem(m_window.contentItem());
        settle(root);

        auto* collapsed = root->findChild<QQuickItem*>(QStringLiteral("collapsed"));
        auto* expanded  = root->findChild<QQuickItem*>(QStringLiteral("expanded"));
        auto* action    = qvariant_cast<QObject*>(root->property("railAction"));
        QVERIFY(collapsed);
        QVERIFY(expanded);
        QVERIFY(action);

        auto verifyIconOnly = [](QQuickItem* item, qreal expectedHeight, qreal expectedIconX) {
            auto* content = qvariant_cast<QQuickItem*>(item->property("contentItem"));
            QVERIFY(content);
            QCOMPARE(content->implicitHeight(), expectedHeight);

            auto* label = itemWithText(content, QStringLiteral("Settings"));
            auto* icon  = itemWithIcon(content);
            QVERIFY(label);
            QVERIFY(icon);
            QVERIFY(! label->isVisible());
            QVERIFY(icon->isVisible());
            QCOMPARE(icon->mapToItem(content, QPointF()).x(), expectedIconX);
        };

        verifyIconOnly(collapsed, 32.0, 36.0);
        verifyIconOnly(expanded, 56.0, 32.0);
        QCOMPARE(expanded->implicitWidth(), 88.0);
        QCOMPARE(expanded->width(), 88.0);

        auto* expandedBackground = qvariant_cast<QQuickItem*>(expanded->property("background"));
        QVERIFY(expandedBackground);
        QCOMPARE(expandedBackground->childItems().size(), 1);
        auto* expandedIndicator = expandedBackground->childItems().front();
        QCOMPARE(expandedIndicator->x(), 16.0);
        QCOMPARE(expandedIndicator->width(), 56.0);
        QCOMPARE(expandedIndicator->height(), 56.0);

        auto* collapsedToolTip = attachedToolTip(collapsed);
        auto* expandedToolTip  = attachedToolTip(expanded);
        QVERIFY(collapsedToolTip);
        QVERIFY(expandedToolTip);
        QCOMPARE(collapsedToolTip->text(), QStringLiteral("Configure application"));
        QCOMPARE(expandedToolTip->text(), QStringLiteral("Configure application"));
        QCOMPARE(collapsedToolTip->toolTip(), expandedToolTip->toolTip());

        collapsedToolTip->show(collapsedToolTip->text(), 0);
        QTRY_VERIFY(collapsedToolTip->visible());
        QCOMPARE(qvariant_cast<QQuickItem*>(collapsedToolTip->toolTip()->property("parent")),
                 collapsed);
        collapsedToolTip->hide();
        QTRY_VERIFY(! collapsedToolTip->visible());

        QVERIFY(action->setProperty("tooltip", QStringLiteral("Configure")));
        settle(root);
        QCOMPARE(collapsedToolTip->text(), QStringLiteral("Configure"));

        QVERIFY(action->setProperty("tooltip", QString()));
        settle(root);
        QCOMPARE(collapsedToolTip->text(), QStringLiteral("Settings"));

        QVERIFY(collapsed->setProperty("iconStyle", root->property("iconAndTextStyle")));
        auto* collapsedContent = qvariant_cast<QQuickItem*>(collapsed->property("contentItem"));
        auto* collapsedLabel   = itemWithText(collapsedContent, QStringLiteral("Settings"));
        QVERIFY(collapsedLabel);
        QTRY_VERIFY(collapsedLabel->isVisible());
        QTRY_VERIFY(collapsedContent->implicitHeight() > 32.0);

        QVERIFY(collapsed->setProperty("iconStyle", root->property("iconOnlyStyle")));
        QTRY_VERIFY(! collapsedLabel->isVisible());
        QTRY_COMPARE(collapsedContent->implicitHeight(), 32.0);
    }

    void railItemExpansionLabels_data() {
        QTest::addColumn<QString>("text");
        for (const auto& text : { QStringLiteral("typography"), QStringLiteral("about") })
            QTest::newRow(qPrintable(text)) << text;
    }

    void railItemExpansionLabels() {
        QFETCH(QString, text);
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.RailItem {
                icon.name: MD.Token.icon.info
                expand: true
                collapsedIconStyle: MD.Enum.IconOnly
                expansionProgress: 0
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(
            component.createWithInitialProperties({ { "text", text } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto item = qobject_cast<QQuickItem*>(object.get());
        item->setParentItem(m_window.contentItem());
        auto content = qvariant_cast<QQuickItem*>(item->property("contentItem"));
        QVERIFY(content);
        for (const auto& sample : QList<QPair<qreal, qreal>> {
                 { .25, 127 }, { .5, 158 }, { .75, 189 }, { 1, 220 }, { 1, 96 } }) {
            QVERIFY(item->setProperty("expansionProgress", sample.first));
            item->setWidth(sample.second);
            settle(item);
            int visibleLabels = 0;
            for (auto label : content->childItems()) {
                if (! label->isVisible() || label->property("text").toString() != text) continue;
                ++visibleLabels;
                QVERIFY(label->property("lineCount").toInt() <= 1);
                QVERIFY(label->implicitHeight() <= item->height());
                QVERIFY(label->y() >= 0);
                QVERIFY(label->y() + label->height() <= item->height() + .1);
            }
            QCOMPARE(visibleLabels, 1);
        }
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

    void textLineHeightAlignment_data() {
        QTest::addColumn<QString>("type");
        QTest::addColumn<QString>("family");
        QTest::addColumn<bool>("multiline");
        QStringList families { QGuiApplication::font().family(), QStringLiteral("serif") };
        const auto  installed = QFontDatabase::families();
        for (const auto& family : { QStringLiteral("FangSong"),
                                    QStringLiteral("SimSun"),
                                    QStringLiteral("Microsoft YaHei"),
                                    QStringLiteral("LXGW WenKai") }) {
            if (installed.contains(family) && ! families.contains(family)) families.append(family);
        }
        for (const auto& type : { QStringLiteral("Text"), QStringLiteral("Label") }) {
            for (const auto& family : families) {
                for (const bool multiline : { false, true }) {
                    const auto name = type + '-' + family + (multiline ? "-multiline" : "-single");
                    QTest::newRow(qPrintable(name)) << type << family << multiline;
                }
            }
        }
    }

    void textLineHeightAlignment() {
        QFETCH(QString, type);
        QFETCH(QString, family);
        QFETCH(bool, multiline);
        const auto    source = QStringLiteral(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                width: 360; height: 160
                property string family
                property string sample
                property bool expanded: false
                MD.%1 {
                    id: styled
                    objectName: "styled"
                    width: 180
                    height: parent.expanded ? 120 : implicitHeight
                    text: parent.sample
                    font.family: parent.family
                    font.pixelSize: 14
                    lineHeight: 40
                    wrapMode: Text.NoWrap
                }
                Text {
                    objectName: "reference"
                    width: styled.width
                    height: styled.height
                    text: styled.text
                    font: styled.font
                    lineHeight: styled.lineHeight
                    lineHeightMode: Text.FixedHeight
                    textFormat: Text.PlainText
                    wrapMode: Text.NoWrap
                    verticalAlignment: Text.AlignVCenter
                }
            }
        )")
                                   .arg(type)
                                   .toUtf8();
        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/text-line-height.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(root);
        root->setParentItem(m_window.contentItem());
        root->setProperty("family", family);
        root->setProperty("sample",
                          multiline ? QStringLiteral("Theme\n\u4e3b\u9898")
                                    : QStringLiteral("\u4e3b\u9898 Theme"));
        auto* styled    = root->findChild<QQuickItem*>(QStringLiteral("styled"));
        auto* reference = root->findChild<QQuickItem*>(QStringLiteral("reference"));
        QVERIFY(styled);
        QVERIFY(reference);
        for (const bool expanded : { false, true }) {
            root->setProperty("expanded", expanded);
            settle(root);
            QCOMPARE(styled->property("verticalAlignment").toInt(), int(Qt::AlignVCenter));
            QCOMPARE(styled->baselineOffset(), reference->baselineOffset());
            QCOMPARE(styled->implicitWidth(), reference->implicitWidth());
            QCOMPARE(styled->implicitHeight(), reference->implicitHeight());
            QCOMPARE(styled->property("lineCount").toInt(), multiline ? 2 : 1);
        }
        const auto implicitSize = QSizeF(styled->implicitWidth(), styled->implicitHeight());
        styled->setProperty("verticalAlignment", int(Qt::AlignTop));
        reference->setProperty("verticalAlignment", int(Qt::AlignTop));
        settle(root);
        QCOMPARE(styled->baselineOffset(), reference->baselineOffset());
        QCOMPARE(QSizeF(styled->implicitWidth(), styled->implicitHeight()), implicitSize);
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
                property bool usesDefaultSize: mdState.size === MD.Enum.M
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
        QVERIFY(textField->property("usesDefaultSize").toBool());
        QCOMPARE(textField->implicitHeight(), 56.0);

        auto* placeholder = itemWithText(textField, QStringLiteral("Label"));
        QVERIFY(placeholder);
        const auto placeholderFont = qvariant_cast<QFont>(placeholder->property("font"));
        const int  restPixelSize   = placeholder->property("restPixelSize").toInt();
        QCOMPARE(restPixelSize, expectedSize);
        QCOMPARE(placeholderFont.pixelSize(),
                 qRound(restPixelSize * placeholder->property("targetScale").toReal()));
    }

    void textFieldSizes_data() {
        QTest::addColumn<QString>("type");
        QTest::addColumn<QString>("size");
        QTest::addColumn<qreal>("containerHeight");
        QTest::addColumn<qreal>("horizontalPadding");
        QTest::addColumn<qreal>("verticalPadding");
        QTest::addColumn<qreal>("iconSize");
        QTest::addColumn<qreal>("iconSpacing");
        QTest::addColumn<int>("fontSize");

        const auto addRows = [](const QString& type) {
            const auto prefix = type == QStringLiteral("TextFieldFilled") ? "filled" : "outlined";
            QTest::newRow(qPrintable(QStringLiteral("%1-xs").arg(prefix)))
                << type << QStringLiteral("XS") << 40.0 << 12.0 << 8.0 << 18.0 << 8.0 << 12;
            QTest::newRow(qPrintable(QStringLiteral("%1-s").arg(prefix)))
                << type << QStringLiteral("S") << 48.0 << 12.0 << 12.0 << 20.0 << 8.0 << 14;
            QTest::newRow(qPrintable(QStringLiteral("%1-m").arg(prefix)))
                << type << QStringLiteral("M") << 56.0 << 16.0 << 16.0 << 24.0 << 12.0 << 16;
            QTest::newRow(qPrintable(QStringLiteral("%1-l").arg(prefix)))
                << type << QStringLiteral("L") << 64.0 << 20.0 << 20.0 << 24.0 << 12.0 << 16;
            QTest::newRow(qPrintable(QStringLiteral("%1-xl").arg(prefix)))
                << type << QStringLiteral("XL") << 72.0 << 24.0 << 24.0 << 32.0 << 16.0 << 16;
        };
        addRows(QStringLiteral("TextFieldFilled"));
        addRows(QStringLiteral("TextFieldOutlined"));
    }

    void textFieldSizes() {
        QFETCH(QString, type);
        QFETCH(QString, size);
        QFETCH(qreal, containerHeight);
        QFETCH(qreal, horizontalPadding);
        QFETCH(qreal, verticalPadding);
        QFETCH(qreal, iconSize);
        QFETCH(qreal, iconSpacing);
        QFETCH(int, fontSize);

        const auto source = QStringLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            MD.TextField {
                type: MD.Enum.%1
                mdState.size: MD.Enum.%2
                text: "Input"
                placeholderText: "Label"
                leadingIcon: MD.Token.icon.search
                trailingIcon: MD.Token.icon.info
                property real resolvedHeight: mdState.containerHeight
                property real resolvedHorizontalPadding: mdState.horizontalPadding
                property real resolvedVerticalPadding: mdState.verticalPadding
                property real resolvedIconSize: mdState.iconSize
                property real resolvedSpacing: mdState.spacing
            }
        )")
                                .arg(type, size)
                                .toUtf8();

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/text-field-size.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* textField = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(textField);
        textField->setParentItem(m_window.contentItem());
        settle(textField);

        QCOMPARE(textField->property("resolvedHeight").toReal(), containerHeight);
        QCOMPARE(textField->implicitHeight(), containerHeight);
        QCOMPARE(textField->property("resolvedHorizontalPadding").toReal(), horizontalPadding);
        QCOMPARE(textField->property("resolvedVerticalPadding").toReal(), verticalPadding);
        QCOMPARE(textField->property("resolvedIconSize").toReal(), iconSize);
        QCOMPARE(textField->property("resolvedSpacing").toReal(), iconSpacing);
        QCOMPARE(textField->property("leftPadding").toReal(),
                 horizontalPadding + iconSize + iconSpacing);
        QCOMPARE(textField->property("rightPadding").toReal(),
                 horizontalPadding + iconSize + iconSpacing);
        QCOMPARE(qvariant_cast<QFont>(textField->property("font")).pixelSize(), fontSize);
    }

    void comboBoxSizes_data() {
        QTest::addColumn<QString>("sizeBinding");
        QTest::addColumn<qreal>("containerHeight");
        QTest::addColumn<qreal>("horizontalPadding");
        QTest::addColumn<qreal>("indicatorSize");
        QTest::addColumn<qreal>("indicatorSpacing");
        QTest::addColumn<int>("fontSize");
        QTest::addColumn<bool>("mediumSize");

        QTest::newRow("xs") << QStringLiteral("mdState.size: MD.Enum.XS") << 32.0 << 8.0 << 18.0
                            << 0.0 << 12 << false;
        QTest::newRow("s") << QStringLiteral("mdState.size: MD.Enum.S") << 40.0 << 10.0 << 20.0
                           << 0.0 << 14 << false;
        QTest::newRow("default-m") << QString() << 48.0 << 12.0 << 24.0 << 0.0 << 16 << true;
        QTest::newRow("l") << QStringLiteral("mdState.size: MD.Enum.L") << 56.0 << 16.0 << 24.0
                           << 0.0 << 16 << false;
        QTest::newRow("xl") << QStringLiteral("mdState.size: MD.Enum.XL") << 72.0 << 24.0 << 32.0
                            << 0.0 << 16 << false;
    }

    void comboBoxSizes() {
        QFETCH(QString, sizeBinding);
        QFETCH(qreal, containerHeight);
        QFETCH(qreal, horizontalPadding);
        QFETCH(qreal, indicatorSize);
        QFETCH(qreal, indicatorSpacing);
        QFETCH(int, fontSize);
        QFETCH(bool, mediumSize);

        const auto source = QStringLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            MD.ComboBox {
                model: ["Option"]
                %1
                property bool mediumSize: mdState.size === MD.Enum.M
                property real resolvedHeight: mdState.containerHeight
                property real resolvedHorizontalPadding: mdState.horizontalPadding
                property real resolvedIndicatorSize: mdState.indicatorSize
                property real resolvedSpacing: mdState.spacing
                property int resolvedFontSize: contentItem.font.pixelSize
            }
        )")
                                .arg(sizeBinding)
                                .toUtf8();

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/combo-box-size.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* comboBox = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(comboBox);
        comboBox->setParentItem(m_window.contentItem());
        settle(comboBox);

        QCOMPARE(comboBox->property("mediumSize").toBool(), mediumSize);
        QCOMPARE(comboBox->property("resolvedHeight").toReal(), containerHeight);
        QCOMPARE(comboBox->implicitHeight(), containerHeight);
        QCOMPARE(comboBox->property("resolvedHorizontalPadding").toReal(), horizontalPadding);
        QCOMPARE(comboBox->property("resolvedIndicatorSize").toReal(), indicatorSize);
        QCOMPARE(comboBox->property("resolvedSpacing").toReal(), indicatorSpacing);
        QCOMPARE(comboBox->property("leftPadding").toReal(), horizontalPadding);
        QCOMPARE(comboBox->property("rightPadding").toReal(),
                 horizontalPadding + indicatorSize + indicatorSpacing);
        QCOMPARE(comboBox->property("resolvedFontSize").toInt(), fontSize);
    }

    void colorPickerSwatchDoesNotOverlapHexText() {
        QQmlComponent component(&m_engine);
        component.setData(QByteArrayLiteral(R"(
            import Qcm.Material as MD

            MD.ColorPicker {
                color: "#6750A4"
                showHeader: false
                visible: false
            }
        )"),
                          QUrl(QStringLiteral("qrc:/tests/color-picker-swatch.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* picker = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(picker);
        picker->setParentItem(m_window.contentItem());
        settle(picker);

        auto* swatch =
            itemWithImplicitSizeAndColor(picker, QSizeF(20, 20), QColor(QStringLiteral("#6750A4")));
        auto* label = itemWithText(picker, QStringLiteral("#6750A4FF"));
        QVERIFY(swatch);
        QVERIFY(label);

        picker->setVisible(true);
        settle(picker);

        QVERIFY(swatch->isVisible());
        auto* chip = itemWithLeadingItem(picker, swatch);
        QVERIFY(chip);

        const QPointF swatchPosition = swatch->mapToItem(chip, QPointF {});
        QCOMPARE(swatchPosition.x(), chip->property("leftPadding").toReal());

        const QRectF swatchRect(swatch->mapToItem(picker, QPointF {}), swatch->size());
        const QRectF labelRect(label->mapToItem(picker, QPointF {}), label->size());
        QVERIFY(! swatchRect.intersects(labelRect));
        QVERIFY(swatchRect.right() <= labelRect.left());
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

    void targetSelection() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                id: root
                width: 600
                height: 100
                property string selection: "all"
                MD.FilterChip {
                    objectName: "all"
                    text: "All"
                    selected: root.selection === "all"
                    onSelectionRequested: root.selection = "all"
                }
                MD.FilterChip {
                    objectName: "a"
                    x: 200
                    text: "A"
                    selected: root.selection === "a"
                    onSelectionRequested: root.selection = "a"
                }
                MD.FilterChip {
                    objectName: "b"
                    x: 400
                    text: "B"
                    selected: root.selection === "b"
                    onSelectionRequested: root.selection = "b"
                }
            }
        )",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QQuickWindow window;
        window.resize(600, 100);
        std::unique_ptr<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        auto* item = qobject_cast<QQuickItem*>(root.get());
        QVERIFY(item);
        item->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* all = root->findChild<qml_material::SelectionButton*>("all");
        auto* a   = root->findChild<qml_material::SelectionButton*>("a");
        auto* b   = root->findChild<qml_material::SelectionButton*>("b");
        QVERIFY(all && a && b);
        const auto leading = [](qml_material::SelectionButton* button) {
            return button->contentItem()->childItems().front();
        };
        const auto verifyIcon = [&](qml_material::SelectionButton* selected,
                                    qml_material::SelectionButton* deselected) {
            auto* icon  = leading(selected);
            auto* inner = icon->childItems().front();
            QTRY_VERIFY(inner->implicitWidth() > 0);
            QTRY_COMPARE(icon->implicitWidth(), inner->implicitWidth());
            QTRY_COMPARE(icon->opacity(), 1);
            QVERIFY(icon->isVisible());
            QTRY_VERIFY(icon->width() > 0);
            QTRY_COMPARE(leading(deselected)->implicitWidth(), 0);
            QTRY_COMPARE(leading(deselected)->opacity(), 0);
        };
        for (int i = 0; i < 3; ++i) {
            a->click();
            QVERIFY(a->isSelected() && ! b->isSelected() && ! all->isSelected());
            verifyIcon(a, all);
            b->click();
            QVERIFY(! a->isSelected() && b->isSelected() && ! all->isSelected());
            verifyIcon(b, a);
            all->click();
            QVERIFY(! a->isSelected() && ! b->isSelected() && all->isSelected());
            verifyIcon(all, b);
        }
    }

    void applicationWindowOwnsContent() {
        QTest::failOnWarning(QRegularExpression(QStringLiteral(".*contentItem.*overrides.*")));
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD

            MD.ApplicationWindow {
                width: 320
                height: 240
                property alias contentChild: contentChild
                property alias appBackground: appBackground
                property alias appMenuBar: appMenuBar
                property alias appHeader: appHeader
                property alias appFooter: appFooter

                background: Item { id: appBackground }
                menuBar: Item { id: appMenuBar }
                header: Item { id: appHeader }
                footer: Item { id: appFooter }

                Item {
                    id: contentChild
                    objectName: "contentChild"
                    width: 16
                    height: 16
                }
            }
        )",
                          QUrl(QStringLiteral("qrc:/tests/application-window-content.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* window = qobject_cast<qml_material::ApplicationWindow*>(object.get());
        QVERIFY(window);

        auto* contentChild = qvariant_cast<QQuickItem*>(object->property("contentChild"));
        QVERIFY(contentChild);
        QCOMPARE(contentChild->parentItem(), window->contentItem());
        QCOMPARE(window->background(),
                 qvariant_cast<QQuickItem*>(object->property("appBackground")));
        QCOMPARE(window->menuBar(), qvariant_cast<QQuickItem*>(object->property("appMenuBar")));
        QCOMPARE(window->header(), qvariant_cast<QQuickItem*>(object->property("appHeader")));
        QCOMPARE(window->footer(), qvariant_cast<QQuickItem*>(object->property("appFooter")));
        QCOMPARE(window->frameItem()->parentItem(),
                 static_cast<QQuickWindow*>(window)->contentItem());
        QCOMPARE(window->frameItem()->size(),
                 static_cast<QQuickWindow*>(window)->contentItem()->size());
    }

private:
    QQmlEngine   m_engine;
    QQuickWindow m_window;
};

int run_control_layout(int argc, char** argv) {
    ControlLayoutTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "control_layout.moc"
