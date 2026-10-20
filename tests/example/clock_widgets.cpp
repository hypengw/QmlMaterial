#include <QtTest/QtTest>
#include <array>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlListReference>
#include <QQuickWindow>
#include <QTimeZone>
#include <QTimer>
#include <private/qqmldata_p.h>
#include "qml_material/control/clock.hpp"
#include "qml_material/control/segmented_button_group.hpp"
#include "qml_material/core/enum.hpp"
#include "qml_material/shape/material_shapes.hpp"
#include "qml_material/token/color.hpp"

using namespace qml_material;
namespace
{
QDateTime utc(const char* text) {
    return QDateTime::fromString(QString::fromLatin1(text), Qt::ISODate).toUTC();
}

void polish(QQuickItem* item) {
    item->ensurePolished();
    for (auto* child : item->childItems()) polish(child);
}
} // namespace

class ClockWidgetsTest : public QObject {
    Q_OBJECT

private slots:
    void init() {
        QTest::failOnWarning(QRegularExpression(
            ".*(Binding loop|Unable to assign|Cannot assign|TypeError|ReferenceError).*"));
    }

    void example() {
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(320, 760);
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(
            &engine, QUrl::fromLocalFile(QFINDTESTDATA("../../example/compositions/Clocks.qml")));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(window.contentItem());
        root->setWidth(320);
        object->setProperty("fixedTime", true);
        auto* clock = root->findChild<Clock*>("clockPreview");
        QVERIFY(clock);
        polish(root);
        QVERIFY(! root->findChild<QObject*>("clockZoneSelector"));
        QVERIFY(! root->findChild<QObject*>("clockLocaleSelector"));
        QVERIFY(clock->timeZoneId().isEmpty());
        const auto click = [&](const char* name) {
            auto* button = root->findChild<QObject*>(name);
            QVERIFY(button);
            QVERIFY(QMetaObject::invokeMethod(button, "click"));
        };
        const auto select = [&](const char* name, int index) {
            auto* group = root->findChild<SegmentedButtonGroup*>(name);
            QVERIFY(group);
            QCOMPARE(group->count(), QString(name) == "clockPresetChoice" ? 6 : 4);
            auto* button = group->itemAt(index);
            QVERIFY(button);
            QVERIFY(QMetaObject::invokeMethod(button, "click"));
        };
        click("clockLiveChoice");
        QVERIFY(! object->property("fixedTime").toBool());
        QCOMPARE(clock->source()->mode(), ClockSource::System);
        click("clockFixedChoice");
        QVERIFY(object->property("fixedTime").toBool());
        QCOMPARE(clock->source()->mode(), ClockSource::Fixed);
        click("clock12HourChoice");
        QCOMPARE(clock->hourFormat(), TimeState::Hour12);
        click("clock24HourChoice");
        QCOMPARE(clock->hourFormat(), TimeState::Hour24);
        click("clockAutoChoice");
        QCOMPARE(clock->hourFormat(), TimeState::LocaleFormat);
        for (const auto* name : { "clockSourceChoice",
                                  "clockHourFormatChoice",
                                  "clockPresetChoice",
                                  "clockShapeChoice" }) {
            auto* group = root->findChild<QQuickItem*>(name);
            QVERIFY(group);
            QVERIFY(group->width() <= root->width());
            QCOMPARE(group->property("size").toInt(), int(Enum::ButtonSize::XS));
        }
        select("clockPresetChoice", 1);
        const QList<MaterialShape::Type> shapes { MaterialShape::Cookie12Sided,
                                                  MaterialShape::Flower,
                                                  MaterialShape::Clover4Leaf,
                                                  MaterialShape::Square };
        for (int shape = 0; shape < 4; ++shape) {
            select("clockShapeChoice", shape);
            QCOMPARE(object->property("shapeIndex").toInt(), shape);
            QCOMPARE(clock->property("shape").toInt(), int(shapes[shape]));
        }
        const QList<Clock::Preset> widgetPresets { Clock::Analog,  Clock::ExpressiveAnalog,
                                                   Clock::Analog,  Clock::ExpressiveAnalog,
                                                   Clock::Digital, Clock::StackedDigital };
        for (int preset = 0; preset < widgetPresets.size(); ++preset) {
            select("clockPresetChoice", preset);
            object->setProperty("formatIndex", 1);
            object->setProperty("seconds", true);
            polish(root);
            QCoreApplication::processEvents();
            QVERIFY(clock->width() <= root->width());
            QCOMPARE(clock->dateTime().toUTC(), utc("2026-10-16T10:09:30Z"));
            QCOMPARE(clock->preset(), widgetPresets[preset]);
            QCOMPARE(clock->property("style").toInt(), preset);
        }
        select("clockPresetChoice", 4);
        click("clockTransparentChoice");
        QVERIFY(! clock->property("solid").toBool());
        click("clockSolidChoice");
        QVERIFY(clock->property("solid").toBool());
        QCOMPARE(root->findChildren<Clock*>().size(), 1);
    }

    void widgetFaces() {
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(400, 400);
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        ClockSource source;
        source.setMode(ClockSource::Fixed);
        source.setFixedDateTime(utc("2026-10-16T10:09:30Z"));
        QQmlComponent component(&engine,
                                QUrl("qrc:/Qcm/Material/Example/compositions/ClockWidget.qml"));
        std::unique_ptr<QObject> object(
            component.createWithInitialProperties({ { "source", QVariant::fromValue(&source) },
                                                    { "locale", QLocale("en_US") },
                                                    { "showDate", true },
                                                    { "showSeconds", true },
                                                    { "hourFormat", int(TimeState::Hour24) } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* clock = qobject_cast<Clock*>(object.get());
        QVERIFY(clock);
        clock->setParentItem(window.contentItem());
        clock->setSize({ 320, 320 });
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        for (const auto* name : { "widgetMediumFont" }) {
            auto* font = clock->findChild<QObject*>(name);
            QVERIFY(font);
            QTRY_COMPARE(font->property("status").toInt(), 1);
            QVERIFY(! font->property("name").toString().isEmpty());
        }
        auto* artwork    = clock->findChild<QQuickItem*>("widgetArtwork");
        auto* background = clock->findChild<QQuickItem*>("widgetSolidBackground");
        auto* day        = clock->findChild<QQuickItem*>("widgetDayWindow");
        auto* orbit      = clock->findChild<QQuickItem*>("widgetOrbitDate");
        QVERIFY(artwork && background && day && orbit);
        for (int width : { 288, 320 }) {
            clock->setWidth(width);
            for (int style = 0; style < 6; ++style) {
                clock->setProperty("style", style);
                polish(clock);
                QCoreApplication::processEvents();
                QCOMPARE(clock->property("analog").toBool(), style < 4);
                QCOMPARE(day->isVisible(), style == 2);
                QCOMPARE(orbit->isVisible(), style == 1);
                QCOMPARE(background->isVisible(), style == 0 || style == 2 || style >= 4);
                QVERIFY(clock->boundingRect()
                            .adjusted(-.01, -.01, .01, .01)
                            .contains(artwork->mapRectToItem(clock, artwork->boundingRect())));
                QCOMPARE(clock->findChild<QQuickItem*>("widgetHourHand")->rotation(),
                         clock->hourAngle());
                QCOMPARE(clock->findChild<QQuickItem*>("widgetMinuteHand")->rotation(),
                         clock->minuteAngle());
                QCOMPARE(clock->findChild<QQuickItem*>("widgetSecondHand")->rotation(),
                         clock->secondAngle());
                if (style == 0) {
                    for (const auto* name : { "widgetNumeral12",
                                              "widgetNumeral3",
                                              "widgetNumeral6",
                                              "widgetNumeral9" }) {
                        auto* numeral = clock->findChild<QObject*>(name);
                        QVERIFY(numeral);
                        QQmlListReference elements(numeral, "pathElements");
                        QCOMPARE(elements.count(), 1);
                        QVERIFY(! elements.at(0)->property("path").toString().isEmpty());
                        QCOMPARE(numeral->property("fillColor"), clock->property("numeralColor"));
                    }
                }
                if (style < 4) {
                    const std::array kinds  = style == 1 ? std::array { 2, 4, 6 }
                                                         : std::array { style == 2 ? 1 : 0, 3, 5 };
                    const std::array names  = { "widgetHourHand",
                                                "widgetMinuteHand",
                                                "widgetSecondHand" };
                    const std::array colors = { "handColor", "minuteHandColor", "secondHandColor" };
                    for (int i = 0; i < 3; ++i) {
                        auto* hand = clock->findChild<QQuickItem*>(names[i]);
                        QCOMPARE(hand->property("kind").toInt(), kinds[i]);
                        QCOMPARE(hand->property("color"), clock->property(colors[i]));
                        QCOMPARE(hand->size(), artwork->size());
                    }
                }
                if (style >= 4) {
                    clock->setProperty("solid", false);
                    QVERIFY(! background->isVisible());
                    clock->setProperty("solid", true);
                    QVERIFY(background->isVisible());
                }
            }
        }
        clock->setProperty("style", 2);
        clock->setShowDate(false);
        QVERIFY(! day->isVisible());
        clock->setShowSeconds(false);
        QVERIFY(! clock->findChild<QQuickItem*>("widgetSecondHand")->isVisible());
        clock->setProperty("style", 4);
        QVERIFY(! clock->findChild<QQuickItem*>("widgetAlarm")->isVisible());
        clock->setProperty("alarmText", "Mon 09:30");
        QVERIFY(clock->findChild<QQuickItem*>("widgetAlarm")->isVisible());
        window.hide();
        QTRY_VERIFY(! clock->active());
    }

    void widgetContentColors() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            import "qrc:/Qcm/Material/Example/compositions" as Demo
            Demo.ClockWidget {
                showDate: true
                showSeconds: true
                alarmText: "Mon 09:30"
                MD.MProp.color: MD.MdColorMgr {
                    objectName: "widgetTheme"
                    mode: MD.Enum.Light
                    useSysColorSM: false
                    useSysAccentColor: false
                }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> clock(component.create());
        QVERIFY2(clock, qPrintable(component.errorString()));
        auto* item = qobject_cast<QQuickItem*>(clock.get());
        QVERIFY(item);
        auto* colors = clock->findChild<MdColorMgr*>("widgetTheme");
        QVERIFY(colors);
        for (auto mode : { Enum::ThemeMode::Light, Enum::ThemeMode::Dark }) {
            colors->setMode(mode);
            for (auto accent : { QColor("#587b6b"), QColor("#b95942") }) {
                colors->setAccentColor(accent);
                const auto ink = clock->property("numeralColor").value<QColor>();
                QCOMPARE(ink, colors->on_primary_container());
                QCOMPARE(clock->property("textColor").value<QColor>(), ink);
                for (auto name : { "handColor", "minuteHandColor", "secondHandColor" })
                    QVERIFY2(ink != clock->property(name).value<QColor>(), name);
                for (auto name :
                     { "widgetNumeral12", "widgetNumeral3", "widgetNumeral6", "widgetNumeral9" }) {
                    auto* numeral = clock->findChild<QObject*>(name);
                    QVERIFY(numeral);
                    QCOMPARE(numeral->property("fillColor").value<QColor>(), ink);
                }
                QCOMPARE(clock->findChild<QObject*>("widgetDayWindow")
                             ->property("color")
                             .value<QColor>(),
                         clock->property("faceColor").value<QColor>());
                QCOMPARE(clock->findChild<QObject*>("widgetAlarmIcon")
                             ->property("color")
                             .value<QColor>(),
                         ink);
                const auto verifyTextColors = [&](auto&& self, QQuickItem* child) -> void {
                    if (child->isVisible() && child->property("text").isValid() &&
                        child->property("color").isValid())
                        QCOMPARE(child->property("color").value<QColor>(), ink);
                    for (auto* next : child->childItems()) self(self, next);
                };
                for (int style = 0; style < 6; ++style) {
                    clock->setProperty("style", style);
                    for (bool solid : { true, false }) {
                        clock->setProperty("solid", solid);
                        verifyTextColors(verifyTextColors, item);
                    }
                }
                QCOMPARE(clock->property("handColor").value<QColor>(), colors->secondary());
                QCOMPARE(clock->property("minuteHandColor").value<QColor>(), colors->primary());
                QCOMPARE(clock->property("secondHandColor").value<QColor>(), colors->tertiary());
            }
        }
    }

    void widgetHandGeometry_data() {
        QTest::addColumn<int>("kind");
        QTest::addColumn<QSizeF>("vectorSize");
        QTest::addColumn<QSizeF>("viewportSize");
        QTest::addColumn<qreal>("topInset");
        QTest::addColumn<qreal>("bottomInset");
        QTest::newRow("simpleHour") << 0 << QSizeF(29, 101) << QSizeF(18, 63) << .187 << .467;
        QTest::newRow("dayHour") << 1 << QSizeF(29, 111) << QSizeF(18, 69) << .2 << .45;
        QTest::newRow("scallopHour") << 2 << QSizeF(38, 114) << QSizeF(38, 114) << .25 << .45;
        QTest::newRow("simpleMinute") << 3 << QSizeF(6, 137) << QSizeF(4, 85) << .044 << .489;
        QTest::newRow("scallopMinute") << 4 << QSizeF(38, 152) << QSizeF(38, 152) << .15 << .45;
        QTest::newRow("simpleSecond") << 5 << QSizeF(10, 145) << QSizeF(6, 87) << .0385 << .4835;
        QTest::newRow("scallopSecond") << 6 << QSizeF(38, 38) << QSizeF(38, 38) << .05 << .85;
    }

    void widgetHandGeometry() {
        QFETCH(int, kind);
        QFETCH(QSizeF, vectorSize);
        QFETCH(QSizeF, viewportSize);
        QFETCH(qreal, topInset);
        QFETCH(qreal, bottomInset);
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(400, 400);
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine,
                                QUrl("qrc:/Qcm/Material/Example/compositions/ClockWidgetHand.qml"));
        const QColor  ink("#246850");
        std::unique_ptr<QObject> object(
            component.createWithInitialProperties({ { "kind", kind }, { "color", ink } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* hand = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(hand);
        hand->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* geometry = hand->findChild<QQuickItem*>("handGeometry");
        auto* outline  = hand->findChild<QObject*>("handOutline");
        QVERIFY(geometry && outline);
        QCOMPARE(outline->property("strokeWidth").toReal(), kind == 1 ? 3.0 : 0.0);
        QCOMPARE(outline->property("fillColor").value<QColor>(),
                 kind == 1 ? QColor(Qt::transparent) : ink);
        for (qreal size : { 380, 240, 190, 0 }) {
            hand->setSize({ size, size });
            polish(hand);
            QCOMPARE(geometry->size(), viewportSize);
            const qreal scale  = size / 380;
            const auto  bounds = geometry->mapRectToItem(hand, QRectF(QPointF(), geometry->size()));
            QVERIFY2(qAbs(bounds.x() - (size - vectorSize.width() * scale) / 2) < 1e-4,
                     qPrintable(QString("size=%1 actualX=%2 expectedX=%3")
                                    .arg(size)
                                    .arg(bounds.x(), 0, 'g', 16)
                                    .arg((size - vectorSize.width() * scale) / 2, 0, 'g', 16)));
            const qreal expectedY = size / 2 + vectorSize.height() * (topInset - .5) /
                                                   (1 - topInset - bottomInset) * scale;
            QVERIFY2(qAbs(bounds.y() - expectedY) < 1e-4,
                     qPrintable(QString("size=%1 actualY=%2 expectedY=%3")
                                    .arg(size)
                                    .arg(bounds.y(), 0, 'g', 16)
                                    .arg(expectedY, 0, 'g', 16)));
            QVERIFY(qAbs(bounds.width() - vectorSize.width() * scale) < 1e-4);
            QVERIFY(qAbs(bounds.height() - vectorSize.height() * scale) < 1e-4);
            QVERIFY(bounds.isEmpty() || hand->boundingRect().contains(bounds));
        }
    }

    void compositionsExample() {
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(1400, 760);
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine, QUrl("qrc:/Qcm/Material/Example/Compositions.qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(window.contentItem());
        auto* clocks = root->findChild<QQuickItem*>("compositionClocks");
        QVERIFY(clocks);
        auto* preview = clocks->findChild<Clock*>("clockPreview");
        QVERIFY(preview);
        QCOMPARE(root->findChildren<Clock*>("clockPreview").size(), 1);
        auto* section    = clocks;
        auto* timePicker = root->findChild<QQuickItem*>("compositionTimePickers");
        QVERIFY(section && timePicker);
        QCOMPARE(section->parentItem(), timePicker->parentItem());
        const auto sections = section->parentItem()->childItems();
        QVERIFY(sections.indexOf(section) < sections.indexOf(timePicker));
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        for (int width : { 900, 1400 }) {
            root->setSize({ qreal(width), 760 });
            polish(root);
            QCoreApplication::processEvents();
            QVERIFY(clocks->width() > 0);
            QVERIFY(clocks->height() > 0);
            QVERIFY(preview->width() <= clocks->width());
            QTRY_VERIFY(timePicker->y() >= section->y() + section->height());
            bool scrollableParent = false;
            for (auto* item = clocks->parentItem(); item; item = item->parentItem()) {
                scrollableParent |= item->inherits("QQuickFlickable");
            }
            QVERIFY(scrollableParent);
            QCOMPARE(preview->parentItem()->parentItem(), clocks);
            for (auto* item : clocks->childItems()) {
                QVERIFY(! item->inherits("QQuickFlickable"));
            }
        }
    }
};

int run_clock_widgets(int argc, char** argv) {
    ClockWidgetsTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "clock_widgets.moc"
