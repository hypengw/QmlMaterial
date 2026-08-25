#include <QCoreApplication>
#include <QColor>
#include <QFont>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QtTest>
#include <memory>

#ifdef Q_OS_WIN
#    include <QAbstractNativeEventFilter>
#    include <windows.h>
#endif

#include "qml_material/control/tool_tip.hpp"
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

QQuickItem* itemWithAction(QQuickItem* root, QObject* action) {
    if (qvariant_cast<QObject*>(root->property("action")) == action) {
        return root;
    }
    for (auto* child : root->childItems()) {
        if (auto* match = itemWithAction(child, action)) {
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

class ControlLayoutTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() {
        m_engine.addImportPath(QCoreApplication::applicationDirPath()
                               + QStringLiteral("/../qml_modules"));
        const QByteArray envPath = qgetenv("QML_IMPORT_PATH");
        if (! envPath.isEmpty()) {
#if defined(Q_OS_WIN)
            const QList<QByteArray> parts = envPath.split(';');
#else
            const QList<QByteArray> parts = envPath.split(':');
#endif
            for (const QByteArray& part : parts) {
                if (! part.isEmpty())
                    m_engine.addImportPath(QString::fromLocal8Bit(part));
            }
        }
        m_window.setGeometry(0, 0, 800, 600);
        m_window.create();
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
                    action: parent.railAction
                    iconStyle: MD.Enum.IconOnly
                }
                MD.RailItem {
                    objectName: "expanded"
                    x: 120
                    expand: true
                    action: parent.railAction
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
            QCOMPARE(icon->x(), expectedIconX);
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

    void attachedToolTipLifecycle() {
        const auto source = QByteArrayLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            Item {
                width: 320
                height: 120

                Item {
                    objectName: "first"
                    width: 80
                    height: 40
                    MD.ToolTip.text: "First"
                    MD.ToolTip.delay: 0
                    MD.ToolTip.timeout: -1
                }
                Item {
                    objectName: "second"
                    x: 100
                    width: 80
                    height: 40
                    MD.ToolTip.text: "Second"
                    MD.ToolTip.delay: 0
                    MD.ToolTip.timeout: -1
                }
            }
        )");

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/attached-tooltip.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(root);
        root->setParentItem(m_window.contentItem());

        auto* first  = root->findChild<QQuickItem*>(QStringLiteral("first"));
        auto* second = root->findChild<QQuickItem*>(QStringLiteral("second"));
        QVERIFY(first);
        QVERIFY(second);

        auto* firstAttached  = attachedToolTip(first);
        auto* secondAttached = attachedToolTip(second);
        QVERIFY(firstAttached);
        QVERIFY(secondAttached);
        QCOMPARE(firstAttached->text(), QStringLiteral("First"));
        QCOMPARE(firstAttached->delay(), 0);
        QCOMPARE(firstAttached->timeout(), -1);
        QVERIFY(! firstAttached->visible());

        auto* sharedToolTip = firstAttached->toolTip();
        QVERIFY(sharedToolTip);
        QCOMPARE(secondAttached->toolTip(), sharedToolTip);

        firstAttached->show(firstAttached->text(), 0);
        QTRY_VERIFY(firstAttached->visible());
        QCOMPARE(sharedToolTip->property("text").toString(), QStringLiteral("First"));
        QCOMPARE(qvariant_cast<QQuickItem*>(sharedToolTip->property("parent")), first);

        secondAttached->setText(QStringLiteral("Inactive second"));
        QCOMPARE(sharedToolTip->property("text").toString(), QStringLiteral("First"));
        firstAttached->setText(QStringLiteral("Updated first"));
        QCOMPARE(sharedToolTip->property("text").toString(), QStringLiteral("Updated first"));

        QVERIFY(sharedToolTip->setProperty("width", 321.0));
        QVERIFY(sharedToolTip->setProperty("height", 123.0));
        secondAttached->show(secondAttached->text(), 0);
        QTRY_VERIFY(secondAttached->visible());
        QVERIFY(! firstAttached->visible());
        QCOMPARE(qvariant_cast<QQuickItem*>(sharedToolTip->property("parent")), second);
        QCOMPARE(sharedToolTip->property("text").toString(), QStringLiteral("Inactive second"));
        QVERIFY(sharedToolTip->property("width").toReal() != 321.0);
        QVERIFY(sharedToolTip->property("height").toReal() != 123.0);

        firstAttached->hide();
        QVERIFY(secondAttached->visible());
        secondAttached->hide();
        QTRY_VERIFY(! secondAttached->visible());

        firstAttached->setDelay(60);
        firstAttached->show(QStringLiteral("Delayed"));
        QVERIFY(! firstAttached->visible());
        firstAttached->hide();
        QTest::qWait(100);
        QVERIFY(! sharedToolTip->property("visible").toBool());

        firstAttached->setDelay(0);
        firstAttached->setTimeout(40);
        firstAttached->show(QStringLiteral("Timed"));
        QTRY_VERIFY(firstAttached->visible());
        QTRY_VERIFY_WITH_TIMEOUT(! firstAttached->visible(), 1000);

        firstAttached->setTimeout(-1);
        firstAttached->show(QStringLiteral("Before destruction"), 0);
        QTRY_VERIFY(firstAttached->visible());
        QPointer<QQuickItem> firstGuard(first);
        delete first;
        QVERIFY(firstGuard.isNull());
        QTRY_VERIFY(! sharedToolTip->property("visible").toBool());

        QQmlEngine otherEngine;
        QQmlComponent otherComponent(&otherEngine);
        otherComponent.setData(
            QByteArrayLiteral(R"(
                import QtQuick
                import Qcm.Material as MD
                Item { MD.ToolTip.text: "Other engine" }
            )"),
            QUrl(QStringLiteral("qrc:/tests/attached-tooltip-other-engine.qml")));
        QVERIFY2(! otherComponent.isError(), qPrintable(otherComponent.errorString()));
        std::unique_ptr<QObject> otherObject(otherComponent.create());
        QVERIFY2(otherObject, qPrintable(otherComponent.errorString()));
        auto* otherAttached = attachedToolTip(otherObject.get());
        QVERIFY(otherAttached);
        QVERIFY(otherAttached->toolTip());
        QVERIFY(otherAttached->toolTip() != sharedToolTip);
    }

    void toolTipTypeContract() {
        QQmlComponent initiallyVisible(&m_engine);
        initiallyVisible.setData(
            QByteArrayLiteral(R"(
                import QtQuick
                import Qcm.Material as MD
                Item {
                    width: 80
                    height: 40
                    MD.ToolTip.visible: true
                    MD.ToolTip.delay: 0
                    MD.ToolTip.text: "Initially visible"
                }
            )"),
            QUrl(QStringLiteral("qrc:/tests/tooltip-initially-visible.qml")));
        QVERIFY2(! initiallyVisible.isError(), qPrintable(initiallyVisible.errorString()));
        std::unique_ptr<QObject> initiallyVisibleObject(initiallyVisible.create());
        QVERIFY2(initiallyVisibleObject, qPrintable(initiallyVisible.errorString()));
        auto* initiallyVisibleItem = qobject_cast<QQuickItem*>(initiallyVisibleObject.get());
        QVERIFY(initiallyVisibleItem);
        initiallyVisibleItem->setParentItem(m_window.contentItem());
        auto* initiallyVisibleAttached = attachedToolTip(initiallyVisibleItem);
        QVERIFY(initiallyVisibleAttached);
        QTRY_VERIFY(initiallyVisibleAttached->visible());
        QCOMPARE(initiallyVisibleAttached->toolTip()->property("text").toString(),
                 QStringLiteral("Initially visible"));
        initiallyVisibleAttached->hide();

        QQmlComponent attachedOnly(&m_engine);
        attachedOnly.setData(
            QByteArrayLiteral(R"(
                import Qcm.Material as MD
                MD.ToolTip {}
            )"),
            QUrl(QStringLiteral("qrc:/tests/tooltip-uncreatable.qml")));
        QVERIFY(attachedOnly.isError());

        QQmlComponent plain(&m_engine);
        plain.setData(
            QByteArrayLiteral(R"(
                import Qcm.Material as MD
                MD.PlainToolTip {}
            )"),
            QUrl(QStringLiteral("qrc:/tests/plain-tooltip.qml")));
        QVERIFY2(! plain.isError(), qPrintable(plain.errorString()));
        std::unique_ptr<QObject> plainObject(plain.create());
        QVERIFY2(plainObject, qPrintable(plain.errorString()));
    }

    void actionToolBarTooltips() {
        const auto source = QByteArrayLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            Item {
                width: 240
                height: 64

                property MD.Action explicitAction: MD.Action {
                    text: "Refresh"
                    tooltip: "Reload content"
                    icon.name: MD.Token.icon.refresh
                }
                property MD.Action fallbackAction: MD.Action {
                    text: "Settings"
                    icon.name: MD.Token.icon.settings
                }

                MD.ActionToolBar {
                    objectName: "toolbar"
                    anchors.fill: parent
                    actions: [parent.explicitAction, parent.fallbackAction]
                }
            }
        )");

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/action-toolbar-tooltip.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(root);
        root->setParentItem(m_window.contentItem());
        settle(root, 12);

        auto* toolbar        = root->findChild<QQuickItem*>(QStringLiteral("toolbar"));
        auto* explicitAction = qvariant_cast<QObject*>(root->property("explicitAction"));
        auto* fallbackAction = qvariant_cast<QObject*>(root->property("fallbackAction"));
        QVERIFY(toolbar);
        QVERIFY(explicitAction);
        QVERIFY(fallbackAction);

        auto* explicitButton = itemWithAction(toolbar, explicitAction);
        auto* fallbackButton = itemWithAction(toolbar, fallbackAction);
        auto* moreAction     = qvariant_cast<QObject*>(toolbar->property("moreAction"));
        auto* moreButton     = itemWithAction(toolbar, moreAction);
        QVERIFY(explicitButton);
        QVERIFY(fallbackButton);
        QVERIFY(moreAction);
        QVERIFY(moreButton);

        QCOMPARE(explicitButton->property("toolTipText").toString(),
                 QStringLiteral("Reload content"));
        QCOMPARE(fallbackButton->property("toolTipText").toString(), QStringLiteral("Settings"));
        QCOMPARE(moreButton->property("toolTipText").toString(), QStringLiteral("More actions"));

        auto* explicitToolTip = attachedToolTip(explicitButton);
        auto* fallbackToolTip = attachedToolTip(fallbackButton);
        auto* moreToolTip     = attachedToolTip(moreButton);
        QVERIFY(explicitToolTip);
        QVERIFY(fallbackToolTip);
        QVERIFY(moreToolTip);
        QCOMPARE(explicitToolTip->text(), QStringLiteral("Reload content"));
        QCOMPARE(fallbackToolTip->text(), QStringLiteral("Settings"));
        QCOMPARE(moreToolTip->text(), QStringLiteral("More actions"));

        auto* sharedToolTip = explicitToolTip->toolTip();
        QVERIFY(sharedToolTip);
        QCOMPARE(fallbackToolTip->toolTip(), sharedToolTip);
        QCOMPARE(moreToolTip->toolTip(), sharedToolTip);

        explicitToolTip->show(explicitToolTip->text(), 0);
        QTRY_VERIFY(explicitToolTip->visible());
        QCOMPARE(sharedToolTip->property("text").toString(), QStringLiteral("Reload content"));
        QCOMPARE(qvariant_cast<QQuickItem*>(sharedToolTip->property("parent")), explicitButton);

        fallbackToolTip->show(fallbackToolTip->text(), 0);
        QTRY_VERIFY(fallbackToolTip->visible());
        QVERIFY(! explicitToolTip->visible());
        QCOMPARE(qvariant_cast<QQuickItem*>(sharedToolTip->property("parent")), fallbackButton);

        explicitToolTip->hide();
        QVERIFY(fallbackToolTip->visible());
        fallbackToolTip->hide();
        QTRY_VERIFY(! fallbackToolTip->visible());

        moreToolTip->show(moreToolTip->text(), 0);
        QTRY_VERIFY(moreToolTip->visible());
        QCOMPARE(sharedToolTip->property("text").toString(), QStringLiteral("More actions"));
        moreToolTip->hide();

        QVERIFY(explicitAction->setProperty("tooltip", QStringLiteral("Reload current page")));
        settle(root);
        QCOMPARE(explicitButton->property("toolTipText").toString(),
                 QStringLiteral("Reload current page"));
        QCOMPARE(explicitToolTip->text(), QStringLiteral("Reload current page"));
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
        QTRY_VERIFY_WITH_TIMEOUT(
            QMetaObject::invokeMethod(
                menu, "itemAt", Q_RETURN_ARG(QQuickItem*, delegate), Q_ARG(int, 0))
                && delegate,
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
        QCOMPARE(placeholderFont.pixelSize(), expectedSize);
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

        QTest::newRow("xs") << QStringLiteral("mdState.size: MD.Enum.XS") << 32.0 << 8.0
                            << 18.0 << 0.0 << 12 << false;
        QTest::newRow("s") << QStringLiteral("mdState.size: MD.Enum.S") << 40.0 << 10.0
                           << 20.0 << 0.0 << 14 << false;
        QTest::newRow("default-m") << QString() << 48.0 << 12.0 << 24.0 << 0.0 << 16 << true;
        QTest::newRow("l") << QStringLiteral("mdState.size: MD.Enum.L") << 56.0 << 16.0
                           << 24.0 << 0.0 << 16 << false;
        QTest::newRow("xl") << QStringLiteral("mdState.size: MD.Enum.XL") << 72.0 << 24.0
                            << 32.0 << 0.0 << 16 << false;
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

        auto* swatch = itemWithImplicitSizeAndColor(
            picker, QSizeF(20, 20), QColor(QStringLiteral("#6750A4")));
        auto* label = itemWithText(picker, QStringLiteral("#6750A4FF"));
        QVERIFY(swatch);
        QVERIFY(label);

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

private:
    QQmlEngine   m_engine;
    QQuickWindow m_window;
};

#ifdef Q_OS_WIN
// Same workaround as example/main.cpp: Windows UIA + Qt accessibility can
// crash while Material controls are created/pressed.
class UiaBlocker : public QAbstractNativeEventFilter {
public:
    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override {
        if (eventType == "windows_generic_MSG") {
            if (static_cast<MSG*>(message)->message == WM_GETOBJECT) {
                *result = 0;
                return true;
            }
        }
        return false;
    }
};
#endif

int main(int argc, char* argv[]) {
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "0");
    qputenv("QT_SCALE_FACTOR", "1");

    QGuiApplication app(argc, argv);
#ifdef Q_OS_WIN
    static UiaBlocker uiaBlocker;
    app.installNativeEventFilter(&uiaBlocker);
#endif

    ControlLayoutTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "control_layout.moc"
