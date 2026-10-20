#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include "qml_material/control/button_presentation.hpp"
#include "qml_material/control/command_button.hpp"
#include "qml_material/control/action.hpp"
#include "qml_material/control/button_group.hpp"
#include "qml_material/control/command_button.hpp"

using namespace qml_material;
namespace
{
class Presentation : public ButtonPresentation {
public:
    using ButtonPresentation::ButtonPresentation;
    void setDefaultText(const QString& value) {
        m_default = value;
        defaultTextChanged();
    }

protected:
    QString defaultText() const override { return m_default; }

private:
    QString m_default;
};
} // namespace
class ButtonPresentationTest : public QObject {
    Q_OBJECT
private slots:
    void presentationOnly() {
        ButtonPresentation item;
        for (const char* name : { "action", "checked", "checkable", "pressed", "pressX", "pressY" })
            QCOMPARE(item.metaObject()->indexOfProperty(name), -1);
        QCOMPARE(item.metaObject()->indexOfSignal("clicked()"), -1);
        QCOMPARE(item.metaObject()->indexOfSignal("toggled()"), -1);
        QVERIFY(item.icon());
        QCOMPARE(item.icon()->parent(), &item);
        QCOMPARE(item.findChildren<IconSpec*>().size(), 1);
        QCOMPARE(item.display(), ButtonPresentation::TextBesideIcon);
        QSignalSpy display(&item, &ButtonPresentation::displayChanged);
        item.setDisplay(ButtonPresentation::IconOnly);
        item.setDisplay(ButtonPresentation::IconOnly);
        QCOMPARE(display.size(), 1);
    }
    void explicitAndDefaultText() {
        Presentation item;
        QSignalSpy   changed(&item, &ButtonPresentation::textChanged);
        item.setDefaultText("First");
        QCOMPARE(item.text(), "First");
        item.setText("");
        QCOMPARE(item.text(), "");
        QCOMPARE(changed.size(), 2);
        item.setDefaultText("Second");
        QCOMPARE(item.text(), "");
        QCOMPARE(changed.size(), 2);
        item.resetText();
        QCOMPARE(item.text(), "Second");
        item.setText("Second");
        item.setDefaultText("Third");
        QCOMPARE(item.text(), "Second");
        QCOMPARE(changed.size(), 3);
        item.resetText();
        QCOMPARE(item.text(), "Third");
    }
    void indicatorOwnershipAndMetrics() {
        ButtonPresentation item;
        QQuickItem         externalParent, first, second(&externalParent);
        first.setImplicitWidth(24);
        first.setImplicitHeight(16);
        second.setImplicitWidth(32);
        second.setImplicitHeight(20);
        item.setIndicator(&first);
        QCOMPARE(first.parentItem(), &item);
        QCOMPARE(item.implicitIndicatorWidth(), 24);
        QCOMPARE(item.implicitIndicatorHeight(), 16);
        item.setIndicator(&second);
        QCOMPARE(first.parentItem(), nullptr);
        QCOMPARE(second.parentItem(), &externalParent);
        QCOMPARE(item.implicitIndicatorWidth(), 32);
        QCOMPARE(item.implicitIndicatorHeight(), 20);
        QSignalSpy width(&item, &ButtonPresentation::implicitIndicatorWidthChanged);
        first.setImplicitWidth(48);
        QCOMPARE(width.size(), 0);
        second.setImplicitWidth(48);
        QCOMPARE(width.size(), 1);
        item.setIndicator(nullptr);
        QCOMPARE(item.implicitIndicatorWidth(), 0);
        QCOMPARE(second.parentItem(), &externalParent);
    }
    void replacementDuringNotification() {
        ButtonPresentation item;
        QQuickItem         first, second;
        first.setImplicitWidth(16);
        first.setImplicitHeight(20);
        second.setImplicitWidth(32);
        second.setImplicitHeight(40);
        QSignalSpy width(&item, &ButtonPresentation::implicitIndicatorWidthChanged);
        QSignalSpy height(&item, &ButtonPresentation::implicitIndicatorHeightChanged);
        connect(&item, &ButtonPresentation::indicatorChanged, &item, [&] {
            if (item.indicator() == &first) item.setIndicator(&second);
        });
        item.setIndicator(&first);
        QCOMPARE(item.indicator(), &second);
        QCOMPARE(item.implicitIndicatorWidth(), 32);
        QCOMPARE(width.size(), 1);
        QCOMPARE(height.size(), 1);
        first.setImplicitWidth(64);
        QCOMPARE(width.size(), 1);
    }
    void replacementDuringDetach() {
        ButtonPresentation item;
        QQuickItem         first, second, replacement;
        item.setIndicator(&first);
        connect(&first, &QQuickItem::parentChanged, &item, [&](QQuickItem* parent) {
            if (! parent) item.setIndicator(&replacement);
        });
        item.setIndicator(&second);
        QCOMPARE(item.indicator(), &replacement);
        QCOMPARE(replacement.parentItem(), &item);
        QCOMPARE(second.parentItem(), nullptr);
    }
    void destroyedIndicatorReplacement() {
        ButtonPresentation item;
        auto               first = std::make_unique<QQuickItem>();
        QQuickItem         second;
        second.setImplicitWidth(30);
        second.setImplicitHeight(40);
        item.setIndicator(first.get());
        QSignalSpy width(&item, &ButtonPresentation::implicitIndicatorWidthChanged);
        QSignalSpy height(&item, &ButtonPresentation::implicitIndicatorHeightChanged);
        const auto connection = connect(&item, &ButtonPresentation::indicatorChanged, &item, [&] {
            if (! item.indicator()) item.setIndicator(&second);
        });
        first.reset();
        QCOMPARE(item.indicator(), &second);
        QCOMPARE(width.size(), 1);
        QCOMPARE(height.size(), 1);
        disconnect(connection);
    }
    void targetDeletedDuringDetach() {
        ButtonPresentation item;
        QQuickItem         first;
        auto               target = std::make_unique<QQuickItem>();
        item.setIndicator(&first);
        connect(&first, &QQuickItem::parentChanged, &item, [&](QQuickItem* parent) {
            if (! parent) target.reset();
        });
        QSignalSpy changed(&item, &ButtonPresentation::indicatorChanged);
        QSignalSpy width(&item, &ButtonPresentation::implicitIndicatorWidthChanged);
        item.setIndicator(target.get());
        QVERIFY(! target);
        QCOMPARE(item.indicator(), nullptr);
        QCOMPARE(item.implicitIndicatorWidth(), 0);
        QCOMPARE(changed.size(), 1);
        QCOMPARE(width.size(), 1);
    }
    void replacementDuringAttach() {
        ButtonPresentation item;
        QQuickItem         first, second;
        connect(&first, &QQuickItem::parentChanged, &item, [&](QQuickItem* parent) {
            if (parent == &item) item.setIndicator(&second);
        });
        QSignalSpy changed(&item, &ButtonPresentation::indicatorChanged);
        item.setIndicator(&first);
        QCOMPARE(item.indicator(), &second);
        QCOMPARE(first.parentItem(), nullptr);
        QCOMPARE(second.parentItem(), &item);
        QCOMPARE(changed.size(), 1);
    }
    void ownerDeletedInNotification() {
        auto       item = std::make_unique<ButtonPresentation>();
        QQuickItem indicator;
        connect(item.get(), &ButtonPresentation::indicatorChanged, this, [&] {
            item.reset();
        });
        item->setIndicator(&indicator);
        QVERIFY(! item);
        QCOMPARE(indicator.parentItem(), nullptr);
    }
    void groupTeardownDisconnectsPresentation() {
        ButtonGroup group;
        auto        item      = std::make_unique<ToggleButton>();
        auto        indicator = std::make_unique<QQuickItem>();
        item->setIndicator(indicator.get());
        group.addButton(item.get());
        QSignalSpy changed(item.get(), &ButtonPresentation::indicatorChanged);
        connect(&group, &ButtonGroup::buttonsChanged, this, [&] {
            indicator.reset();
        });
        item.reset();
        QVERIFY(! indicator);
        QCOMPARE(changed.size(), 0);
    }
    void qmlInheritedProperties() {
        QQmlEngine    engine;
        QSignalSpy    warnings(&engine, &QQmlEngine::warnings);
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.ActionButton {
                id: button
                property string label: "First"
                property MD.Action command: MD.Action { text: button.label; icon.name: "check" }
                action: command
                display: MD.CommandButtonBase.TextUnderIcon
                indicator: Item { implicitWidth: 18; implicitHeight: 20 }
                function overrideText() { text = ""; }
                function restoreText() { text = Qt.binding(() => MD.ActionCommand.text); }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* button = qobject_cast<CommandButton*>(object.get());
        QVERIFY(button);
        QCOMPARE(button->text(), "First");
        QCOMPARE(button->icon()->name(), "check");
        QCOMPARE(button->display(), ButtonPresentation::TextUnderIcon);
        QCOMPARE(button->implicitIndicatorWidth(), 18);
        QVERIFY(QMetaObject::invokeMethod(object.get(), "overrideText"));
        object->setProperty("label", "Second");
        QCOMPARE(button->text(), "");
        QVERIFY(QMetaObject::invokeMethod(object.get(), "restoreText"));
        QCOMPARE(button->text(), "Second");
        object->setProperty("label", "Third");
        QCOMPARE(button->text(), "Third");
        auto* action = button->property("action").value<Action*>();
        button->setProperty("action", QVariant::fromValue<Action*>(nullptr));
        QCOMPARE(button->text(), "");
        button->setProperty("action", QVariant::fromValue(action));
        QCOMPARE(button->text(), "Third");
        delete action;
        QCOMPARE(button->text(), "");
        QVERIFY(button->icon()->isEmpty());
        QVERIFY(warnings.isEmpty());
    }
};
int run_button_presentation(int argc, char** argv) {
    ButtonPresentationTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "button_presentation.moc"
