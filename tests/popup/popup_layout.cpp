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
#include "qml_material/control/popup.hpp"
#include "qml_material/control/dialog.hpp"
#include "qml_material/control/color_picker_button.hpp"
#include "qml_material/scrollable/flickable.hpp"

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

qml_material::ToolTipAttached* attachedToolTip(QObject* target) {
    return static_cast<qml_material::ToolTipAttached*>(
        qmlAttachedPropertiesObject<qml_material::ToolTip>(target, false));
}
} // namespace

class PopupLayoutTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        m_engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        m_window.setGeometry(0, 0, 800, 600);
        m_window.create();
    }

    void popupSurfaceRegion_data() {
        QTest::addColumn<bool>("dialog");
        QTest::newRow("popup") << false;
        QTest::newRow("dialog") << true;
    }

    void popupSurfaceRegion() {
        QFETCH(bool, dialog);
        std::unique_ptr<qml_material::Popup> popup;
        if (dialog)
            popup = std::make_unique<qml_material::Dialog>();
        else
            popup = std::make_unique<qml_material::Popup>();
        popup->setParentItem(m_window.contentItem());
        popup->setWidth(400);
        popup->setHeight(400);
        static_cast<QQmlParserStatus*>(popup.get())->componentComplete();
        popup->open();
        auto* surface = popup->surfaceItem();
        QVERIFY(surface->contains(QPointF(100, 100)));
        QVERIFY(! surface->contains(QPointF(500, 500)));
        QQuickItem container(surface);
        container.setPosition(QPointF(30, 40));
        container.setScale(2);
        auto hit = std::make_unique<QQuickItem>(&container);
        hit->setSize(QSizeF(50, 50));
        hit->setPosition(QPointF(10, 20));
        popup->setPopupItem(hit.get());
        auto center = [&] {
            return hit->mapToItem(surface, QPointF(25, 25));
        };
        QVERIFY(surface->contains(center()));
        QVERIFY(! surface->contains(QPointF(1, 1)));
        auto oldCenter = center();
        hit->setY(150);
        QVERIFY(! surface->contains(oldCenter));
        QVERIFY(surface->contains(center()));
        hit->setVisible(false);
        QVERIFY(! surface->contains(center()));
        hit->setVisible(true);
        QVERIFY(surface->contains(center()));
        QQuickItem replacement(surface);
        replacement.setSize(QSizeF(20, 20));
        popup->setPopupItem(&replacement);
        QVERIFY(surface->contains(QPointF(1, 1)));
        QVERIFY(! surface->contains(center()));
        popup->setPopupItem(hit.get());
        hit.reset();
        QCOMPARE(popup->popupItem(), surface);
        QVERIFY(surface->contains(QPointF(100, 100)));
        popup->close();
    }

    void dialogInRecycledDelegate_data() {
        QTest::addColumn<bool>("reuse");
        QTest::newRow("reused") << true;
        QTest::newRow("not-reused") << false;
    }

    void dialogInRecycledDelegate() {
        QFETCH(bool, reuse);
        QTest::failOnWarning(QRegularExpression(".*TypeError.*"));
        QQmlComponent component(&m_engine);
        component.setData(R"(
            pragma ComponentBehavior: Bound
            import QtQuick
            import Qcm.Material as MD
            Item {
                id: root
                width: 400; height: 500
                property int rows: 80
                property bool reuse: true
                property int pooled: 0
                property int reused: 0
                MD.BottomSheet {
                    objectName: "sheet"
                    parent: root
                    animationDuration: 0
                    preferredContentHeight: 400
                    MD.VerticalListView {
                        id: list
                        objectName: "list"
                        width: 360; height: 300
                        model: root.rows
                        reuseItems: root.reuse
                        delegate: Item {
                            required property int index
                            width: list.contentWidth; height: 64
                            ListView.onPooled: ++root.pooled
                            ListView.onReused: ++root.reused
                            MD.ColorPickerDialog {}
                        }
                    }
                }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(root);
        QVERIFY(root->setProperty("reuse", reuse));
        root->setParentItem(m_window.contentItem());
        auto* sheet = root->findChild<qml_material::Popup*>("sheet");
        auto* list  = root->findChild<QQuickItem*>("list");
        QVERIFY(sheet);
        QVERIFY(list);
        sheet->open();
        QTRY_VERIFY(sheet->isOpened());
        settle(root);
        for (int i = 1; i <= 8; ++i) {
            QVERIFY(list->setProperty("contentY", i * 320));
            settle(root);
        }
        if (reuse) {
            QVERIFY(root->property("pooled").toInt() > 0);
            QVERIFY(root->property("reused").toInt() > 0);
        } else {
            QCOMPARE(root->property("pooled").toInt(), 0);
            QCOMPARE(root->property("reused").toInt(), 0);
        }
        sheet->close();
        QTRY_VERIFY(! sheet->isVisible());
        QVERIFY(root->setProperty("rows", 0));
        settle(root);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(root->setProperty("rows", 80));
        sheet->open();
        QTRY_VERIFY(sheet->isOpened());
        settle(root);
        sheet->close();
    }

    void popupExplicitImplicitSize() {
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.Dialog {
                implicitWidth: 720
                implicitHeight: 520
                contentItem: Item { implicitWidth: 100; implicitHeight: 80 }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* popup = qobject_cast<qml_material::Popup*>(object.get());
        QVERIFY(popup);
        QCOMPARE(popup->implicitWidth(), 720);
        QCOMPARE(popup->implicitHeight(), 520);
        popup->contentItem()->setImplicitWidth(900);
        popup->contentItem()->setImplicitHeight(800);
        QCOMPARE(popup->implicitWidth(), 720);
        QCOMPARE(popup->implicitHeight(), 520);
    }

    void bottomSheetInputRegion() {
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                width: 400; height: 400
                property int clicks: 0
                property int wheels: 0
                property int sheetClicks: 0
                MouseArea {
                    objectName: "wallpapers"
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: parent.clicks++
                    onWheel: parent.wheels++
                }
                MD.BottomSheet {
                    objectName: "sheet"
                    sheetType: MD.Enum.BottomSheetStandard
                    animationDuration: 0
                    Item {
                        width: 400; height: 80
                        MouseArea {
                            width: 60; height: 60
                            objectName: "sheetButton"
                            onClicked: rootItem.sheetClicks++
                        }
                    }
                }
                id: rootItem
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(m_window.contentItem());
        auto* sheet = root->findChild<qml_material::Popup*>("sheet");
        QVERIFY(sheet);
        sheet->open();
        QTRY_VERIFY(sheet->isOpened());
        settle(root);
        auto* wallpapers = root->findChild<QQuickItem*>("wallpapers");
        QVERIFY(wallpapers);
        QTest::mouseMove(&m_window, QPoint(100, 100));
        QVERIFY(wallpapers->property("containsMouse").toBool());
        QTest::mouseClick(&m_window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
        QCOMPARE(root->property("clicks").toInt(), 1);
        auto wheel = [&](QPoint position) {
            QWheelEvent event(position,
                              m_window.mapToGlobal(position),
                              {},
                              QPoint(0, -120),
                              Qt::NoButton,
                              Qt::NoModifier,
                              Qt::NoScrollPhase,
                              false);
            QCoreApplication::sendEvent(&m_window, &event);
        };
        wheel(QPoint(100, 100));
        QCOMPARE(root->property("wheels").toInt(), 1);
        QTest::mouseClick(&m_window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 370));
        wheel(QPoint(100, 370));
        QCOMPARE(root->property("clicks").toInt(), 1);
        QCOMPARE(root->property("wheels").toInt(), 1);
        auto* button = root->findChild<QQuickItem*>("sheetButton");
        QVERIFY(button);
        const QPoint buttonPoint = button->mapToScene(QPointF(20, 20)).toPoint();
        QTest::mouseClick(&m_window, Qt::LeftButton, Qt::NoModifier, buttonPoint);
        QCOMPARE(root->property("sheetClicks").toInt(), 1);
        QTest::mousePress(&m_window, Qt::LeftButton, Qt::NoModifier, buttonPoint);
        QTest::mouseMove(&m_window, QPoint(100, 100));
        QTest::mouseRelease(&m_window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
        QVERIFY(! button->property("pressed").toBool());
        QCOMPARE(root->property("clicks").toInt(), 1);
        auto* scroller = sheet->findChild<qml_material::Flickable*>();
        QVERIFY(scroller);
        const QPoint handlePoint = sheet->popupItem()->mapToScene(QPointF(200, 24)).toPoint();
        QTest::mousePress(&m_window, Qt::LeftButton, Qt::NoModifier, handlePoint);
        QTest::mouseMove(&m_window, handlePoint + QPoint(0, 40));
        QTest::mouseMove(&m_window, handlePoint + QPoint(0, 80));
        QVERIFY(scroller->isDragging());
        QTest::mouseRelease(&m_window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
        QVERIFY(! scroller->isDragging());
        QCOMPARE(root->property("clicks").toInt(), 1);
        for (bool dim : { false, true }) {
            sheet->setModal(true);
            sheet->setDim(dim);
            sheet->setClosePolicy(qml_material::Popup::NoAutoClose);
            QTest::mouseClick(&m_window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
            wheel(QPoint(100, 100));
            QCOMPARE(root->property("clicks").toInt(), 1);
            QCOMPARE(root->property("wheels").toInt(), 1);
        }
        sheet->close();
        QTRY_VERIFY(! sheet->isVisible());
        QTest::mouseClick(&m_window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
        QCOMPARE(root->property("clicks").toInt(), 2);
        sheet->setModal(false);
        sheet->open();
        QTRY_VERIFY(sheet->isOpened());
        QTest::mouseClick(&m_window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
        QCOMPARE(root->property("clicks").toInt(), 3);
        sheet->setClosePolicy(qml_material::Popup::CloseOnPressOutside);
        QTest::mouseClick(&m_window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
        QTRY_VERIFY(! sheet->isVisible());
        QCOMPARE(root->property("clicks").toInt(), 4);
        qml_material::Popup blocker;
        blocker.setParentItem(root);
        blocker.setWidth(40);
        blocker.setHeight(40);
        blocker.setModal(true);
        blocker.setDim(false);
        blocker.setClosePolicy(qml_material::Popup::NoAutoClose);
        static_cast<QQmlParserStatus*>(&blocker)->componentComplete();
        blocker.open();
        sheet->setClosePolicy(qml_material::Popup::NoAutoClose);
        sheet->open();
        QTRY_VERIFY(sheet->isOpened());
        QTest::mouseClick(&m_window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
        QCOMPARE(root->property("clicks").toInt(), 4);
        sheet->close();
        blocker.close();
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

        firstAttached->setTimeout(-1);
        firstAttached->show(QStringLiteral("Before destruction"), 0);
        QTRY_VERIFY(firstAttached->visible());
        QPointer<QQuickItem> firstGuard(first);
        delete first;
        QVERIFY(firstGuard.isNull());
        QTRY_VERIFY(! sharedToolTip->property("visible").toBool());

        QQmlEngine    otherEngine;
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
        initiallyVisible.setData(QByteArrayLiteral(R"(
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
        attachedOnly.setData(QByteArrayLiteral(R"(
                import Qcm.Material as MD
                MD.ToolTip {}
            )"),
                             QUrl(QStringLiteral("qrc:/tests/tooltip-uncreatable.qml")));
        QVERIFY(attachedOnly.isError());

        QQmlComponent plain(&m_engine);
        plain.setData(QByteArrayLiteral(R"(
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

    void colorPickerButtonDialogLifecycle() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.ColorPickerButton { color: "#123456"; showAlpha: false }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* button = qobject_cast<qml_material::ColorPickerButton*>(object.get());
        QVERIFY(button);
        QVERIFY(! button->dialog());
        QVERIFY(QMetaObject::invokeMethod(button, "dismissPopup"));
        QVERIFY(! button->dialog());
        button->click();
        QVERIFY(! button->dialog());
        button->setParentItem(m_window.contentItem());
        QSignalSpy accepted(button, SIGNAL(accepted(QColor)));
        QVERIFY(accepted.isValid());

        button->click();
        QPointer<qml_material::Dialog> dialog = button->dialog();
        QVERIFY(dialog);
        QVERIFY(dialog->isVisible());
        QCOMPARE(dialog->property("color").value<QColor>(), QColor("#123456"));
        QCOMPARE(dialog->property("showAlpha").toBool(), false);
        button->click();
        QCOMPARE(button->dialog(), dialog);
        button->setProperty("color", QColor("#abcdef"));
        button->setProperty("showAlpha", true);
        QCOMPARE(dialog->property("color").value<QColor>(), QColor("#abcdef"));
        QCOMPARE(dialog->property("showAlpha").toBool(), true);

        dialog->setProperty("color", QColor("#fedcba"));
        dialog->setExit(nullptr);
        dialog->setDeferredCompletion(true);
        dialog->reject();
        QVERIFY(dialog->closing());
        button->click();
        QCOMPARE(button->dialog(), dialog);
        dialog->completeExit();
        QTRY_VERIFY(dialog.isNull());
        QCOMPARE(accepted.size(), 0);
        QCOMPARE(button->property("color").value<QColor>(), QColor("#abcdef"));

        button->click();
        dialog = button->dialog();
        QVERIFY(dialog);
        QCOMPARE(dialog->property("color").value<QColor>(), QColor("#abcdef"));
        dialog->setExit(nullptr);
        dialog->setProperty("color", QColor("#2468ac"));
        dialog->accept();
        QCOMPARE(accepted.size(), 1);
        QCOMPARE(accepted.at(0).at(0).value<QColor>(), QColor("#2468ac"));
        QCOMPARE(button->property("color").value<QColor>(), QColor("#2468ac"));
        QTRY_VERIFY(dialog.isNull());

        button->click();
        dialog = button->dialog();
        QVERIFY(dialog);
        QVERIFY(QMetaObject::invokeMethod(button, "dismissPopup"));
        QVERIFY(! dialog->isVisible());
        QTRY_VERIFY(dialog.isNull());
        QCOMPARE(accepted.size(), 1);

        button->click();
        dialog = button->dialog();
        QVERIFY(dialog);
        object.reset();
        QTRY_VERIFY(dialog.isNull());
    }

private:
    QQmlEngine   m_engine;
    QQuickWindow m_window;
};

int run_popup_layout(int argc, char** argv) {
    PopupLayoutTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "popup_layout.moc"
