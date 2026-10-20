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

#include "qml_material/control/icon_spec.hpp"
#include "qml_material/layout/layout_container.hpp"
#include "qml_material/core/enum.hpp"

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

QQuickItem* itemWithIconView(QQuickItem* root) {
    if (auto* icon = root->property("icon").value<qml_material::IconSpec*>();
        icon && ! icon->isEmpty()) {
        return root;
    }
    for (auto* child : root->childItems()) {
        if (auto* match = itemWithIconView(child)) return match;
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

QQuickItem* innerTextItem(QQuickItem* root) {
    if (root->property("font").isValid() && root->property("text").isValid()) {
        return root;
    }
    for (auto* child : root->childItems()) {
        if (auto* match = innerTextItem(child)) {
            return match;
        }
    }
    return nullptr;
}

int curveRenderingType(QQmlEngine& engine) {
    static int cached = -1;
    if (cached >= 0) {
        return cached;
    }
    QQmlComponent component(&engine);
    component.setData(
        QByteArrayLiteral("import QtQuick\nQtObject { property int v: Text.CurveRendering }"),
        QUrl(QStringLiteral("qrc:/tests/curve-rendering-enum.qml")));
    if (component.isError()) {
        return cached = 0;
    }
    std::unique_ptr<QObject> object(component.create());
    if (! object) {
        return cached = 0;
    }
    return cached = object->property("v").toInt();
}
} // namespace

class IconLayoutTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        m_engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        m_window.setGeometry(0, 0, 800, 600);
        m_window.create();
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

        auto* icon = itemWithIconView(row);
        QVERIFY(icon);
        QCOMPARE(icon->isVisible(), iconVisible);
    }

    void iconRenderingUsesCurvePath_data() {
        QTest::addColumn<int>("size");

        QTest::newRow("18") << 18;
        QTest::newRow("20") << 20;
        QTest::newRow("24") << 24;
    }

    void iconRenderingUsesCurvePath() {
        QFETCH(int, size);

        const auto source = QStringLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            MD.Icon {
                id: icon
                name: MD.Token.icon.content_copy
                size: %1
            }
        )")
                                .arg(size)
                                .toUtf8();

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/icon-rendering.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* icon = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(icon);
        icon->setParentItem(m_window.contentItem());
        settle(icon);

        QCOMPARE(icon->implicitWidth(), qreal(size));
        QCOMPARE(icon->implicitHeight(), qreal(size));

        auto* text = innerTextItem(icon);
        QVERIFY(text);
        const auto font = qvariant_cast<QFont>(text->property("font"));
        QCOMPARE(font.pixelSize(), size);
        QCOMPARE(text->property("lineHeight").toReal(), qreal(font.pixelSize()));
        QCOMPARE(text->scale(), 1.0);
        QCOMPARE(text->property("renderType").toInt(), curveRenderingType(m_engine));
    }

    void iconKeepsCurveConfigurationUnderParentScale_data() {
        QTest::addColumn<qreal>("parentScale");

        QTest::newRow("zoom-out-small") << 0.3;
        QTest::newRow("zoom-out") << 0.5;
        QTest::newRow("zoom-in-small") << 1.5;
        QTest::newRow("zoom-in") << 2.0;
        QTest::newRow("zoom-in-large") << 3.0;
    }

    void iconKeepsCurveConfigurationUnderParentScale() {
        QFETCH(qreal, parentScale);

        const auto source = QByteArrayLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            Item {
                id: host
                MD.Icon {
                    name: MD.Token.icon.content_copy
                    size: 20
                }
            }
        )");

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/icon-parent-scale.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* host = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(host);
        host->setScale(parentScale);
        host->setParentItem(m_window.contentItem());
        settle(host);

        auto* icon = itemWithIcon(host);
        QVERIFY(icon);
        QCOMPARE(icon->property("size").toInt(), 20);

        auto* text = innerTextItem(icon);
        QVERIFY(text);
        const auto font = qvariant_cast<QFont>(text->property("font"));
        QCOMPARE(font.pixelSize(), 20);
        QCOMPARE(text->property("lineHeight").toReal(), qreal(font.pixelSize()));
        QCOMPARE(text->scale(), 1.0);
        QCOMPARE(text->property("renderType").toInt(), curveRenderingType(m_engine));
    }

    void iconRenderTypeCanBeOverridden() {
        const auto source = QByteArrayLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            MD.Icon {
                property int expectedRenderType: Text.QtRendering
                name: MD.Token.icon.content_copy
                renderType: Text.QtRendering
            }
        )");

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/icon-render-type.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* icon = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(icon);

        const int expectedRenderType = icon->property("expectedRenderType").toInt();
        QCOMPARE(icon->property("renderType").toInt(), expectedRenderType);

        auto* text = innerTextItem(icon);
        QVERIFY(text);
        QCOMPARE(text->property("renderType").toInt(), expectedRenderType);
    }

    void iconButtonContentIconSize() {
        const auto source = QByteArrayLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            MD.IconButton {
                id: button
                icon.name: MD.Token.icon.delete
            }
        )");

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/icon-button-icon-size.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* button = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(button);
        button->setParentItem(m_window.contentItem());
        settle(button);

        auto* mdState = qvariant_cast<QObject*>(button->property("mdState"));
        QVERIFY(mdState);
        const qreal iconSize = mdState->property("iconSize").toReal();
        QCOMPARE(iconSize, 24.0);

        auto* content = qvariant_cast<QQuickItem*>(button->property("contentItem"));
        QVERIFY(content);
        QCOMPARE(content->implicitWidth(), iconSize);
        QCOMPARE(content->implicitHeight(), iconSize);

        auto* icon = itemWithIcon(content);
        QVERIFY(icon);
        QCOMPARE(icon->property("size").toInt(), qRound(iconSize));

        QVERIFY(mdState->setProperty("size", int(qml_material::Enum::ButtonSize::XS)));
        settle(button);
        QCOMPARE(mdState->property("iconSize").toReal(), 22.0);
        QCOMPARE(button->property("padding").toReal(), 8.0);
        QCOMPARE(button->implicitWidth(), 38.0);
        QCOMPARE(button->implicitHeight(), 38.0);
        auto* background = qvariant_cast<QQuickItem*>(button->property("background"));
        QVERIFY(background);
        QCOMPARE(background->implicitWidth(), 0.0);
        QCOMPARE(background->implicitHeight(), 0.0);
        QCOMPARE(background->width(), 30.0);
        QCOMPARE(background->height(), 30.0);
        QCOMPARE(icon->property("size").toInt(), 22);

        QVERIFY(button->setProperty("padding", 4.0));
        settle(button);
        QCOMPARE(button->implicitWidth(), 30.0);
        QCOMPARE(button->implicitHeight(), 30.0);

        QVERIFY(button->setProperty("padding", 8.0));
        QVERIFY(mdState->setProperty("size", int(qml_material::Enum::ButtonSize::S)));
        settle(button);
        QCOMPARE(button->implicitWidth(), 48.0);
        QCOMPARE(button->implicitHeight(), 48.0);
        QCOMPARE(icon->property("size").toInt(), 24);
    }

private:
    QQmlEngine   m_engine;
    QQuickWindow m_window;
};

int run_icon_layout(int argc, char** argv) {
    IconLayoutTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "icon_layout.moc"
