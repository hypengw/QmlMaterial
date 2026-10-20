#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlIncubator>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QtTest>
#include <memory>

#include "qml_material/control/action_group.hpp"
#include "qml_material/control/button_group.hpp"
#include "qml_material/control/tool_separator.hpp"

namespace
{

void processDeferredDeletes() {
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
}

} // namespace

class ControlRegistrationTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() { m_engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH)); }

    void nativeControlExports() {
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                MD.ActionGroup {
                    id: actions
                    objectName: "actions"
                    MD.Action { id: firstAction; checkable: true }
                }
                MD.Action {
                    id: secondAction
                    checkable: true
                    MD.ActionGroup.group: actions
                }
                MD.ButtonGroup { id: buttons; objectName: "buttons" }
                MD.ToggleButtonBase {
                    id: firstButton
                    checked: MD.ButtonGroup.checked
                    MD.ButtonGroup.group: buttons
                }
                MD.ToggleButtonBase {
                    id: secondButton
                    checked: MD.ButtonGroup.checked
                    MD.ButtonGroup.group: buttons
                }
                MD.ToolSeparator { id: separator; objectName: "separator" }
                function check() {
                    if (actions.actions.length !== 2 || buttons.buttons.length !== 2)
                        return "group membership";
                    firstAction.checked = true;
                    secondAction.checked = true;
                    if (firstAction.checked || actions.checkedAction !== secondAction)
                        return "action selection";
                    buttons.checkedButton = firstButton;
                    buttons.checkedButton = secondButton;
                    if (firstButton.checked || buttons.checkedButton !== secondButton)
                        return "button selection";
                    secondAction.MD.ActionGroup.group = null;
                    secondButton.MD.ButtonGroup.group = null;
                    if (actions.actions.length !== 1 || buttons.buttons.length !== 1)
                        return "detach";
                    if (!separator.vertical) return "default orientation";
                    separator.orientation = Qt.Horizontal;
                    if (!separator.horizontal || separator.vertical) return "orientation";
                    return "";
                }
            }
        )",
                          QUrl("qrc:/tests/NativeControlExports.qml"));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> host(component.create());
        QVERIFY(host);
        auto* actions   = host->findChild<qml_material::ActionGroup*>("actions");
        auto* buttons   = host->findChild<qml_material::ButtonGroup*>("buttons");
        auto* separator = host->findChild<qml_material::ToolSeparator*>("separator");
        QVERIFY(actions && buttons && separator);
        QCOMPARE(actions->metaObject(), &qml_material::ActionGroup::staticMetaObject);
        QCOMPARE(buttons->metaObject(), &qml_material::ButtonGroup::staticMetaObject);
        QCOMPARE(separator->metaObject(), &qml_material::ToolSeparator::staticMetaObject);
        QVariant result;
        QVERIFY(QMetaObject::invokeMethod(host.get(), "check", Q_RETURN_ARG(QVariant, result)));
        QCOMPARE(result.toString(), QString());
    }

private:
    QQmlEngine m_engine;
};

int run_control_registration(int argc, char** argv) {
    ControlRegistrationTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "control_registration.moc"
