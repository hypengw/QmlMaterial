#include <QtTest/QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QAbstractItemModel>
#include <QJSValue>
#include <algorithm>
#include <cmath>
#include <limits>
#include <QDir>
#include <QMutex>
#include <private/qquickanimation_p.h>
#include "qml_material/control/button_control.hpp"
#include "qml_material/input/swipe_to_dismiss_state.hpp"

namespace
{
void polish(QQuickItem* item) {
    item->ensurePolished();
    for (auto* child : item->childItems()) polish(child);
}

QQuickItem* visualChild(QQuickItem* item, const QString& name) {
    if (item->objectName() == name) return item;
    for (auto* child : item->childItems()) {
        if (auto* result = visualChild(child, name)) return result;
    }
    return nullptr;
}

void click(QObject* button) {
    QVERIFY(button);
    QVERIFY(QMetaObject::invokeMethod(button, "click"));
}
} // namespace

class NotificationsTest : public QObject {
    Q_OBJECT

private slots:
    void init() {
        QTest::failOnWarning(
            QRegularExpression(".*(Binding loop|Unable to assign|Cannot "
                               "assign|Cannot open|TypeError|ReferenceError|polish.*loop).*"));
    }

    void notifications() {
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(400, 800);
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(
            &engine, QUrl("qrc:/Qcm/Material/Example/compositions/NotificationList.qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        root->setProperty("animationsEnabled", false);
        root->setParentItem(window.contentItem());
        root->setSize({ 360, 740 });
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        polish(root);
        auto* feed = root->findChild<QObject*>("notificationFeed");
        QVERIFY(feed);
        auto* message = visualChild(root, "notification-message");
        QVERIFY(message);
        click(message->findChild<QObject*>("expandNotification"));
        QVERIFY(message->property("expanded").toBool());
        QVERIFY(message->height() > 72);
        click(message->findChild<QObject*>("replyNotification"));
        QVERIFY(message->property("composing").toBool());
        auto* input = message->findChild<QQuickItem*>("notificationReplyInput");
        QVERIFY(input);
        input->forceActiveFocus();
        for (char character : QByteArrayLiteral("See you there"))
            QTest::keyClick(&window, character);
        QTest::keyClick(&window, Qt::Key_Return);
        QCOMPARE(message->property("message").toString(), QString("You: See you there"));
        QVERIFY(! message->property("composing").toBool());
        QVERIFY(! input->hasActiveFocus());
        QCOMPARE(feed->property("count").toInt(), 2);
        click(message->findChild<QObject*>("replyNotification"));
        QVERIFY(message->property("composing").toBool());
        click(message->findChild<QObject*>("expandNotification"));
        QVERIFY(! message->property("expanded").toBool());
        QVERIFY(! message->property("composing").toBool());
        QVERIFY(! input->hasActiveFocus());
        click(message->findChild<QObject*>("expandNotification"));
        click(message->findChild<QObject*>("readNotification"));
        QCOMPARE(feed->property("count").toInt(), 1);
        QVERIFY(! visualChild(root, "notification-message"));
        QVERIFY(QMetaObject::invokeMethod(feed, "reset"));
        QCOMPARE(feed->property("count").toInt(), 2);
        message = visualChild(root, "notification-message");
        QVERIFY(message);
        message->setProperty("animationsEnabled", false);
        QCoreApplication::processEvents();
        polish(root);
        const auto start = message->mapToScene({ 40, 44 }).toPoint();
        QVERIFY(window.contentItem()->boundingRect().contains(start));
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, start);
        for (int offset : { 20, 60, 120, 200 })
            QTest::mouseMove(&window, start + QPoint(offset, 0));
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, start + QPoint(200, 0));
        QTRY_COMPARE(feed->property("count").toInt(), 1);
        QVERIFY(! visualChild(root, "notification-message"));
    }

    void notificationTemplates() {
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(400, 800);
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(
            &engine, QUrl("qrc:/Qcm/Material/Example/compositions/NotificationCard.qml"));
        std::unique_ptr<QObject> object(component.createWithInitialProperties(
            { { "app", "Downloads" },
              { "title", "Download complete" },
              { "message", "12 images" },
              { "detail", "12 images saved to Downloads." },
              { "age", "Now" },
              { "symbol", "download_done" },
              { "thumbnail",
                QUrl("qrc:/Qcm/Material/Example/assets/carousel/abstract-mint-square.jpg") },
              { "animationsEnabled", false } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* card = qobject_cast<QQuickItem*>(object.get());
        card->setParentItem(window.contentItem());
        card->setWidth(360);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        polish(card);
        QCOMPARE(card->height(), 72);
        QCOMPARE(card->findChild<QQuickItem*>("notificationAppIcon")->size(), QSizeF(40, 40));
        auto* thumbnail = card->findChild<QQuickItem*>("notificationThumbnail");
        auto* summary   = card->findChild<QQuickItem*>("notificationSummary");
        QVERIFY(thumbnail && summary);
        QVERIFY(thumbnail->isVisible());
        QCOMPARE(thumbnail->size(), QSizeF(48, 48));
        QTRY_COMPARE(thumbnail->property("status").toInt(), 1);
        QCOMPARE(summary->x() + summary->width(), thumbnail->x() - 16);
        auto* expander = qobject_cast<qml_material::ButtonControl*>(
            card->findChild<QObject*>("expandNotification"));
        QVERIFY(expander);
        QCOMPARE(expander->background()->size(), QSizeF(26, 18));
        click(expander);
        auto* title  = card->findChild<QQuickItem*>("notificationExpandedTitle");
        auto* detail = card->findChild<QQuickItem*>("notificationDetail");
        QVERIFY(title && detail);
        QCOMPARE(title->y(), 38);
        QCOMPARE(detail->y(), title->y() + title->height() + 8);
        QCOMPARE(card->height(), detail->y() + detail->implicitHeight() + 16);
        card->setProperty("title", "");
        QCOMPARE(
            card->findChild<QObject*>("notificationCollapsedTitle")->property("text").toString(),
            QString("Downloads"));
        QCOMPARE(detail->y(), 46);
        card->setProperty("ongoing", true);
        auto* state = card->property("dismissState").value<qml_material::SwipeToDismissState*>();
        QVERIFY(state);
        QVERIFY(! card->property("startToEndEnabled").toBool());
        QVERIFY(! card->property("endToStartEnabled").toBool());
    }

    void notificationExpansion() {
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(1000, 800);
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            import "qrc:/Qcm/Material/Example/compositions" as Demo
            MD.VerticalFlickable {
                property alias animationsEnabled: list.animationsEnabled
                Demo.NotificationList { id: list; width: parent.width }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(window.contentItem());
        root->setSize({ 900, 740 });
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        polish(root);
        auto* message  = visualChild(root, "notification-message");
        auto* calendar = visualChild(root, "notification-calendar");
        auto* scroll   = root;
        QVERIFY(message && calendar && scroll);
        auto* detail    = message->findChild<QQuickItem*>("notificationDetail");
        auto* summary   = message->findChild<QQuickItem*>("notificationSummary");
        auto* animation = message->findChild<QObject*>("notificationHeightAnimation");
        QVERIFY(detail && summary && animation);
        QCOMPARE(animation->property("duration").toInt(), 360);
        QCOMPARE(message->height(), 72);
        const auto   calendarY = calendar->mapToItem(root, QPointF()).y();
        const auto   scrollY   = scroll->property("contentY").toReal();
        QList<qreal> heights;
        const auto   samples = connect(message, &QQuickItem::heightChanged, this, [&] {
            heights.append(message->height());
        });
        click(message->findChild<QObject*>("expandNotification"));
        QVERIFY(message->property("expansionAnimating").toBool());
        QCOMPARE(message->height(), 72);
        QTRY_VERIFY(! message->property("expansionAnimating").toBool());
        const auto expandedHeight = message->property("expandedHeight").toReal();
        QCOMPARE(message->height(), expandedHeight);
        QVERIFY(std::any_of(heights.begin(), heights.end(), [&](qreal height) {
            return height > 72 && height < expandedHeight;
        }));
        QCOMPARE(detail->opacity(), 1);
        QCOMPARE(summary->opacity(), 0);
        polish(root);
        QCOMPARE(calendar->mapToItem(root, QPointF()).y() - calendarY, expandedHeight - 72);
        QCOMPARE(scroll->property("contentY").toReal(), scrollY);
        click(message->findChild<QObject*>("replyNotification"));
        QTRY_VERIFY(! message->property("expansionAnimating").toBool());
        QCOMPARE(message->height(), expandedHeight + 64);
        auto* input = message->findChild<QQuickItem*>("notificationReplyInput");
        QVERIFY(input);
        QTRY_VERIFY(input->hasActiveFocus());
        click(message->findChild<QObject*>("expandNotification"));
        QVERIFY(! message->property("composing").toBool());
        QVERIFY(! input->hasActiveFocus());
        QTRY_VERIFY(! message->property("expansionAnimating").toBool());
        QCOMPARE(message->height(), 72);
        QCOMPARE(summary->opacity(), 1);
        QCOMPARE(detail->opacity(), 0);
        disconnect(samples);

        bool       reversed   = false;
        bool       continuous = false;
        const auto reversal   = connect(message, &QQuickItem::heightChanged, this, [&] {
            if (! reversed && message->height() > 72 && message->height() < expandedHeight) {
                reversed = true;
                QMetaObject::invokeMethod(
                    message,
                    [&] {
                        const auto before = message->height();
                        message->setProperty("expanded", false);
                        continuous = message->height() == before;
                    },
                    Qt::QueuedConnection);
            }
        });
        message->setProperty("expanded", true);
        QTRY_VERIFY(reversed && ! message->property("expansionAnimating").toBool());
        QCOMPARE(message->height(), 72);
        QVERIFY(continuous);
        disconnect(reversal);

        bool       reduced   = false;
        const auto reduction = connect(message, &QQuickItem::heightChanged, this, [&] {
            if (! reduced && message->height() > 72 && message->height() < expandedHeight) {
                reduced = true;
                root->setProperty("animationsEnabled", false);
            }
        });
        message->setProperty("expanded", true);
        QTRY_VERIFY(reduced && ! message->property("expansionAnimating").toBool());
        QCOMPARE(message->height(), expandedHeight);
        message->setProperty("expanded", false);
        QCOMPARE(message->height(), 72);
        disconnect(reduction);
        root->setProperty("animationsEnabled", true);
        message->setProperty("expanded", true);
        QVERIFY(message->property("expansionAnimating").toBool());
        window.hide();
        QTRY_VERIFY(! message->property("expansionAnimating").toBool());
        QCOMPARE(message->height(), expandedHeight);
    }
};

int run_notifications(int argc, char** argv) {
    NotificationsTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "notifications.moc"
