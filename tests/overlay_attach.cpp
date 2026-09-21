#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QtTest>
#include <memory>

class OverlayAttachTest : public QObject {
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
    }

    void attachOverlaySetsParentAndRejectsNullAnchor() {
        QQmlComponent component(&m_engine);
        component.setData(QByteArrayLiteral(R"(
            import QtQuick
            import QtQuick.Controls
            import Qcm.Material as MD

            ApplicationWindow {
                id: root
                width: 320
                height: 240
                visible: true

                property var attachResult: null
                property var nullAnchorResult: null
                property var nullPopupResult: null
                property Item stuckParentBefore: null

                Item {
                    id: dummy
                    objectName: "dummy"
                }

                Popup {
                    id: popup
                    objectName: "popup"
                    width: 40
                    height: 40
                }

                Component.onCompleted: {
                    popup.parent = dummy
                    stuckParentBefore = popup.parent
                    nullAnchorResult = MD.Util.attachOverlay(popup, null)
                    nullPopupResult = MD.Util.attachOverlay(null, contentItem)
                    attachResult = MD.Util.attachOverlay(popup, contentItem)
                }
            }
        )"),
                          QUrl(QStringLiteral("qrc:/tests/overlay-attach-util.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* window = qobject_cast<QQuickWindow*>(object.get());
        QVERIFY(window);
        QTRY_VERIFY(window->isExposed() || window->isVisible());

        auto* popup = window->findChild<QObject*>(QStringLiteral("popup"));
        auto* dummy = window->findChild<QQuickItem*>(QStringLiteral("dummy"));
        QVERIFY(popup);
        QVERIFY(dummy);

        QCOMPARE(qvariant_cast<QQuickItem*>(object->property("stuckParentBefore")), dummy);
        QVERIFY(object->property("nullAnchorResult").isNull());
        QVERIFY(object->property("nullPopupResult").isNull());

        auto* overlay = qvariant_cast<QQuickItem*>(object->property("attachResult"));
        QVERIFY(overlay);
        QCOMPARE(qvariant_cast<QQuickItem*>(popup->property("parent")), overlay);
    }

    void colorPickerButtonReparentsDialogOnOpen() {
        QQmlComponent component(&m_engine);
        component.setData(QByteArrayLiteral(R"(
            import QtQuick
            import QtQuick.Controls
            import Qcm.Material as MD

            ApplicationWindow {
                id: root
                width: 400
                height: 300
                visible: true

                property Item buttonOverlay: null
                property alias button: button

                MD.ColorPickerButton {
                    id: button
                    objectName: "colorPickerButton"
                    anchors.centerIn: parent
                    Component.onCompleted: root.buttonOverlay = Overlay.overlay
                }
            }
        )"),
                          QUrl(QStringLiteral("qrc:/tests/overlay-attach-colorpicker.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* window = qobject_cast<QQuickWindow*>(object.get());
        QVERIFY(window);
        QTRY_VERIFY(window->isExposed() || window->isVisible());

        auto* button = window->findChild<QQuickItem*>(QStringLiteral("colorPickerButton"));
        QVERIFY(button);
        auto* dialog = button->findChild<QObject*>(QStringLiteral("colorPickerDialog"));
        QVERIFY(dialog);

        auto* buttonOverlay = qvariant_cast<QQuickItem*>(object->property("buttonOverlay"));
        QVERIFY(buttonOverlay);

        QVERIFY(dialog->setProperty("parent", QVariant::fromValue(button)));
        QCOMPARE(qvariant_cast<QQuickItem*>(dialog->property("parent")), button);
        QVERIFY(qvariant_cast<QQuickItem*>(dialog->property("parent")) != buttonOverlay);

        QVERIFY(QMetaObject::invokeMethod(button, "click"));
        QTRY_VERIFY(dialog->property("visible").toBool());
        QCOMPARE(qvariant_cast<QQuickItem*>(dialog->property("parent")), buttonOverlay);

        QVERIFY(QMetaObject::invokeMethod(dialog, "close"));
        QTRY_VERIFY(! dialog->property("visible").toBool());
    }

    void colorPickerButtonReparentsAfterCrossWindowStuckParent() {
        QQmlComponent component(&m_engine);
        component.setData(QByteArrayLiteral(R"(
            import QtQuick
            import QtQuick.Controls
            import Qcm.Material as MD

            Item {
                id: root
                width: 1
                height: 1

                property Item mainOverlay: null
                property Item floatOverlay: null
                property alias button: button

                ApplicationWindow {
                    id: mainWin
                    width: 400
                    height: 300
                    visible: true
                    title: "main"
                    Component.onCompleted: root.mainOverlay = Overlay.overlay
                }

                ApplicationWindow {
                    id: floatWin
                    width: 400
                    height: 300
                    visible: true
                    title: "float"
                    x: 420

                    MD.ColorPickerButton {
                        id: button
                        objectName: "colorPickerButton"
                        anchors.centerIn: parent
                    }

                    Component.onCompleted: root.floatOverlay = Overlay.overlay
                }
            }
        )"),
                          QUrl(QStringLiteral("qrc:/tests/overlay-attach-hop.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));

        QTRY_VERIFY(qvariant_cast<QQuickItem*>(object->property("mainOverlay")));
        QTRY_VERIFY(qvariant_cast<QQuickItem*>(object->property("floatOverlay")));

        auto* button = object->findChild<QQuickItem*>(QStringLiteral("colorPickerButton"));
        QVERIFY(button);
        auto* dialog = button->findChild<QObject*>(QStringLiteral("colorPickerDialog"));
        QVERIFY(dialog);

        auto* mainOverlay  = qvariant_cast<QQuickItem*>(object->property("mainOverlay"));
        auto* floatOverlay = qvariant_cast<QQuickItem*>(object->property("floatOverlay"));
        QVERIFY(mainOverlay != floatOverlay);

        QVERIFY(dialog->setProperty("parent", QVariant::fromValue(mainOverlay)));
        QCOMPARE(qvariant_cast<QQuickItem*>(dialog->property("parent")), mainOverlay);

        QVERIFY(QMetaObject::invokeMethod(button, "click"));
        QTRY_VERIFY(dialog->property("visible").toBool());
        QCOMPARE(qvariant_cast<QQuickItem*>(dialog->property("parent")), floatOverlay);

        QVERIFY(QMetaObject::invokeMethod(dialog, "close"));
        QTRY_VERIFY(! dialog->property("visible").toBool());
    }

private:
    QQmlEngine m_engine;
};

int main(int argc, char* argv[]) {
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "0");
    qputenv("QT_SCALE_FACTOR", "1");

    QGuiApplication app(argc, argv);
    OverlayAttachTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "overlay_attach.moc"
