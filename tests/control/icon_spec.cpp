#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QRawFont>
#include "qml_material/control/action.hpp"
#include "qml_material/control/command_button.hpp"

using namespace qml_material;

class IconSpecTest : public QObject {
    Q_OBJECT
private slots:
    void init() {
        QTest::failOnWarning(
            QRegularExpression(".*(Binding loop|Unable to assign|Cannot assign|TypeError).*"));
    }

    void fontCoverage_data() {
        QTest::addColumn<QString>("fileName");
        for (const auto* fill : { "0", "1" }) {
            const auto name =
                QStringLiteral("MaterialSymbolsRounded.wght_400.opsz_24.fill_") + fill;
            QTest::newRow(qPrintable(name + ".ttf")) << name + ".ttf";
#if ! defined(Q_OS_WIN) && ! defined(Q_OS_ANDROID) && ! defined(Q_OS_WASM)
            QTest::newRow(qPrintable(name + ".woff2")) << name + ".woff2";
#endif
        }
#if ! defined(Q_OS_WIN) && ! defined(Q_OS_ANDROID) && ! defined(Q_OS_WASM)
        QTest::newRow("FILL-variable")
            << QStringLiteral("MaterialSymbolsRounded.wght_400.opsz_24.woff2");
#endif
    }
    void fontCoverage() {
        QFETCH(QString, fileName);
        const auto path =
            QTest::qFindTestData(qPrintable("../../assets/" + fileName), __FILE__, __LINE__);
        QVERIFY(! path.isEmpty());
        const QRawFont font(path, 24);
        QVERIFY2(font.isValid(), qPrintable(fileName));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData("import Qcm.Material as MD; MD.TokenImpl {}", QUrl());
        std::unique_ptr<QObject> token(component.create());
        QVERIFY2(token, qPrintable(component.errorString()));
        auto* icons = token->property("icon").value<QObject*>();
        QVERIFY(icons);
        const auto* meta = icons->metaObject();
        QVERIFY(meta->propertyCount() > meta->propertyOffset());
        QCOMPARE(icons->property("2d_2").toString(), QString::fromUcs4(U"\U000fff0e"));
        for (int i = meta->propertyOffset(); i < meta->propertyCount(); ++i) {
            const auto property = meta->property(i);
            const auto text     = property.read(icons).toString();
            const auto points   = text.toUcs4();
            QCOMPARE(points.size(), 1);
            QVERIFY2(font.supportsCharacter(points.first()), property.name());
            const auto glyphs = font.glyphIndexesForString(text);
            QCOMPARE(glyphs.size(), 1);
            QVERIFY2(glyphs.first() != 0, property.name());
        }
    }

    void interfaces() {
        Action        action;
        CommandButton button;
        const auto    actionIcon =
            action.metaObject()->property(action.metaObject()->indexOfProperty("icon"));
        const auto controlIcon =
            button.metaObject()->property(button.metaObject()->indexOfProperty("icon"));
        QVERIFY(! actionIcon.isWritable());
        QVERIFY(! controlIcon.isWritable());
        QVERIFY(actionIcon.isConstant());
        QVERIFY(controlIcon.isConstant());
        QCOMPARE(action.icon()->metaObject()->indexOfProperty("width"), -1);
        QCOMPARE(action.icon()->metaObject()->indexOfProperty("height"), -1);
        QCOMPARE(button.metaObject()->indexOfProperty("_iconDefaults"), -1);
        QCOMPARE(button.metaObject()->indexOfProperty("effectiveIcon"), -1);
        QCOMPARE(button.icon()->parent(), &button);
        QCOMPARE(action.icon()->parent(), &action);
    }

    void precedence() {
        Action        action;
        CommandButton first, second;
        auto*         icon = first.icon();
        icon->setName("local");
        icon->setColor(Qt::red);
        icon->setFill(true);
        icon->setCache(false);
        icon->setWidth(18);
        second.icon()->setWidth(32);
        first.icon()->setActionIcon(action.icon());
        second.icon()->setActionIcon(action.icon());
        QCOMPARE(icon->name(), QString("local"));
        QCOMPARE(icon->color(), QColor(Qt::red));
        QVERIFY(icon->fill());
        QSignalSpy fillChanged(icon, &ActionIcon::fillChanged);
        action.icon()->setFill(false);
        QCOMPARE(fillChanged.count(), 1);
        QVERIFY(! icon->fill());
        action.icon()->setFill(false);
        QCOMPARE(fillChanged.count(), 1);
        action.icon()->setColor(Qt::transparent);
        action.icon()->setCache(true);
        QCOMPARE(icon->color(), QColor(Qt::transparent));
        QVERIFY(icon->cache());
        icon->setColor(Qt::blue);
        icon->setFill(false);
        icon->setFill(true);
        QCOMPARE(icon->color(), QColor(Qt::transparent));
        QVERIFY(! icon->fill());
        action.icon()->resetColor();
        action.icon()->resetFill();
        action.icon()->resetCache();
        QCOMPARE(icon->color(), QColor(Qt::blue));
        QVERIFY(icon->fill());
        QVERIFY(! icon->cache());
        action.icon()->setName("home");
        QCOMPARE(icon->name(), QString("home"));
        QCOMPARE(icon->width(), 18);
        QCOMPARE(second.icon()->width(), 32);
        icon->setWidth(0);
        QCOMPARE(icon->width(), 0);
        icon->resetWidth();
        QCOMPARE(icon->width(), 0);
        action.icon()->resetName();
        QCOMPARE(icon->name(), QString("local"));
        icon->resetColor();
        QCOMPARE(icon->color(), QColor(Qt::transparent));
    }

    void contentAndLifetime() {
        CommandButton button;
        auto*         icon = button.icon();
        icon->setSource(QUrl("file:///tmp/local.svg"));
        auto action = std::make_unique<Action>();
        button.icon()->setActionIcon(action->icon());
        QVERIFY(icon->isImage());
        action->icon()->setName("");
        QVERIFY(icon->isEmpty());
        action->icon()->setName("home");
        QVERIFY(! icon->isImage());
        QCOMPARE(icon->name(), QString("home"));
        action->icon()->setSource(QUrl("file:///tmp/action.svg"));
        QCOMPARE(icon->resolvedSource(), QUrl("file:///tmp/action.svg"));
        action->icon()->resetSource();
        QCOMPARE(icon->name(), QString("home"));
        action->icon()->resetName();
        QVERIFY(icon->isImage());
        Action replacement;
        replacement.icon()->setName("edit");
        button.icon()->setActionIcon(replacement.icon());
        action->icon()->setName("ignored");
        QCOMPARE(icon->name(), QString("edit"));
        button.icon()->setActionIcon(action->icon());
        action.reset();
        QCOMPARE(icon->resolvedSource(), QUrl("file:///tmp/local.svg"));
        QVERIFY(button.icon() == icon);

        auto                    dying = std::make_unique<CommandButton>();
        QPointer<CommandButton> guard(dying.get());
        dying->icon()->setActionIcon(replacement.icon());
        connect(dying->icon(), &ActionIcon::colorChanged, &replacement, [&] {
            dying.reset();
        });
        replacement.icon()->setColor(Qt::red);
        QVERIFY(guard.isNull());
    }

    void qmlBindingsAndUrls_data() {
        QTest::addColumn<QByteArray>("kind");
        QTest::newRow("command") << QByteArray("CommandButtonBase");
        QTest::newRow("selection") << QByteArray("SelectionButtonBase");
    }
    void qmlBindingsAndUrls() {
        QFETCH(QByteArray, kind);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent actionComponent(&engine);
        actionComponent.setData(R"(
            import Qcm.Material as MD
            MD.Action { icon.source: "images/action.svg" }
        )",
                                QUrl("file:///tmp/action-owner/Action.qml"));
        QVERIFY2(actionComponent.isReady(), qPrintable(actionComponent.errorString()));
        std::unique_ptr<QObject> actionObject(actionComponent.create());
        auto*                    action = qobject_cast<Action*>(actionObject.get());
        QVERIFY(action);

        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.)" + kind + R"( {
                property MD.Action iconOwner: null
                property color localColor: "red"
                property bool localFill: true
                icon.color: localColor
                icon.fill: localFill
                icon.source: "images/control.svg"
                icon.width: 18
                icon.actionIcon: iconOwner?.icon ?? null
                function resetActionColor() { iconOwner.icon.color = undefined }
            }
        )",
                          QUrl("file:///tmp/control-owner/CommandButton.qml"));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        auto*                    button = qobject_cast<ButtonPresentation*>(object.get());
        QVERIFY(button);
        QVERIFY(button->setProperty("iconOwner", QVariant::fromValue(action)));
        QCOMPARE(button->icon()->actionIcon(), action->icon());
        QCOMPARE(button->icon()->resolvedSource(),
                 QUrl("file:///tmp/action-owner/images/action.svg"));
        action->icon()->setColor(Qt::transparent);
        action->icon()->setFill(false);
        button->setProperty("localColor", QColor(Qt::blue));
        button->setProperty("localFill", false);
        button->setProperty("localFill", true);
        QCOMPARE(button->icon()->color(), QColor(Qt::transparent));
        QVERIFY(! button->icon()->fill());
        QVERIFY(QMetaObject::invokeMethod(button, "resetActionColor"));
        action->icon()->resetFill();
        QCOMPARE(button->icon()->color(), QColor(Qt::blue));
        QVERIFY(button->icon()->fill());
        button->setProperty("localColor", QColor(Qt::green));
        QCOMPARE(button->icon()->color(), QColor(Qt::green));
        action->icon()->resetSource();
        QCOMPARE(button->icon()->resolvedSource(),
                 QUrl("file:///tmp/control-owner/images/control.svg"));
        actionObject.reset();
        QCOMPARE(button->icon()->actionIcon(), nullptr);
        QCOMPARE(button->icon()->color(), QColor(Qt::green));
        Action replacement;
        replacement.icon()->setName("replacement");
        QVERIFY(button->setProperty("iconOwner", QVariant::fromValue(&replacement)));
        QCOMPARE(button->icon()->name(), QString("replacement"));
    }

    void rejectedAssignments_data() {
        QTest::addColumn<QByteArray>("body");
        QTest::newRow("action width") << QByteArray("MD.Action { icon.width: 24 }");
        QTest::newRow("action height") << QByteArray("MD.Action { icon.height: 24 }");
        QTest::newRow("action replacement") << QByteArray("MD.Action { icon: null }");
        QTest::newRow("button replacement") << QByteArray("MD.CommandButtonBase { icon: null }");
    }
    void rejectedAssignments() {
        QFETCH(QByteArray, body);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData("import Qcm.Material as MD\n" + body, QUrl());
        QVERIFY(component.isError());
    }

    void components() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(QM_ICON_TEST_QML)));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> host(component.create());
        QVERIFY(host);
        QVariant result;
        QVERIFY(QMetaObject::invokeMethod(host.get(), "check", Q_RETURN_ARG(QVariant, result)));
        QCOMPARE(result.toString(), QString());
        QTRY_VERIFY(host->property("imageReady").toBool());
        QVERIFY(QMetaObject::invokeMethod(host.get(), "failImage"));
        QTRY_COMPARE(host->property("imageStatus").toInt(), 3);
        QVERIFY(QMetaObject::invokeMethod(host.get(), "showFont"));
        QVERIFY(QMetaObject::invokeMethod(host.get(), "fontShown", Q_RETURN_ARG(QVariant, result)));
        QVERIFY(result.toBool());
        QVERIFY(QMetaObject::invokeMethod(host.get(), "clearImage"));
        QVERIFY(
            QMetaObject::invokeMethod(host.get(), "emptyImage", Q_RETURN_ARG(QVariant, result)));
        QVERIFY(result.toBool());
    }
};

int run_icon_spec(int argc, char** argv) {
    QCoreApplication::setAttribute(Qt::AA_Use96Dpi, true);
    IconSpecTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}
#include "icon_spec.moc"
