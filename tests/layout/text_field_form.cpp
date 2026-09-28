#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include "qml_material/control/text_field.hpp"
#include "qml_material/style/text_field_state.hpp"

using namespace qml_material;

class TextFieldFormTest : public QObject {
    Q_OBJECT
private slots:
    void naturalWidthAndInput() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import Qcm.Material as MD
MD.TextField {
    prefix: "www."
    suffix: ".example"
    text: "site"
    supportingText: "Choose a name"
}
)",
                          QUrl());
        QQuickWindow window;
        window.resize(600, 300);
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* field = qobject_cast<TextField*>(object.get());
        QVERIFY(field);
        field->setParentItem(window.contentItem());
        QVERIFY(field->width() > field->contentWidth() + 32);
        const qreal naturalWidth = field->width();
        field->setWidth(90);
        QVERIFY(field->leftPadding() + field->rightPadding() <= field->width());
        QCOMPARE(field->implicitWidth(), naturalWidth);
        field->setWidth(naturalWidth);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, QPoint(30, 64));
        QVERIFY(field->hasActiveFocus());
        field->selectAll();
        for (auto key : { Qt::Key_N, Qt::Key_E, Qt::Key_W }) QTest::keyClick(&window, key);
        QCOMPARE(field->text(), QStringLiteral("new"));
        QCOMPARE(field->property("prefix").toString(), QStringLiteral("www."));
        QCOMPARE(field->property("suffix").toString(), QStringLiteral(".example"));
    }
    void form_data() {
        QTest::addColumn<int>("type");
        QTest::newRow("filled") << int(Enum::TextFieldType::TextFieldFilled);
        QTest::newRow("outlined") << int(Enum::TextFieldType::TextFieldOutlined);
    }
    void form() {
        QFETCH(int, type);
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.TextField {
    width: 240
    placeholderText: "Amount"
    text: "25"
    prefix: "$"
    suffix: "USD"
    property bool rtl: false
    LayoutMirroring.enabled: rtl
}
)",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QQuickWindow             window;
        std::unique_ptr<QObject> object(
            component.createWithInitialProperties({ { "type", type } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* field = qobject_cast<TextField*>(object.get());
        QVERIFY(field);
        field->setParentItem(window.contentItem());
        const auto labelFor = [field](const QString& text) -> QQuickItem* {
            for (auto* child : field->childItems()) {
                if (child->property("text").toString() == text) return child;
            }
            return nullptr;
        };
        auto* prefix = labelFor("$");
        auto* suffix = labelFor("USD");
        QVERIFY(prefix && suffix);
        QCOMPARE(field->height(), 56.);
        QCOMPARE(field->background()->height(), 56.);
        QCOMPARE(prefix->y() + prefix->baselineOffset(), field->baselineOffset());
        QCOMPARE(suffix->y() + suffix->baselineOffset(), field->baselineOffset());
        const auto baseline = field->baselineOffset();
        const auto padding  = field->bottomPadding();
        field->setProperty("supportingText", "Amount before tax");
        auto* support = labelFor("Amount before tax");
        QVERIFY(support);
        QVERIFY(field->height() > 56.);
        QCOMPARE(field->background()->height(), 56.);
        QCOMPARE(field->baselineOffset(), baseline);
        QCOMPARE(support->y(), 60.);
        QCOMPARE(field->bottomPadding(), padding + field->property("supportingHeight").toReal());
        const auto height = field->height();
        field->setProperty("supportingText",
                           "Long supporting text that wraps across several lines without changing "
                           "the input container height");
        QVERIFY(field->height() > height);
        QCOMPARE(field->background()->height(), 56.);
        QCOMPARE(field->baselineOffset(), baseline);
        field->setProperty("rtl", true);
        QCOMPARE(prefix->x(), field->width() - 16 - prefix->width());
        QCOMPARE(suffix->x(), 16.);
        field->setProperty("errorText", "Invalid amount");
        field->setProperty("error", true);
        QCOMPARE(support->property("text").toString(), QStringLiteral("Invalid amount"));
        auto* state = qvariant_cast<TextFieldState*>(field->property("mdState"));
        QVERIFY(state);
        QVERIFY(state->error());
        QCOMPARE(state->indicatorColor(), state->outlineColor());
        field->setFocus(true);
        QCOMPARE(state->indicatorHeight(), 2);
        field->setFocus(false);
        QCOMPARE(state->indicatorHeight(), 1);
        QCOMPARE(support->property("color").value<QColor>(), state->supportTextColor());
        field->setEnabled(false);
        QCOMPARE(state->state(), QStringLiteral("disabled"));
        field->setEnabled(true);
        const auto supportedHeight = field->height();
        field->setVisible(false);
        QCOMPARE(field->height(), supportedHeight);
        field->setVisible(true);
        field->setProperty("error", false);
        field->setProperty("supportingText", "");
        QCOMPARE(field->height(), 56.);
        QCOMPARE(field->bottomPadding(), padding);
        field->setText("");
        QVERIFY(! prefix->isVisible());
        QVERIFY(! suffix->isVisible());
        QCOMPARE(field->leftPadding(), 16.);
        field->setPlaceholderText("");
        QVERIFY(prefix->isVisible());
        QVERIFY(suffix->isVisible());
    }
    void validatorCompatibility() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.TextField {
    text: "1"
    validator: IntValidator { bottom: 10; top: 99 }
    errorText: "Enter 10–99"
}
)",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* field = qobject_cast<TextField*>(object.get());
        QVERIFY(field);
        QVERIFY(field->property("error").toBool());
        field->setText("25");
        QVERIFY(! field->property("error").toBool());
        QCOMPARE(field->height(), 56.);
        field->setProperty("error", true);
        field->setText("50");
        QVERIFY(field->property("error").toBool());
    }
};

int run_text_field_form(int argc, char** argv) {
    TextFieldFormTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "text_field_form.moc"
