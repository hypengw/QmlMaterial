#include <QCoreApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QtTest>
#include <memory>

#include "qml_material/layout/layout_container.hpp"

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
    if (root->property("text").toString() == text) {
        return root;
    }
    for (auto* child : root->childItems()) {
        if (auto* match = itemWithText(child, text)) {
            return match;
        }
    }
    return nullptr;
}

} // namespace

class ChipLayoutTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void constrainedTextElides_data() {
        QTest::addColumn<QString>("type");
        QTest::addColumn<QString>("setup");

        QTest::newRow("assist") << QStringLiteral("AssistChip")
                                << QStringLiteral("icon.name: MD.Token.icon.search");
        QTest::newRow("filter") << QStringLiteral("FilterChip")
                                << QStringLiteral("checked: true; icon.name: MD.Token.icon.close");
        QTest::newRow("input")
            << QStringLiteral("InputChip")
            << QStringLiteral("leadingItem: Item { implicitWidth: 18; implicitHeight: 18 }\n"
                              "icon.name: MD.Token.icon.close");
        QTest::newRow("suggestion") << QStringLiteral("SuggestionChip")
                                    << QStringLiteral("icon.name: MD.Token.icon.search");
        QTest::newRow("embed") << QStringLiteral("EmbedChip")
                               << QStringLiteral(
                                      "icon.name: MD.Token.icon.search; trailingIconName: "
                                      "MD.Token.icon.close");
    }

    void constrainedTextElides() {
        QFETCH(QString, type);
        QFETCH(QString, setup);

        const QString text   = QStringLiteral("A chip label that is much wider than its control");
        const auto    source = QStringLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            MD.%1 {
                width: 112
                text: "%2"
                %3
            }
        )")
                                   .arg(type, text, setup)
                                   .toUtf8();

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/chip-layout.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* chip = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(chip);
        settle(chip);

        auto* content = qvariant_cast<QQuickItem*>(chip->property("contentItem"));
        QVERIFY(content);
        QVERIFY(qobject_cast<qml_material::Row*>(content));

        auto* label = itemWithText(content, text);
        QVERIFY(label);
        QVERIFY(chip->implicitWidth() > chip->width());
        QVERIFY(label->implicitWidth() > label->width());
        QCOMPARE(label->property("elide").toInt(), int(Qt::ElideRight));
        QCOMPARE(label->property("wrapMode").toInt(), 0);
        QVERIFY(label->property("truncated").isValid());
        QVERIFY(label->property("truncated").toBool());
    }

private:
    QQmlEngine m_engine;
};

QTEST_MAIN(ChipLayoutTest)

#include "chip_layout.moc"
