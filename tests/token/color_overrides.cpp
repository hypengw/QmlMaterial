#include <QColor>
#include <QMetaProperty>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSignalSpy>
#include <QtTest>
#include <memory>

#include "qml_material/token/color.hpp"

using qml_material::MdColorMgr;

namespace
{
auto roleColors(const QObject& obj) -> QVariantMap {
    QVariantMap out;
    auto*       meta = obj.metaObject();
    for (int i = meta->propertyOffset(); i < meta->propertyCount(); ++i) {
        auto prop = meta->property(i);
        if (prop.metaType() == QMetaType::fromType<QColor>())
            out.insert(prop.name(), prop.read(&obj));
    }
    return out;
}
} // namespace

class ColorOverridesTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void emptyKeepsScheme() {
        MdColorMgr mgr;
        mgr.setUseSysColorSM(false);
        mgr.setMode(qml_material::Enum::ThemeMode::Dark);
        const auto before = roleColors(mgr);
        mgr.setOverrides({ { "surface_container", "#1b2838" } });
        mgr.setOverrides({});
        QCOMPARE(roleColors(mgr), before);
    }

    void overrideOneRole() {
        MdColorMgr mgr;
        mgr.setUseSysColorSM(false);
        mgr.setMode(qml_material::Enum::ThemeMode::Dark);
        const auto before = roleColors(mgr);

        QSignalSpy scheme(&mgr, &MdColorMgr::schemeChanged);
        mgr.setOverrides({ { "surface_container", "#1b2838" },
                           { "on_surface", QColor(0xc7, 0xd5, 0xe0) },
                           { "no_such_role", "#ffffff" } });
        QCOMPARE(scheme.count(), 1);

        auto after = roleColors(mgr);
        QCOMPARE(after.take("surface_container").value<QColor>(), QColor("#1b2838"));
        QCOMPARE(after.take("on_surface").value<QColor>(), QColor(0xc7, 0xd5, 0xe0));
        auto expected = before;
        expected.remove("surface_container");
        expected.remove("on_surface");
        QCOMPARE(after, expected);

        // overrides survive a scheme regeneration
        mgr.setMode(qml_material::Enum::ThemeMode::Light);
        QCOMPARE(mgr.surface_container(), QColor("#1b2838"));
    }

    void qmlBinding() {
        QQmlEngine    engine;
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD

            QtObject {
                property color c: MD.Token.color.surface_container
                property color other: MD.Token.color.surface
                function set(v) { MD.Token.color.overrides = v; }
            }
        )",
                          QUrl(QStringLiteral("qrc:/tests/ColorOverrides.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> obj(component.create());
        QVERIFY(obj);

        const auto     c0     = obj->property("c").value<QColor>();
        const auto     other0 = obj->property("other").value<QColor>();
        const QVariant steam  = QVariantMap { { "surface_container", "#1b2838" } };
        QVERIFY(QMetaObject::invokeMethod(obj.get(), "set", Q_ARG(QVariant, steam)));
        QCOMPARE(obj->property("c").value<QColor>(), QColor("#1b2838"));
        QCOMPARE(obj->property("other").value<QColor>(), other0);

        const QVariant none = QVariantMap {};
        QVERIFY(QMetaObject::invokeMethod(obj.get(), "set", Q_ARG(QVariant, none)));
        QCOMPARE(obj->property("c").value<QColor>(), c0);
    }
};

int run_color_overrides(int argc, char** argv) {
    ColorOverridesTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "color_overrides.moc"
