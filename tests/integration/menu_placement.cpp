#include <QtTest>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlIncubator>
#include <QQuickItem>
#include <QQuickWindow>
#include <QtQuick/private/qquicktranslate_p.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include <memory>
#include "qml_material/control/menu.hpp"
#include "qml_material/control/action.hpp"
#include "qml_material/control/overlay.hpp"
#include "qml_material/control/panel.hpp"
#include "qml_material/control/popup_presenter.hpp"
#include "qml_material/layout/toolbar_layout.hpp"
#include "../../src/control/popup_placement.hpp"

using namespace qml_material;

class MenuPlacementTest : public QObject {
    Q_OBJECT
    QQmlIncubationController controller;
    QQmlEngine               engine;
    QQuickWindow             window;
    std::unique_ptr<QObject> root;

    bool create(const QByteArray& body) {
        QQmlComponent component(&engine);
        component.setData("pragma ComponentBehavior: Bound\nimport QtQuick\nimport Qcm.Material as "
                          "MD\nItem { id: root; width: 640; height: 480; " +
                              body + " }",
                          QUrl("qrc:/tests/menu-placement.qml"));
        root.reset(component.create());
        if (! root) qWarning() << component.errors();
        if (! root) return false;
        auto item = qobject_cast<QQuickItem*>(root.get());
        item->setParentItem(window.contentItem());
        window.resize(640, 480);
        window.show();
        QCoreApplication::processEvents();
        return true;
    }
    Menu*       menu() { return root->findChild<Menu*>("menu"); }
    QQuickItem* anchor() { return root->findChild<QQuickItem*>("anchor"); }
private slots:
    void initTestCase() { engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH)); }
    void cleanup() {
        engine.setIncubationController(nullptr);
        root.reset();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
    void menuDefaults() {
        Menu menu;
        QVERIFY(menu.modal());
        QVERIFY(menu.focus());
        QVERIFY(! menu.dim());
        Popup popup;
        QVERIFY(! popup.modal());
        QVERIFY(! popup.focus());
        QVERIFY(! popup.dim());
    }
    void openerClick_data() {
        QTest::addColumn<bool>("anchored");
        QTest::addColumn<bool>("independent");
        QTest::addColumn<bool>("releaseClose");
        QTest::addColumn<bool>("deferred");
        QTest::addColumn<bool>("touchInput");
        QTest::newRow("nested") << false << false << false << false << false;
        QTest::newRow("anchored") << true << false << false << false << false;
        QTest::newRow("independent") << false << true << false << false << false;
        QTest::newRow("release") << true << false << true << false << false;
        QTest::newRow("closing") << false << false << false << true << false;
        QTest::newRow("touch") << false << false << false << false << true;
        QTest::newRow("touch-independent") << false << true << false << false << true;
        QTest::newRow("touch-release") << true << false << true << false << true;
    }
    void openerClick() {
        QFETCH(bool, anchored);
        QFETCH(bool, independent);
        QFETCH(bool, releaseClose);
        QFETCH(bool, deferred);
        QFETCH(bool, touchInput);
        QVERIFY(create(R"(
            property int clicks: 0
            MD.Button {
                id: button; objectName: "anchor"; x: 40; y: 40; width: 120; height: 48
                text: "Menu"
                onClicked: { root.clicks++; menu.open() }
                MD.Menu {
                    id: menu; objectName: "menu"; y: button.height; width: 180
                    enter: null; exit: null
                    MD.MenuItem { text: "First" }
                }
            }
        )"));
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto m = menu();
        if (independent) {
            m->setParentItem(qobject_cast<QQuickItem*>(root.get()));
            m->setX(40);
            m->setY(100);
        }
        if (anchored) {
            m->setParentItem(qobject_cast<QQuickItem*>(root.get()));
            m->setAnchorItem(anchor());
        }
        if (releaseClose) m->setClosePolicy(Popup::CloseOnReleaseOutside);
        m->setDeferredCompletion(deferred);
        QSignalSpy  shown(m, &Popup::aboutToShow);
        const auto  point  = anchor()->mapToScene({ 60, 24 }).toPoint();
        static auto device = QTest::createTouchDevice();
        auto        touch  = QTest::touchEvent(&window, device, false);
        auto        send   = [&](bool press) {
            if (touchInput) {
                if (press)
                    touch.press(0, point, &window).commit();
                else
                    touch.release(0, point, &window).commit();
                QQuickWindowPrivate::get(&window)
                    ->deliveryAgentPrivate()
                    ->flushFrameSynchronousEvents(&window);
            } else if (press) {
                QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, point);
            } else {
                QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, point);
            }
        };
        send(true);
        send(false);
        if (deferred) m->completeEnter();
        QVERIFY(m->isOpened());
        QCOMPARE(shown.count(), 1);
        send(true);
        if (releaseClose)
            QVERIFY(m->isOpened());
        else if (deferred)
            QVERIFY(m->closing());
        else
            QVERIFY(! m->isVisible());
        send(false);
        QCOMPARE(shown.count(), 1);
        QCOMPARE(root->property("clicks").toInt(), 1);
        if (deferred) {
            QVERIFY(m->closing());
            // The opener remains blocked while the menu is still exiting.
            send(true);
            send(false);
            QCOMPARE(shown.count(), 1);
            m->completeExit();
        }
        QVERIFY(! m->isVisible());
        send(true);
        send(false);
        QCOMPARE(shown.count(), 2);
        QCOMPARE(root->property("clicks").toInt(), 2);
    }
    void outsideClick_data() {
        QTest::addColumn<bool>("nonModal");
        QTest::newRow("default-modal") << false;
        QTest::newRow("explicit-non-modal") << true;
    }
    void outsideClick() {
        QFETCH(bool, nonModal);
        QVERIFY(create(R"(
            property int clicks: 0
            MD.Button { id: button; x: 40; y: 40; text: "Other"; onClicked: root.clicks++ }
            MD.Menu {
                objectName: "menu"; x: 240; y: 40; width: 180; enter: null; exit: null
                MD.MenuItem { text: "First" }
            }
        )"));
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        if (nonModal) menu()->setModal(false);
        menu()->open();
        QVERIFY(menu()->isOpened());
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, { 60, 60 });
        QVERIFY(! menu()->isVisible());
        QCOMPARE(root->property("clicks").toInt(), nonModal ? 1 : 0);
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, { 60, 60 });
        QCOMPARE(root->property("clicks").toInt(), nonModal ? 2 : 1);
    }
    void openerClosePolicy_data() {
        QTest::addColumn<int>("policy");
        QTest::addColumn<bool>("modal");
        QTest::newRow("non-modal-manual") << int(Popup::NoAutoClose) << false;
        QTest::newRow("non-modal-outside-parent") << int(Popup::CloseOnPressOutsideParent) << false;
        QTest::newRow("non-modal-release-outside-parent")
            << int(Popup::CloseOnReleaseOutsideParent) << false;
        QTest::newRow("modal-manual") << int(Popup::NoAutoClose) << true;
        QTest::newRow("modal-outside-parent") << int(Popup::CloseOnPressOutsideParent) << true;
        QTest::newRow("modal-release-outside-parent")
            << int(Popup::CloseOnReleaseOutsideParent) << true;
    }
    void openerClosePolicy() {
        QFETCH(int, policy);
        QFETCH(bool, modal);
        QVERIFY(create(R"(
            property int clicks: 0
            MD.Button {
                objectName: "anchor"; x: 40; y: 40; width: 120; height: 48
                onClicked: { root.clicks++; if (menu.visible) menu.close(); else menu.open() }
                MD.Menu {
                    id: menu; objectName: "menu"; y: parent.height; width: 180
                    enter: null; exit: null
                    MD.MenuItem { text: "First" }
                }
            }
        )"));
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        menu()->setClosePolicy(Popup::ClosePolicy(policy));
        menu()->setModal(modal);
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, { 100, 64 });
        QVERIFY(menu()->isOpened());
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, { 100, 64 });
        QVERIFY(menu()->isOpened());
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, { 100, 64 });
        QCOMPARE(menu()->isOpened(), modal);
        QCOMPARE(root->property("clicks").toInt(), modal ? 1 : 2);
    }
    void splitButtonOpener() {
        QVERIFY(create(R"(
            property int clicks: 0
            MD.SplitButton {
                objectName: "split"; x: 40; y: 40; text: "Action"
                onClicked: root.clicks++
                menu: MD.Menu {
                    objectName: "menu"; width: 180; enter: null; exit: null
                    MD.MenuItem { text: "First" }
                }
            }
        )"));
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto       split = root->findChild<QQuickItem*>("split");
        const auto point =
            split->mapToScene({ split->width() - 12, split->height() / 2 }).toPoint();
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, point);
        QVERIFY(menu()->isOpened());
        QSignalSpy shown(menu(), &Popup::aboutToShow);
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, point);
        QVERIFY(! menu()->isVisible());
        QCOMPARE(shown.count(), 0);
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, point);
        QVERIFY(menu()->isOpened());
        QTest::mouseClick(&window,
                          Qt::LeftButton,
                          Qt::NoModifier,
                          split->mapToScene({ 16, split->height() / 2 }).toPoint());
        QVERIFY(! menu()->isVisible());
        QCOMPARE(root->property("clicks").toInt(), 0);
        QTest::mouseClick(&window,
                          Qt::LeftButton,
                          Qt::NoModifier,
                          split->mapToScene({ 16, split->height() / 2 }).toPoint());
        QCOMPARE(root->property("clicks").toInt(), 1);
    }
    void submenuOpenerClick() {
        QVERIFY(create(R"(
            property int clicks: 0
            MD.Button {
                id: button; objectName: "anchor"; x: 40; y: 40; width: 120; height: 48
                onClicked: { root.clicks++; menu.open() }
                MD.Menu {
                    id: menu; objectName: "menu"; y: button.height; width: 180
                    enter: null; exit: null
                    MD.MenuItem { objectName: "childAnchor"; text: "Submenu" }
                }
            }
            MD.Menu {
                objectName: "child"; width: 180; enter: null; exit: null
                MD.MenuItem { text: "Child" }
            }
        )"));
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        const QPoint point(100, 64);
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, point);
        auto child = root->findChild<Menu*>("child");
        menu()->openSubMenu(child, root->findChild<QQuickItem*>("childAnchor"));
        QVERIFY(child->isOpened());
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, point);
        QVERIFY(! menu()->isVisible());
        QVERIFY(! child->isVisible());
        QCOMPARE(root->property("clicks").toInt(), 1);
    }
    void comboBoxOpenerClick() {
        QVERIFY(create(R"(
            MD.ComboBox {
                id: combo; x: 40; y: 40; width: 180; model: ["First", "Second"]
                popup: MD.Menu {
                    objectName: "menu"; parent: combo; modal: false
                    y: combo.height; width: combo.width; enter: null; exit: null; focus: false
                    closePolicy: MD.PopupBase.CloseOnPressOutsideParent
                    MD.MenuItem { text: "First" }
                    MD.MenuItem { text: "Second" }
                }
            }
        )"));
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        const QPoint point(200, 64);
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, point);
        QVERIFY(menu()->isOpened());
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, point);
        QVERIFY(! menu()->isVisible());
    }
    void cascadeParentInteraction() {
        QVERIFY(create(R"(
            property int parentClicks: 0
            property int childClicks: 0
            MD.Menu {
                id: parentMenu; objectName: "menu"; x: 100; y: 100; width: 180
                enter: null; exit: null
                MD.MenuItem { text: "Submenu"; subMenu: childMenu }
                MD.MenuItem {
                    action: MD.Action {
                        text: "Parent action"; closeMenu: false
                        onTriggered: root.parentClicks++
                    }
                }
            }
            MD.Menu {
                id: childMenu; objectName: "child"; width: 180; enter: null; exit: null
                MD.MenuItem { text: "Child action"; onTriggered: root.childClicks++ }
            }
        )"));
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto parent = menu();
        auto child  = root->findChild<Menu*>("child");
        parent->open();
        parent->contentItem()->ensurePolished();
        parent->openSubMenu(child, parent->itemAt(0));
        child->contentItem()->ensurePolished();
        QVERIFY(parent->modal());
        QVERIFY(child->modal());
        QVERIFY(child->isOpened());
        auto       action = parent->itemAt(1);
        const auto point =
            action->mapToScene({ action->width() / 2, action->height() / 2 }).toPoint();
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, point);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, point);
        QCOMPARE(root->property("parentClicks").toInt(), 1);
        QVERIFY(parent->isOpened());
        QVERIFY(! child->isVisible());

        parent->openSubMenu(child, parent->itemAt(0));
        child->contentItem()->ensurePolished();
        QVERIFY(child->isOpened());
        action = child->itemAt(0);
        QTest::mouseClick(
            &window,
            Qt::LeftButton,
            Qt::NoModifier,
            action->mapToScene({ action->width() / 2, action->height() / 2 }).toPoint());
        QCOMPARE(root->property("childClicks").toInt(), 1);
        QVERIFY(! parent->isVisible());
        QVERIFY(! child->isVisible());
    }
    void destructiveHideCallback() {
        QVERIFY(create(R"(
            property int clicks: 0
            MD.Button { x: 40; y: 40; text: "Other"; onClicked: root.clicks++ }
            MD.Menu {
                objectName: "menu"; x: 240; y: 40; width: 180; enter: null; exit: null
                MD.MenuItem { text: "First" }
            }
        )"));
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QPointer<Menu> m = menu();
        connect(m, &Popup::aboutToHide, m, [&] {
            delete m.data();
        });
        m->open();
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, { 60, 60 });
        QVERIFY(! m);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, { 60, 60 });
        QCOMPARE(root->property("clicks").toInt(), 0);
    }
    void geometryMatrix() {
        const QRectF bounds(8, 8, 624, 464);
        for (bool rtl : { false, true })
            for (auto side : { Popup::Below, Popup::Above, Popup::Leading, Popup::Trailing })
                for (auto alignment : { Popup::Start, Popup::Center, Popup::End })
                    for (auto rect : { QRectF(10, 10, 40, 40),
                                       QRectF(590, 10, 40, 40),
                                       QRectF(10, 420, 40, 40),
                                       QRectF(590, 420, 40, 40),
                                       QRectF(300, 220, 40, 40) }) {
                        const auto result =
                            placePopup(rect, { 180, 140 }, bounds, side, alignment, 4, rtl);
                        QVERIFY(result);
                        QVERIFY(bounds.contains(result->rect));
                        QCOMPARE(result->rect.size(), QSizeF(180, 140));
                        const auto again =
                            placePopup(rect, { 180, 140 }, bounds, side, alignment, 4, rtl);
                        QCOMPARE(again->rect, result->rect);
                        QCOMPARE(again->placement, result->placement);
                    }
        auto result = placePopup(
            { 550, 40, 40, 40 }, { 180, 140 }, bounds, Popup::Below, Popup::End, 4, false);
        QVERIFY(result);
        QCOMPARE(result->rect, QRectF(410, 84, 180, 140));
        QCOMPARE(result->origin, QQuickItem::TopRight);
        result = placePopup(
            { 200, 328, 40, 40 }, { 180, 100 }, bounds, Popup::Below, Popup::Start, 4, false);
        QCOMPARE(result->placement, Popup::Below);
        QCOMPARE(result->rect.bottom(), qreal(472));
        result = placePopup(
            { 200, 200, 40, 40 }, { 900, 900 }, bounds, Popup::Below, Popup::Start, 4, false);
        QCOMPARE(result->rect.size(), QSizeF(624, 228));
        QVERIFY(bounds.contains(result->rect));
        QVERIFY(
            ! placePopup({ 0, 0, 10, 10 }, { 100, 100 }, {}, Popup::Below, Popup::Start, 0, false));
        QVERIFY(! placePopup(
            { 0, 0, 10, 10 }, { NAN, 100 }, bounds, Popup::Below, Popup::Start, 0, false));
    }
    void initialLayoutAndMovement() {
        QVERIFY(create(R"(
            Item { id: container; objectName: "container"; x: 40; y: 30
                transform: Translate { id: translation; objectName: "translation" }
                Rectangle { id: anchor; objectName: "anchor"; x: 500; y: 20; width: 40; height: 40 }
            }
            MD.Menu { objectName: "menu"; anchorItem: anchor; anchorAlignment: MD.PopupBase.End; enter: null; exit: null
                MD.MenuItem { text: "First" }
                MD.MenuItem { text: "Second" }
                MD.MenuItem { text: "Third" }
            }
        )"));
        auto m = menu();
        auto a = anchor();
        QCOMPARE(m->parentItem(), qobject_cast<QQuickItem*>(root.get()));
        QObject* owner = m->parent();
        QRectF   shown;
        connect(m, &Popup::aboutToShow, m, [&] {
            shown = m->presentationRect();
        });
        m->open();
        QVERIFY(m->isOpened());
        QCOMPARE(m->count(), 3);
        QVERIFY(shown.height() > 100);
        QCOMPARE(shown.right(), a->mapToScene({ a->width(), 0 }).x());
        QCOMPARE(shown.top(), a->mapToScene({ 0, a->height() }).y() + 4);
        QCOMPARE(m->parent(), owner);
        QCOMPARE(m->transformOrigin(), QQuickItem::TopRight);
        auto translation = root->findChild<QQuickTranslate*>("translation");
        translation->setY(75);
        QTRY_COMPARE(m->presentationRect().top(), shown.top() + 75);
        auto container = root->findChild<QQuickItem*>("container");
        container->setScale(0.8);
        QTRY_COMPARE(m->presentationRect().top(), a->mapToScene({ 0, a->height() }).y() + 4);
        a->setVisible(false);
        QTRY_VERIFY(! m->isVisible());
        a->setVisible(true);
        QCoreApplication::processEvents();
        QVERIFY(! m->isVisible());
        m->open();
        QVERIFY(m->isOpened());
        a->setParentItem(nullptr);
        QTRY_VERIFY(! m->isVisible());
    }
    void sizeRecoveryAndKeyboard() {
        QVERIFY(create(R"(
            Rectangle { id: anchor; objectName: "anchor"; x: 580; y: 400; width: 40; height: 40 }
            MD.Menu { objectName: "menu"; anchorItem: anchor; enter: null; exit: null
                model: 20
                contentDelegate: MD.MenuItem { required property int index; text: "Entry " + index }
            }
        )"));
        auto m = menu();
        m->open();
        QVERIFY(m->isOpened());
        QCOMPARE(m->resolvedPlacement(), Popup::Above);
        QVERIFY(m->height() > m->surfaceItem()->height());
        QVERIFY(m->contentItem()->property("interactive").toBool());
        QVERIFY(QRectF(8, 8, 624, 464).contains(m->presentationRect()));
        for (int i = 0; i < 20; ++i) QTest::keyClick(&window, Qt::Key_Down);
        QCOMPARE(m->currentIndex(), 19);
        QTRY_VERIFY(m->contentItem()->property("contentY").toReal() > 0);
        const auto requested = m->height();
        window.resize(640, 2400);
        QTRY_COMPARE(m->surfaceItem()->height(), requested);
        QCOMPARE(m->height(), requested);
        window.resize(150, 200);
        QTRY_VERIFY(m->presentationRect().right() <= 142);
        QVERIFY(m->surfaceItem()->width() <= 134);
    }
    void actionMenuFirstOpen() {
        QVERIFY(create(R"(
            Rectangle { id: anchor; objectName: "anchor"; x: 500; y: 10; width: 40; height: 40 }
            MD.ActionMenu { objectName: "menu"; anchorItem: anchor; enter: null; exit: null
                actions: [MD.Action { text: "One" },MD.Action { text: "Two" },MD.Action { text: "Three" }]
            }
        )"));
        QRectF first;
        connect(menu(), &Popup::aboutToShow, menu(), [&] {
            first = menu()->presentationRect();
        });
        menu()->open();
        QCOMPARE(menu()->count(), 3);
        QVERIFY2(first.height() > 100, qPrintable(QString::number(first.height())));
        QCoreApplication::processEvents();
        QCOMPARE(menu()->presentationRect(), first);
    }
    void invalidAndReplacement() {
        QVERIFY(create(R"(
            Rectangle { id: anchor; objectName: "anchor"; width: 40; height: 40 }
            Rectangle { id: other; objectName: "other"; x: 300; width: 40; height: 40 }
            MD.Menu { objectName: "menu"; anchorItem: anchor; width: 180; height: 100; enter: null; exit: null }
        )"));
        auto m = menu();
        m->open();
        auto other = root->findChild<QQuickItem*>("other");
        m->openFor(other);
        QCOMPARE(m->presentationRect().left(), qreal(300));
        m->setAnchorItem(nullptr);
        QVERIFY(! m->isVisible());
        m->openFor(other);
        other->setVisible(false);
        other->setVisible(true);
        QTRY_VERIFY(! m->isVisible());
        m->openFor(other);
        anchor()->setVisible(false);
        QCoreApplication::processEvents();
        QVERIFY(m->isOpened());
        delete other;
        QTRY_VERIFY(! m->isVisible());
        QSignalSpy rejection(m, &Popup::openRejected);
        m->open();
        QCOMPARE(rejection.size(), 1);
        QQuickWindow otherWindow;
        auto         otherAnchor = new QQuickItem(otherWindow.contentItem());
        m->openFor(otherAnchor);
        QCOMPARE(rejection.size(), 2);
    }
    void submenuAndManual() {
        QVERIFY(create(R"(
            Rectangle { id: anchor; objectName: "anchor"; x: 580; y: 20; width: 40; height: 40 }
            MD.Menu { objectName: "menu"; anchorItem: anchor; width: 180; enter: null; exit: null
                MD.MenuItem { text: "Submenu"; subMenu: MD.Menu { objectName: "child"; enter: null; exit: null; MD.MenuItem {text: "Action"} } }
            }
            MD.Menu { objectName: "manual"; x: 35; y: 65; width: 180; height: 100; enter: null; exit: null }
        )"));
        auto m = menu();
        m->open();
        auto child = root->findChild<Menu*>("child");
        QVERIFY(child);
        m->openSubMenu(child, m->itemAt(0));
        QVERIFY(child->isOpened());
        QCOMPARE(child->resolvedPlacement(), Popup::Leading);
        QCOMPARE(child->parentMenu(), m);
        child->dismiss();
        QVERIFY(! m->isVisible());
        QVERIFY(! child->isVisible());
        auto manual = root->findChild<Menu*>("manual");
        manual->open();
        QCOMPARE(manual->presentationRect(), QRectF(35, 65, 180, 100));
        manual->popup(100, 200);
        QCOMPARE(manual->presentationRect(), QRectF(100, 200, 180, 100));
    }
    void presenterWaitingCancellation() {
        QVERIFY(create(R"(
            Rectangle { id: anchor; objectName: "anchor"; x: 500; width: 40; height: 40 }
            MD.PopupPresenter {objectName: "presenter"; host: root; incubationMode: MD.Pool.Synchronous}
            property Component source: Component { MD.Menu { MD.Presentation.ready: false; enter: null; exit: null; MD.MenuItem { text: "One" } } }
        )"));
        auto presenter = root->findChild<PopupPresenter*>("presenter");
        auto p         = presenter->presentMenu(root->property("source"), anchor());
        QCOMPARE(p->status(), PopupPresentation::WaitingForReady);
        QSignalSpy cancelled(p, &PopupPresentation::cancelled);
        anchor()->setVisible(false);
        QCOMPARE(p->status(), PopupPresentation::Cancelled);
        QCOMPARE(cancelled.size(), 1);
        QCoreApplication::processEvents();
        QCOMPARE(cancelled.size(), 1);
    }
    void standaloneSubmenu() {
        QVERIFY(create(R"(
            MD.MenuItem {objectName: "standalone"; x: 580; y: 20; width: 40; height: 40
                subMenu: MD.Menu {objectName: "child"; enter: null; exit: null; MD.MenuItem {text: "Entry"}}
            }
        )"));
        auto item  = root->findChild<QQuickItem*>("standalone");
        auto child = root->findChild<Menu*>("child");
        QVERIFY(QMetaObject::invokeMethod(item, "click"));
        QVERIFY(child->isOpened());
        QCOMPARE(child->anchorItem(), item);
        QCOMPARE(child->resolvedPlacement(), Popup::Leading);
    }
    void directionAndRotation() {
        QVERIFY(create(R"(
            MD.Control { id: scope; objectName: "scope"; anchors.fill: parent; layoutDirection: Qt.RightToLeft
                Rectangle {id: anchor; objectName: "anchor"; x: 300; y: 40; width: 40; height: 40}
                MD.Menu {objectName: "menu"; anchorItem: anchor; width: 180; height: 100; enter: null; exit: null}
            }
        )"));
        auto m = menu();
        m->open();
        QVERIFY(m->mirrored());
        QCOMPARE(m->presentationRect().left(), qreal(160));
        root->findChild<QQuickItem*>("scope")->setProperty("layoutDirection", Qt::LeftToRight);
        QTRY_COMPARE(m->presentationRect().left(), qreal(300));
        anchor()->setRotation(30);
        const auto rect = anchor()->mapRectToScene(anchor()->boundingRect());
        QTRY_COMPARE(m->presentationRect().top(), rect.bottom() + 4);
        QCOMPARE(m->presentationRect().left(), rect.left());
    }
    void presenterLoadingCancellation() {
        engine.setIncubationController(&controller);
        QVERIFY(create(R"(
            Rectangle { id: anchor; objectName: "anchor"; width: 40; height: 40 }
            MD.PopupPresenter {objectName: "presenter"; host: root; incubationMode: MD.Pool.Asynchronous}
            property Component source: Component { MD.Menu { enter: null; exit: null; MD.MenuItem { text: "One" } } }
        )"));
        auto presenter = root->findChild<PopupPresenter*>("presenter");
        auto p         = presenter->presentMenu(root->property("source"), anchor());
        QCOMPARE(p->status(), PopupPresentation::Loading);
        QSignalSpy cancelled(p, &PopupPresentation::cancelled);
        delete anchor();
        QCOMPARE(p->status(), PopupPresentation::Cancelled);
        controller.incubateFor(100);
        QCoreApplication::processEvents();
        QCOMPARE(cancelled.size(), 1);
        QCOMPARE(OverlayManager::get(&window)->childItems().size(), 0);
    }
    void destructiveShowCallback() {
        QVERIFY(create(R"(
            Rectangle { id: anchor; objectName: "anchor"; width: 40; height: 40 }
            MD.Menu { objectName: "menu"; anchorItem: anchor; width: 180; height: 100; enter: null; exit: null }
        )"));
        QPointer<Menu> m = menu();
        connect(m, &Popup::aboutToShow, m, [&] {
            delete m.data();
        });
        m->open();
        QVERIFY(! m);
        QCoreApplication::processEvents();
        QCOMPARE(OverlayManager::get(&window)->childItems().size(), 0);
    }
    void toolbarUsesActualButton() {
        QVERIFY(create(R"(
            MD.ActionToolBar {objectName: "toolbar"; x: 300; y: 50; width: 200; height: 48; maxShowActionNum: 1
                actions: [MD.Action {text: "First"},MD.Action {text: "Second"}]
            }
        )"));
        auto toolbar = root->findChild<QQuickItem*>("toolbar");
        auto layout  = toolbar->findChild<ToolBarLayout*>();
        QVERIFY(layout);
        QTRY_VERIFY(layout->moreButtonItem() && layout->moreButtonItem()->isVisible());
        auto button = layout->moreButtonItem();
        QVERIFY(QMetaObject::invokeMethod(button, "click"));
        Menu* popup    = nullptr;
        auto  findMenu = [&] {
            for (auto item : OverlayManager::get(&window)->childItems())
                if (auto candidate = qobject_cast<Menu*>(item->parent())) {
                    popup = candidate;
                    return true;
                }
            return false;
        };
        QTRY_VERIFY(findMenu());
        QCOMPARE(popup->anchorItem(), button);
        QTRY_VERIFY(popup->isOpened());
        QCOMPARE(popup->presentationRect().right(), button->mapToScene({ button->width(), 0 }).x());
    }
    void overflowActionKeepsAnchor() {
        QVERIFY(create(R"(
            property var nextMenu: null
            property bool fromMenuItem: false
            property Item selectedAnchor: null
            property string rejection: ""
            MD.Action {id: openAction; objectName: "openAction"; text: "Open menu"
                onTriggered: source => {
                    root.fromMenuItem = source instanceof MD.MenuItemBase;
                    const anchor = (source as MD.MenuItemBase)?.menu?.anchorItem ?? (source as Item);
                    root.selectedAnchor = anchor;
                    root.nextMenu = MD.Util.showPopup(nextSource, {anchorItem: anchor}, anchor);
                }
            }
            property Component nextSource: Component {
                MD.Menu {enter: null; exit: null
                    onOpenRejected: error => { root.rejection = error; }
                    MD.MenuItem {text: "Next"}
                }
            }
            MD.Action {id: firstAction; text: "First"}
            MD.Action {id: extraA; text: "Extra A"}
            MD.Action {id: extraB; text: "Extra B"}
            MD.Action {id: extraC; text: "Extra C"}
            MD.ActionToolBar {objectName: "toolbar"; width: 200; height: 48; maxShowActionNum: 1
                actions: [firstAction, openAction, extraA, extraB, extraC]
            }
        )"));
        auto toolbar = root->findChild<QQuickItem*>("toolbar");
        auto layout  = toolbar->findChild<ToolBarLayout*>();
        QTRY_VERIFY(layout->moreButtonItem() && layout->moreButtonItem()->isVisible());
        QVERIFY(QMetaObject::invokeMethod(layout->moreButtonItem(), "click"));
        QPointer<Menu> previous;
        auto           find = [&] {
            for (auto item : OverlayManager::get(&window)->childItems())
                if (auto m = qobject_cast<Menu*>(item->parent())) {
                    previous = m;
                    return true;
                }
            return false;
        };
        QTRY_VERIFY(find());
        QTRY_VERIFY(previous->isOpened());
        QCOMPARE(previous->count(), 4);
        MenuItem* trigger = nullptr;
        auto      action  = root->findChild<Action*>("openAction");
        for (int index = 0; index < previous->count(); ++index) {
            auto item = qobject_cast<MenuItem*>(previous->itemAt(index));
            QVERIFY(item);
            QCOMPARE(item->menu(), previous.data());
            if (item->action() == action) trigger = item;
        }
        QVERIFY(trigger);
        QVERIFY(QMetaObject::invokeMethod(trigger, "click"));
        QVERIFY(root->property("fromMenuItem").toBool());
        QCOMPARE(qvariant_cast<QQuickItem*>(root->property("selectedAnchor")),
                 layout->moreButtonItem());
        auto next = qvariant_cast<Menu*>(root->property("nextMenu"));
        QVERIFY2(next, qPrintable(root->property("rejection").toString()));
        QVERIFY2(next->isOpened(), qPrintable(root->property("rejection").toString()));
        QTRY_VERIFY(! previous || ! previous->isVisible());
        QVERIFY(next->isOpened());
    }
    void nativeItemOwnership() {
        Menu     first, second;
        MenuItem item;
        first.addItem(&item);
        QCOMPARE(item.menu(), &first);
        second.addItem(&item);
        QCOMPARE(first.count(), 0);
        QCOMPARE(second.count(), 1);
        QCOMPARE(item.menu(), &second);
        QCOMPARE(second.takeItem(0), &item);
        QCOMPARE(item.menu(), nullptr);
        QSignalSpy changed(&item, &MenuItem::subMenuChanged);
        auto       child = new Menu;
        item.setSubMenu(child);
        QCOMPARE(item.subMenu(), child);
        delete child;
        QCOMPARE(item.subMenu(), nullptr);
        QCOMPARE(changed.size(), 2);
    }
};
int run_menu_placement(int argc, char** argv) {
    MenuPlacementTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "menu_placement.moc"
