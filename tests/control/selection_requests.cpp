#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include "qml_material/control/button_control.hpp"
#include "qml_material/control/button_group.hpp"
#include "qml_material/control/radio_button.hpp"
#include "qml_material/control/popup.hpp"
#include "qml_material/control/tab_bar.hpp"
#include "qml_material/control/navigation_control.hpp"
#include "qml_material/style/radio_button_state.hpp"
#include "qml_material/style/chip_state.hpp"
#include "qml_material/token/color.hpp"
#include "../input_test.hpp"

using namespace qml_material;
namespace
{
std::unique_ptr<SelectionButton> selectionFor(int kind) {
    if (kind == 1) return std::make_unique<RadioButton>();
    if (kind == 2) return std::make_unique<TabButton>();
    return std::make_unique<SelectionButton>();
}
SelectionButton* navigationItem(QQuickItem* root, const QString& text) {
    if (auto* item = qobject_cast<SelectionButton*>(root); item && item->text() == text)
        return item;
    for (auto* child : root->childItems())
        if (auto* item = navigationItem(child, text)) return item;
    return nullptr;
}
QByteArray selectionOwner(const QByteArray& kind) {
    return R"(
        import QtQuick
        import Qcm.Material as MD
        Item {
            id: owner
            width: 240; height: 120
            property bool value: false
            property bool accept: false
            property bool toggle: false
            property int requests: 0
            MD.)" +
           kind + R"( {
                objectName: "control"
                width: 100; height: 60
                selected: owner.value
                onSelectionRequested: {
                    owner.requests++
                    if (owner.accept) owner.value = owner.toggle ? !owner.value : true
                }
            }
        }
    )";
}
constexpr auto tabSource = R"(
    import QtQuick
    import Qcm.Material as MD
    MD.TabBar {
        id: control
        width: 360; height: 48
        property bool accept: false
        property int committedIndex: 0
        property int requests: 0
        property int lastRequest: -99
        currentIndex: committedIndex
        onCurrentIndexChangeRequested: index => {
            requests++
            lastRequest = index
            if (accept) committedIndex = index
        }
        MD.TabButton { text: "First" }
        MD.TabButton { text: "Second" }
        MD.TabButton { text: "Third" }
    }
)";
} // namespace

class SelectionRequestsTest : public QObject {
    Q_OBJECT

private slots:
    void init() { QTest::failOnWarning(QRegularExpression(".*")); }

    void inputRequests_data() {
        QTest::addColumn<int>("kind");
        QTest::addColumn<int>("input");
        QTest::addColumn<bool>("accept");
        QTest::addColumn<bool>("initial");
        for (int kind = 0; kind < 3; ++kind)
            for (int input = 0; input < 4; ++input)
                if (kind == 0 || input == 1)
                    for (bool accept : { false, true })
                        for (bool initial : { false, true })
                            QTest::newRow(qPrintable(QString("%1-%2-%3-%4")
                                                         .arg(kind)
                                                         .arg(input)
                                                         .arg(accept)
                                                         .arg(initial)))
                                << kind << input << accept << initial;
    }

    void inputRequests() {
        QFETCH(int, kind);
        QFETCH(int, input);
        QFETCH(bool, accept);
        QFETCH(bool, initial);
        QProperty<bool> owner(initial);
        auto            control = selectionFor(kind);
        control->bindableSelected().setBinding([&] {
            return owner.value();
        });
        QQuickWindow window;
        window.resize(200, 100);
        control->setParentItem(window.contentItem());
        control->setSize({ 100, 50 });
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QSignalSpy requests(control.get(), &SelectionButton::selectionRequested);
        connect(control.get(), &SelectionButton::selectionRequested, this, [&] {
            if (accept) owner = true;
        });
        const auto activate = [&] {
            if (input == 0)
                control->click();
            else if (input == 1)
                QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, { 20, 20 });
            else if (input == 2) {
                control->forceActiveFocus(Qt::TabFocusReason);
                QTest::keyClick(&window, Qt::Key_Space);
            } else {
                static auto* device = QTest::createTouchDevice();
                QTest::touchEvent(&window, device).press(0, { 20, 20 }, &window).commit();
                QTest::touchEvent(&window, device).release(0, { 20, 20 }, &window).commit();
            }
            flushInteractions();
        };
        activate();
        activate();
        QCOMPARE(requests.size(), 2);
        QCOMPARE(control->isSelected(), accept || initial);
        owner = ! owner.value();
        QCOMPARE(control->isSelected(), owner.value());
        QVERIFY(control->bindableSelected().hasBinding());
    }

    void qmlOwner_data() {
        QTest::addColumn<QByteArray>("kind");
        for (const auto& kind : { QByteArray("SelectionButtonBase"),
                                  QByteArray("RadioButton"),
                                  QByteArray("TabButton"),
                                  QByteArray("RailItem"),
                                  QByteArray("BarItem"),
                                  QByteArray("DrawerItem"),
                                  QByteArray("FilterChip"),
                                  QByteArray("InputChip") })
            QTest::newRow(kind.constData()) << kind;
    }

    void qmlOwner() {
        QFETCH(QByteArray, kind);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(selectionOwner(kind), QUrl("qrc:/tests/selection-owner.qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* control = object->findChild<SelectionButton*>("control");
        QVERIFY(control);
        control->click();
        QVERIFY(! control->isSelected());
        object->setProperty("accept", true);
        control->click();
        QVERIFY(control->isSelected());
        control->click();
        QVERIFY(control->isSelected());
        QCOMPARE(object->property("requests").toInt(), 3);
        object->setProperty("value", false);
        QVERIFY(! control->isSelected());
        if (auto* state = control->findChild<RadioButtonState*>())
            QCOMPARE(state->iconColor(), state->colors()->on_surface_variant());
    }

    void chipInput_data() {
        QTest::addColumn<QByteArray>("kind");
        QTest::addColumn<int>("input");
        QTest::addColumn<bool>("accept");
        QTest::addColumn<bool>("initial");
        for (const QByteArray kind : { "FilterChip", "InputChip" })
            for (int input = 0; input < 4; ++input)
                for (bool accept : { false, true })
                    for (bool initial : { false, true })
                        QTest::newRow(qPrintable(QString("%1-%2-%3-%4")
                                                     .arg(QString::fromLatin1(kind))
                                                     .arg(input)
                                                     .arg(accept)
                                                     .arg(initial)))
                            << kind << input << accept << initial;
    }

    void chipInput() {
        QFETCH(QByteArray, kind);
        QFETCH(int, input);
        QFETCH(bool, accept);
        QFETCH(bool, initial);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(selectionOwner(kind), QUrl());
        std::unique_ptr<QObject> object(component.createWithInitialProperties(
            { { "accept", accept }, { "value", initial }, { "toggle", true } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root    = qobject_cast<QQuickItem*>(object.get());
        auto* control = object->findChild<SelectionButton*>("control");
        QVERIFY(root && control);
        auto* state = control->findChild<ChipState*>();
        QVERIFY(state && state->isSelected() == initial);
        QQuickWindow window;
        window.resize(240, 120);
        root->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        window.requestActivate();
        QTRY_VERIFY(window.isActive());
        const auto activate = [&] {
            if (input == 0)
                control->click();
            else if (input == 1)
                QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, { 20, 20 });
            else if (input == 2) {
                control->forceActiveFocus(Qt::TabFocusReason);
                QTest::keyClick(&window, Qt::Key_Space);
            } else {
                static auto* device = QTest::createTouchDevice();
                QTest::touchEvent(&window, device).press(0, { 20, 20 }, &window).commit();
                QTest::touchEvent(&window, device).release(0, { 20, 20 }, &window).commit();
            }
            flushInteractions();
        };
        activate();
        QCOMPARE(control->isSelected(), accept ? ! initial : initial);
        QCOMPARE(state->isSelected(), control->isSelected());
        activate();
        QCOMPARE(object->property("requests").toInt(), 2);
        QCOMPARE(control->isSelected(), initial);
        object->setProperty("value", ! initial);
        QCOMPARE(control->isSelected(), ! initial);
        QCOMPARE(state->isSelected(), ! initial);
        control->setEnabled(false);
        activate();
        QCOMPARE(object->property("requests").toInt(), 2);
        object->setProperty("value", initial);
        QCOMPARE(control->isSelected(), initial);
        QCOMPARE(state->isSelected(), initial);
    }

    void chipCancelled_data() {
        QTest::addColumn<QByteArray>("kind");
        QTest::addColumn<int>("change");
        for (const QByteArray kind : { "FilterChip", "InputChip" })
            for (int change = 0; change < 3; ++change)
                QTest::newRow(qPrintable(QString::fromLatin1(kind) + QString::number(change)))
                    << kind << change;
    }

    void chipCancelled() {
        QFETCH(QByteArray, kind);
        QFETCH(int, change);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(selectionOwner(kind), QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* control = object->findChild<SelectionButton*>("control");
        QVERIFY(control);
        InteractionAccumulator feedback(this);
        feedback.setSource(control->interactionSource());
        setButtonPressed(control, true);
        QVERIFY(feedback.values().pressed);
        InteractionSource replacement;
        if (change == 0)
            control->setEnabled(false);
        else if (change == 1)
            control->setVisible(false);
        else
            control->setInteractionSource(&replacement);
        QMouseEvent release(QEvent::MouseButtonRelease,
                            { 20, 20 },
                            { 20, 20 },
                            Qt::LeftButton,
                            Qt::NoButton,
                            Qt::NoModifier);
        QCoreApplication::sendEvent(control, &release);
        flushInteractions();
        QCOMPARE(object->property("requests").toInt(), 0);
        QVERIFY(! feedback.values().pressed && ! control->isSelected());
    }

    void chipDeletedInRequest_data() {
        QTest::addColumn<QByteArray>("kind");
        for (const QByteArray kind : { "FilterChip", "InputChip" })
            QTest::newRow(kind.constData()) << kind;
    }

    void chipDeletedInRequest() {
        QFETCH(QByteArray, kind);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(selectionOwner(kind), QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* control = object->findChild<SelectionButton*>("control");
        QVERIFY(control);
        const QPointer<SelectionButton> alive(control);
        int                             requests = 0;
        connect(control, &SelectionButton::selectionRequested, this, [&] {
            ++requests;
            object.reset();
        });
        control->click();
        QCOMPARE(requests, 1);
        QVERIFY(! alive && ! object);
    }

    void chipDeletedInSelectionNotification_data() { chipDeletedInRequest_data(); }

    void chipDeletedInSelectionNotification() {
        QFETCH(QByteArray, kind);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                id: root
                property bool value: false
                MD.)" + kind + R"( { objectName: "first"; selected: root.value }
                MD.)" + kind + R"( { objectName: "second"; selected: root.value }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        QPointer<SelectionButton> first  = object->findChild<SelectionButton*>("first");
        QPointer<SelectionButton> second = object->findChild<SelectionButton*>("second");
        QVERIFY(first && second);
        int notifications = 0;
        connect(first, &SelectionButton::selectedChanged, this, [&] {
            ++notifications;
            delete second.data();
        });
        connect(second, &SelectionButton::selectedChanged, this, [&] {
            ++notifications;
            delete first.data();
        });
        object->setProperty("value", true);
        QCOMPARE(notifications, 1);
        QVERIFY(bool(first) != bool(second));
    }

    void navigationInput_data() {
        QTest::addColumn<QByteArray>("kind");
        QTest::addColumn<int>("input");
        QTest::addColumn<bool>("accept");
        QTest::addColumn<bool>("initial");
        for (const auto& kind :
             { QByteArray("RailItem"), QByteArray("BarItem"), QByteArray("DrawerItem") })
            for (int input = 0; input < 4; ++input)
                for (bool accept : { false, true })
                    for (bool initial : { false, true })
                        QTest::newRow(qPrintable(QString("%1-%2-%3-%4")
                                                     .arg(QString::fromUtf8(kind))
                                                     .arg(input)
                                                     .arg(accept)
                                                     .arg(initial)))
                            << kind << input << accept << initial;
    }

    void navigationInput() {
        QFETCH(QByteArray, kind);
        QFETCH(int, input);
        QFETCH(bool, accept);
        QFETCH(bool, initial);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(selectionOwner(kind), QUrl("qrc:/tests/navigation-input.qml"));
        std::unique_ptr<QObject> object(
            component.createWithInitialProperties({ { "accept", accept }, { "value", initial } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root    = qobject_cast<QQuickItem*>(object.get());
        auto* control = object->findChild<SelectionButton*>("control");
        QVERIFY(root && control);
        QQuickWindow window;
        window.resize(240, 120);
        root->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        const auto activate = [&] {
            if (input == 0)
                control->click();
            else if (input == 1)
                QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, { 20, 20 });
            else if (input == 2) {
                control->forceActiveFocus(Qt::TabFocusReason);
                QTest::keyClick(&window, Qt::Key_Space);
            } else {
                static auto* device = QTest::createTouchDevice();
                QTest::touchEvent(&window, device).press(0, { 20, 20 }, &window).commit();
                QTest::touchEvent(&window, device).release(0, { 20, 20 }, &window).commit();
            }
            flushInteractions();
        };
        activate();
        activate();
        QCOMPARE(object->property("requests").toInt(), 2);
        QCOMPARE(control->isSelected(), accept || initial);
        QVERIFY(control->icon()->fill() == control->isSelected());
        object->setProperty("value", ! control->isSelected());
        QCOMPARE(control->isSelected(), ! (accept || initial));
        control->setEnabled(false);
        activate();
        QCOMPARE(object->property("requests").toInt(), 2);
    }

    void navigationOwner_data() {
        QTest::addColumn<QByteArray>("kind");
        QTest::addColumn<bool>("accept");
        QTest::addColumn<bool>("modal");
        for (const auto& kind : { QByteArray("NavigationRail"), QByteArray("StandardDrawer") })
            for (bool accept : { false, true })
                for (bool modal : { false, true })
                    QTest::newRow(qPrintable(
                        QString("%1-%2-%3").arg(QString::fromUtf8(kind)).arg(accept).arg(modal)))
                        << kind << accept << modal;
    }

    void navigationOwner() {
        QFETCH(QByteArray, kind);
        QFETCH(bool, accept);
        QFETCH(bool, modal);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                id: owner
                width: modal ? 840 : 1280; height: 500
                property bool modal: false
                property int selection: 0
                property bool accept: false
                property bool closeOnSelection: false
                property int requests: 0
                property int lastRequest: -1
                property var rows: [{name: "One"}, {name: "Two"}, {name: "Three"}]
                MD.MProp.size: MD.ThemeSize { width: owner.width; duration: 0 }
                MD.)" + kind + R"( {
                    id: navigation
                    objectName: "navigation"
                    width: 220; height: 400
                    expanded: !owner.modal
                    header: null
                    currentIndex: owner.selection
                    model: owner.rows
                    onCurrentIndexChangeRequested: index => {
                        owner.requests++
                        owner.lastRequest = index
                        if (owner.accept) {
                            owner.selection = index
                            if (owner.closeOnSelection) navigation.close()
                        }
                    }
                }
            }
        )",
                          QUrl("qrc:/tests/navigation-owner.qml"));
        std::unique_ptr<QObject> object(
            component.createWithInitialProperties({ { "accept", accept }, { "modal", modal } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(root);
        QQuickWindow window;
        window.resize(modal ? 840 : 1280, 500);
        root->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* navigation = root->findChild<QQuickItem*>("navigation");
        QVERIFY(navigation);
        if (modal) {
            QVERIFY(QMetaObject::invokeMethod(navigation, "open"));
            QTRY_VERIFY(navigation->property("drawerOpened").toBool());
        }
        QCOMPARE(navigation->metaObject()->indexOfSignal("clicked(QVariant)"), -1);
        SelectionButton *first = nullptr, *second = nullptr, *third = nullptr;
        QTRY_VERIFY((first = navigationItem(window.contentItem(), "One")) &&
                    (second = navigationItem(window.contentItem(), "Two")) &&
                    (third = navigationItem(window.contentItem(), "Three")));
        QVERIFY(first->isSelected() && ! second->isSelected());
        second->click();
        second->click();
        QCOMPARE(object->property("requests").toInt(), 2);
        QCOMPARE(object->property("lastRequest").toInt(), 1);
        QCOMPARE(navigation->property("currentIndex").toInt(), accept ? 1 : 0);
        QCOMPARE(second->isSelected(), accept);
        object->setProperty("selection", 2);
        QCOMPARE(navigation->property("currentIndex").toInt(), 2);
        QTRY_VERIFY(third->isSelected() && ! first->isSelected() && ! second->isSelected());
        object->setProperty("accept", true);
        first->click();
        QCOMPARE(navigation->property("currentIndex").toInt(), 0);
        QVERIFY(first->isSelected() && ! third->isSelected());
        // Selection requests do not own the drawer's presentation state.
        QVERIFY(navigation->property(modal ? "drawerOpened" : "expanded").toBool());
        navigation->setEnabled(false);
        second->click();
        QCOMPARE(object->property("requests").toInt(), 3);
        navigation->setEnabled(true);
        navigation->setVisible(false);
        second->click();
        QCOMPARE(object->property("requests").toInt(), 3);
        navigation->setVisible(true);
        object->setProperty("closeOnSelection", true);
        third->click();
        QCOMPARE(object->property("requests").toInt(), 4);
        QCOMPARE(navigation->property("currentIndex").toInt(), 2);
        QTRY_VERIFY(! navigation->property(modal ? "drawerOpened" : "expanded").toBool());
    }

    void navigationDeletedInRequest_data() {
        QTest::addColumn<QByteArray>("kind");
        QTest::addColumn<bool>("modal");
        QTest::addColumn<bool>("wholeOwner");
        for (const auto& kind : { QByteArray("NavigationRail"), QByteArray("StandardDrawer") })
            for (bool modal : { false, true })
                for (bool wholeOwner : { false, true })
                    QTest::newRow(qPrintable(QString("%1-%2-%3")
                                                 .arg(QString::fromUtf8(kind))
                                                 .arg(modal)
                                                 .arg(wholeOwner)))
                        << kind << modal << wholeOwner;
    }

    void navigationDeletedInRequest() {
        QFETCH(QByteArray, kind);
        QFETCH(bool, modal);
        QFETCH(bool, wholeOwner);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                id: owner
                property bool modal: false
                width: modal ? 840 : 1280; height: 500
                MD.MProp.size: MD.ThemeSize { width: owner.width; duration: 0 }
                MD.)" + kind + R"( {
                    objectName: "navigation"
                    width: 220; height: 400
                    expanded: !owner.modal
                    header: null
                    model: [{name: "One"}, {name: "Two"}]
                }
            }
        )",
                          QUrl("qrc:/tests/navigation-delete-in-request.qml"));
        QQuickWindow             window;
        std::unique_ptr<QObject> object(
            component.createWithInitialProperties({ { "modal", modal } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root       = qobject_cast<QQuickItem*>(object.get());
        auto* navigation = object->findChild<QQuickItem*>("navigation");
        QVERIFY(root && navigation);
        window.resize(modal ? 840 : 1280, 500);
        root->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        if (modal) {
            QVERIFY(QMetaObject::invokeMethod(navigation, "open"));
            QTRY_VERIFY(navigation->property("drawerOpened").toBool());
        }
        SelectionButton* second = nullptr;
        QTRY_VERIFY((second = navigationItem(window.contentItem(), "Two")));
        QPointer<QQuickItem>      alive(navigation);
        QPointer<SelectionButton> button(second);
        bool                      requested = false;
        auto*                     owner     = qobject_cast<NavigationControl*>(navigation);
        QVERIFY(owner);
        connect(owner, &NavigationControl::currentIndexChangeRequested, this, [&](int index) {
            QCOMPARE(index, 1);
            requested = true;
            if (wholeOwner)
                object.reset();
            else
                delete navigation;
        });
        second->click();
        QVERIFY(requested && ! alive && ! button);
        QCOMPARE(bool(object), ! wholeOwner);
    }

    void navigationRouting() {
        QProperty<int>    selected(0);
        NavigationControl first, second;
        NavigationItem    item;
        first.bindableCurrentIndex().setBinding([&] {
            return selected.value();
        });
        item.setNavigation(&first);
        item.setNavigationIndex(1);
        QSignalSpy firstRequests(&first, &NavigationControl::currentIndexChangeRequested);
        QSignalSpy secondRequests(&second, &NavigationControl::currentIndexChangeRequested);
        item.click();
        QCOMPARE(firstRequests.size(), 1);
        QCOMPARE(first.currentIndex(), 0);
        connect(&first, &NavigationControl::currentIndexChangeRequested, this, [&](int index) {
            selected = index;
        });
        item.click();
        QCOMPARE(first.currentIndex(), 1);
        QVERIFY(first.bindableCurrentIndex().hasBinding());
        item.click();
        QCOMPARE(firstRequests.size(), 3);
        item.setNavigationIndex(-1);
        item.click();
        QCOMPARE(firstRequests.size(), 3);
        item.setNavigation(&second);
        item.setNavigationIndex(9);
        first.requestSelection(&item);
        QCOMPARE(firstRequests.size(), 3);
        item.click();
        QCOMPARE(secondRequests.size(), 1);
        QCOMPARE(secondRequests.last().first().toInt(), 9);
        QCOMPARE(second.currentIndex(), 0);
        item.setNavigation(nullptr);
        item.click();
        QCOMPARE(secondRequests.size(), 1);
        auto owner = std::make_unique<NavigationControl>();
        item.setNavigation(owner.get());
        QSignalSpy detached(&item, &NavigationItem::navigationChanged);
        owner.reset();
        QCOMPARE(item.navigation(), nullptr);
        QCOMPARE(detached.size(), 1);
        item.click();
    }

    void navigationRetarget_data() {
        QTest::addColumn<bool>("keyboard");
        QTest::addColumn<int>("change");
        for (bool keyboard : { false, true })
            for (int change = 0; change < 5; ++change)
                QTest::newRow(qPrintable(QString("%1-%2").arg(keyboard).arg(change)))
                    << keyboard << change;
    }

    void navigationRetarget() {
        QFETCH(bool, keyboard);
        QFETCH(int, change);
        QQuickWindow      window;
        auto              first = std::make_unique<NavigationControl>(window.contentItem());
        NavigationControl second(window.contentItem());
        NavigationItem    item(window.contentItem());
        item.setSize({ 100, 50 });
        item.setHoverEnabled(true);
        item.setNavigation(first.get());
        item.setNavigationIndex(0);
        InteractionAccumulator feedback(this);
        feedback.setSource(item.interactionSource());
        QSignalSpy traffic(item.interactionSource(), &InteractionSource::interaction);
        QSignalSpy requests(&item, &SelectionButton::selectionRequested);
        QSignalSpy secondRequests(&second, &NavigationControl::currentIndexChangeRequested);
        window.resize(200, 100);
        window.show();
        window.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        item.forceActiveFocus(Qt::TabFocusReason);
        QTRY_VERIFY(feedback.values().focused);
        QHoverEvent hover(QEvent::HoverEnter, QPointF(10, 10), QPointF(10, 10), QPointF());
        QCoreApplication::sendEvent(&item, &hover);
        flushInteractions();
        QVERIFY(feedback.values().hovered && item.focusVisible());
        const auto pointer = [&](QEvent::Type type) {
            QMouseEvent event(type,
                              { 20, 20 },
                              { 20, 20 },
                              Qt::LeftButton,
                              type == QEvent::MouseButtonPress ? Qt::LeftButton : Qt::NoButton,
                              Qt::NoModifier);
            QCoreApplication::sendEvent(&item, &event);
            flushInteractions();
        };
        const auto key = [&](QEvent::Type type) {
            QKeyEvent event(type, Qt::Key_Space, Qt::NoModifier);
            QCoreApplication::sendEvent(&item, &event);
            flushInteractions();
        };
        if (keyboard)
            key(QEvent::KeyPress);
        else
            pointer(QEvent::MouseButtonPress);
        QVERIFY(feedback.values().pressed);
        traffic.clear();
        if (change == 0)
            item.setNavigation(&second);
        else if (change == 1)
            item.setNavigationIndex(1);
        else if (change == 2)
            first.reset();
        else if (change == 3)
            first->setEnabled(false);
        else
            first->setVisible(false);
        flushInteractions();
        QVERIFY(! feedback.values().pressed);
        if (change < 3) {
            QVERIFY(feedback.values().focused && feedback.values().hovered);
            for (const auto& args : traffic) {
                const auto event = args.first().value<Interaction>();
                QVERIFY(event.kind != Interaction::Focus && event.kind != Interaction::Hover);
            }
        } else {
            QVERIFY(! feedback.values().focused && ! feedback.values().hovered);
        }
        if (keyboard)
            key(QEvent::KeyRelease);
        else
            pointer(QEvent::MouseButtonRelease);
        QVERIFY(requests.isEmpty() && secondRequests.isEmpty());
        if (first) {
            first->setEnabled(true);
            first->setVisible(true);
            QVERIFY(first->isVisible() && first->isEnabled());
        }
        item.click();
        QCOMPARE(requests.size(), 1);
        QCOMPARE(secondRequests.size(), change == 0 ? 1 : 0);
    }

    void navigationReindex_data() {
        QTest::addColumn<QByteArray>("kind");
        QTest::addColumn<bool>("modal");
        for (const auto& kind : { QByteArray("NavigationRail"), QByteArray("StandardDrawer") })
            for (bool modal : { false, true })
                QTest::newRow(qPrintable(QString("%1-%2").arg(QString::fromUtf8(kind)).arg(modal)))
                    << kind << modal;
    }

    void navigationReindex() {
        QFETCH(QByteArray, kind);
        QFETCH(bool, modal);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                id: owner
                property bool modal: false
                property int selected: 1
                width: modal ? 840 : 1280; height: 500
                MD.MProp.size: MD.ThemeSize { width: owner.width; duration: 0 }
                ListModel {
                    id: rows
                    ListElement { name: "One" }
                    ListElement { name: "Two" }
                    ListElement { name: "Three" }
                }
                function moveFirst() { rows.move(0, 2, 1) }
                MD.)" + kind + R"( {
                    objectName: "navigation"
                    width: 220; height: 400
                    expanded: !owner.modal
                    header: null
                    model: rows
                    currentIndex: owner.selected
                    onCurrentIndexChangeRequested: index => owner.selected = index
                }
            }
        )",
                          QUrl("qrc:/tests/navigation-reindex.qml"));
        QQuickWindow             window;
        std::unique_ptr<QObject> object(
            component.createWithInitialProperties({ { "modal", modal } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root       = qobject_cast<QQuickItem*>(object.get());
        auto* navigation = object->findChild<NavigationControl*>("navigation");
        QVERIFY(root && navigation);
        root->setParentItem(window.contentItem());
        window.resize(modal ? 840 : 1280, 500);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        if (modal) {
            QVERIFY(QMetaObject::invokeMethod(navigation, "open"));
            QTRY_VERIFY(navigation->property("drawerOpened").toBool());
        }
        NavigationItem* first = nullptr;
        QTRY_VERIFY(
            (first = qobject_cast<NavigationItem*>(navigationItem(window.contentItem(), "One"))));
        QCOMPARE(first->navigationIndex(), 0);
        QCOMPARE(first->navigation(), navigation);
        QSignalSpy requests(navigation, &NavigationControl::currentIndexChangeRequested);
        InteractionAccumulator feedback(this);
        feedback.setSource(first->interactionSource());
        first->forceActiveFocus(Qt::TabFocusReason);
        QTest::keyPress(&window, Qt::Key_Space);
        flushInteractions();
        QVERIFY(feedback.values().pressed);
        QPointer<NavigationItem> item(first);
        QVERIFY(QMetaObject::invokeMethod(object.get(), "moveFirst"));
        QTRY_VERIFY(item && item->navigationIndex() == 2);
        flushInteractions();
        QVERIFY(! feedback.values().pressed);
        QTest::keyRelease(&window, Qt::Key_Space);
        QVERIFY(requests.isEmpty());
        QCOMPARE(navigation->currentIndex(), 1);
        QCOMPARE(object->property("selected").toInt(), 1);
        item->click();
        QCOMPARE(requests.size(), 1);
        QCOMPARE(requests.last().first().toInt(), 2);
        QCOMPARE(navigation->currentIndex(), 2);
        QTRY_VERIFY(item->isSelected());
        object->setProperty("selected", 0);
        QCOMPARE(navigation->currentIndex(), 0);
        QTRY_VERIFY(! item->isSelected());
    }

    void navigationWindowDestroyed_data() {
        QTest::addColumn<QByteArray>("kind");
        QTest::newRow("rail") << QByteArray("NavigationRail");
        QTest::newRow("drawer") << QByteArray("StandardDrawer");
    }

    void navigationWindowDestroyed() {
        QFETCH(QByteArray, kind);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.)" + kind + R"( {
                width: 220; height: 400
                MD.MProp.size: MD.ThemeSize { width: 840; duration: 0 }
                header: null
                model: [{name: "One"}]
            }
        )",
                          QUrl("qrc:/tests/navigation-window-destroyed.qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(root);
        auto window = std::make_unique<QQuickWindow>();
        window->resize(840, 500);
        root->setParentItem(window->contentItem());
        window->show();
        QVERIFY(QTest::qWaitForWindowExposed(window.get()));
        auto* popup = root->findChild<Popup*>();
        QVERIFY(popup);
        QVERIFY(QMetaObject::invokeMethod(root, "open"));
        QVERIFY(popup->entering());
        window.reset();
        QVERIFY(! root->window());
        QVERIFY(! popup->isVisible());
        object.reset();
    }

    void cancelled_data() {
        QTest::addColumn<int>("kind");
        QTest::addColumn<int>("change");
        for (int kind = 0; kind < 3; ++kind)
            for (int change = 0; change < 3; ++change)
                QTest::newRow(qPrintable(QString("%1-%2").arg(kind).arg(change))) << kind << change;
    }

    void cancelled() {
        QFETCH(int, kind);
        QFETCH(int, change);
        auto control = selectionFor(kind);
        control->setSize({ 100, 50 });
        InteractionAccumulator feedback(this);
        feedback.setSource(control->interactionSource());
        QSignalSpy  requests(control.get(), &SelectionButton::selectionRequested);
        QMouseEvent press(QEvent::MouseButtonPress,
                          { 20, 20 },
                          { 20, 20 },
                          Qt::LeftButton,
                          Qt::LeftButton,
                          Qt::NoModifier);
        QCoreApplication::sendEvent(control.get(), &press);
        flushInteractions();
        QVERIFY(feedback.values().pressed);
        InteractionSource replacement;
        if (change == 0)
            control->setEnabled(false);
        else if (change == 1)
            control->setVisible(false);
        else
            control->setInteractionSource(&replacement);
        QMouseEvent release(QEvent::MouseButtonRelease,
                            { 20, 20 },
                            { 20, 20 },
                            Qt::LeftButton,
                            Qt::NoButton,
                            Qt::NoModifier);
        QCoreApplication::sendEvent(control.get(), &release);
        flushInteractions();
        QVERIFY(requests.isEmpty());
        QVERIFY(! control->isSelected() && ! feedback.values().pressed);
    }

    void tabRequests_data() {
        QTest::addColumn<int>("input");
        QTest::addColumn<bool>("accept");
        QTest::addColumn<bool>("rtl");
        for (int input = 0; input < 5; ++input)
            for (bool accept : { false, true })
                for (bool rtl : { false, true })
                    QTest::newRow(qPrintable(QString("%1-%2-%3").arg(input).arg(accept).arg(rtl)))
                        << input << accept << rtl;
    }

    void tabRequests() {
        QFETCH(int, input);
        QFETCH(bool, accept);
        QFETCH(bool, rtl);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(tabSource, QUrl("qrc:/tests/tab-owner.qml"));
        std::unique_ptr<QObject> object(
            component.createWithInitialProperties({ { "accept", accept } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* bar = qobject_cast<TabBar*>(object.get());
        QVERIFY(bar);
        QVERIFY(bar->findChildren<ButtonGroup*>().isEmpty());
        bar->setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
        QQuickWindow window;
        window.resize(360, 120);
        bar->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* first  = bar->itemAt(0);
        auto* second = bar->itemAt(1);
        QVERIFY(first && second);
        QTRY_VERIFY(second->x() > 0 && second->width() > 0);
        const auto activate = [&] {
            if (input == 0)
                second->click();
            else if (input == 1)
                QTest::mouseClick(&window,
                                  Qt::LeftButton,
                                  Qt::NoModifier,
                                  second->mapToScene({ second->width() / 2, 24 }).toPoint());
            else if (input == 2) {
                first->forceActiveFocus(Qt::TabFocusReason);
                QTest::keyClick(&window, rtl ? Qt::Key_Left : Qt::Key_Right);
            } else if (input == 3)
                bar->requestCurrentIndex(1);
            else
                bar->incrementCurrentIndex();
        };
        activate();
        QCOMPARE(bar->property("requests").toInt(), 1);
        QCOMPARE(bar->property("lastRequest").toInt(), 1);
        QCOMPARE(bar->currentIndex(), accept ? 1 : 0);
        QCOMPARE(bar->currentItem(), accept ? second : first);
        for (int i = 0; i < 3; ++i)
            QTRY_COMPARE(bar->itemAt(i)->isSelected(), i == bar->currentIndex());
        if (input == 2) QTRY_VERIFY((accept ? second : first)->hasActiveFocus());
        if (! accept) {
            activate();
            QCOMPARE(bar->property("requests").toInt(), 2);
            QCOMPARE(bar->currentIndex(), 0);
        }
        bar->setProperty("committedIndex", 2);
        QCOMPARE(bar->currentIndex(), 2);
        QTRY_VERIFY(bar->itemAt(2)->isSelected());
    }

    void tabRequestReentry() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(tabSource, QUrl("qrc:/tests/tab-request-reentry.qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* bar = qobject_cast<TabBar*>(object.get());
        QVERIFY(bar);
        QSignalSpy changes(bar, &Container::currentIndexChanged);
        connect(bar, &TabBar::currentIndexChangeRequested, this, [bar](int index) {
            if (index == 1)
                bar->requestCurrentIndex(2);
            else
                bar->setProperty("committedIndex", index);
        });
        bar->incrementCurrentIndex();
        QCOMPARE(bar->property("requests").toInt(), 2);
        QCOMPARE(bar->property("lastRequest").toInt(), 2);
        QCOMPARE(changes.size(), 1);
        QCOMPARE(bar->currentIndex(), 2);
        QCOMPARE(bar->currentItem(), bar->itemAt(2));
        QTRY_VERIFY(bar->itemAt(2)->isSelected());
    }

    void tabStructureKeepsInputIndex() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(tabSource, QUrl("qrc:/tests/tab-structure.qml"));
        std::unique_ptr<QObject> object(
            component.createWithInitialProperties({ { "accept", true }, { "committedIndex", 1 } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* bar = qobject_cast<TabBar*>(object.get());
        QVERIFY(bar);
        auto* first  = bar->itemAt(0);
        auto* second = bar->itemAt(1);
        bar->moveItem(1, 0);
        QCOMPARE(bar->currentIndex(), 1);
        QCOMPARE(bar->currentItem(), first);
        QCOMPARE(bar->property("committedIndex").toInt(), 1);
        second->click();
        QCOMPARE(bar->currentIndex(), 0);
        QCOMPARE(bar->currentItem(), second);
        bar->setProperty("committedIndex", 2);
        std::unique_ptr<QQuickItem> removed(bar->takeItem(2));
        QCOMPARE(bar->currentIndex(), 2);
        QCOMPARE(bar->currentItem(), nullptr);
        QCOMPARE(bar->property("committedIndex").toInt(), 2);
        auto* replacement = new TabButton;
        replacement->setParent(bar);
        bar->addItem(replacement);
        QCOMPARE(bar->currentIndex(), 2);
        QCOMPARE(bar->currentItem(), replacement);
        const int requests = bar->property("requests").toInt();
        static_cast<TabButton*>(removed.get())->click();
        QCOMPARE(bar->property("requests").toInt(), requests);
    }

    void ordinaryContainerRetainsItem() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.Container {
                Rectangle { width: 20; height: 20 }
                Rectangle { width: 20; height: 20 }
                Rectangle { width: 20; height: 20 }
            }
        )",
                          QUrl("qrc:/tests/container-selection.qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* container = qobject_cast<Container*>(object.get());
        QVERIFY(container);
        container->setCurrentIndex(1);
        auto* selected = container->currentItem();
        container->moveItem(1, 0);
        QCOMPARE(container->currentIndex(), 0);
        QCOMPARE(container->currentItem(), selected);
        container->setCurrentIndex(99);
        QCOMPARE(container->currentIndex(), 2);
        QCOMPARE(container->currentItem(), container->itemAt(2));
    }

    void tabRequestValidation() {
        QQuickWindow window;
        TabBar       bar(window.contentItem());
        TabButton    first, second;
        window.resize(360, 120);
        bar.setSize({ 360, 48 });
        bar.addItem(&first);
        bar.addItem(&second);
        bar.setCurrentIndex(0);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QSignalSpy requests(&bar, &TabBar::currentIndexChangeRequested);
        bar.requestCurrentIndex(-2);
        bar.requestCurrentIndex(2);
        second.setEnabled(false);
        bar.requestCurrentIndex(1);
        second.setEnabled(true);
        second.setVisible(false);
        bar.requestCurrentIndex(1);
        bar.setEnabled(false);
        bar.requestCurrentIndex(-1);
        bar.setEnabled(true);
        bar.setVisible(false);
        bar.requestCurrentIndex(0);
        bar.setVisible(true);
        QVERIFY(requests.isEmpty());
        bar.requestCurrentIndex(-1);
        bar.requestCurrentIndex(0);
        QCOMPARE(requests.size(), 2);
        QCOMPARE(bar.currentIndex(), 0);
    }

    void deleteInRequest_data() {
        QTest::addColumn<int>("kind");
        for (int kind = 0; kind < 3; ++kind)
            QTest::newRow(qPrintable(QString::number(kind))) << kind;
    }

    void deleteInRequest() {
        QFETCH(int, kind);
        auto                      control = selectionFor(kind);
        QPointer<SelectionButton> guard(control.get());
        connect(control.get(), &SelectionButton::selectionRequested, this, [&] {
            control.reset();
        });
        control->click();
        QVERIFY(! guard);
    }

    void tabDeletedInRequest_data() {
        QTest::addColumn<bool>("navigate");
        QTest::newRow("button") << false;
        QTest::newRow("navigation") << true;
    }

    void tabDeletedInRequest() {
        QFETCH(bool, navigate);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(tabSource, QUrl("qrc:/tests/tab-deletion.qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* bar = qobject_cast<TabBar*>(object.get());
        QVERIFY(bar);
        QPointer<TabButton> second(bar->itemAt(1));
        connect(bar, &TabBar::currentIndexChangeRequested, this, [&] {
            object.reset();
        });
        if (navigate)
            bar->incrementCurrentIndex();
        else
            second->click();
        QVERIFY(! object && ! second);
    }

    void tabSelection() {
        QProperty<bool> source(false);
        TabBar          bar;
        TabButton       a, b;
        bar.addItem(&a);
        bar.addItem(&b);
        a.bindableSelected().setBinding([&] {
            return source.value();
        });
        b.bindableSelected().setBinding([&] {
            return ! source.value();
        });
        bar.setCurrentIndex(0);
        bar.setCurrentIndex(1);
        QVERIFY(! a.isSelected() && b.isSelected());
        bar.setCurrentIndex(0);
        QVERIFY(! a.isSelected() && b.isSelected());
        source = true;
        QVERIFY(a.isSelected() && ! b.isSelected());
        QVERIFY(a.bindableSelected().hasBinding());
        QVERIFY(b.bindableSelected().hasBinding());
    }
};

int run_selection_requests(int argc, char** argv) {
    SelectionRequestsTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "selection_requests.moc"
