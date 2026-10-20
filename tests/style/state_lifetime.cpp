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

#include "qml_material/style/list_item_state.hpp"
#include "qml_material/style/common_state.hpp"
#include "qml_material/style/button_interaction_state.hpp"
#include "qml_material/style/text_field_state.hpp"
#include "qml_material/style/slider_state.hpp"
#include "qml_material/style/drag_handle_state.hpp"
#include "qml_material/style/material_state.hpp"
#include "qml_material/style/theme.hpp"
#include "qml_material/token/color.hpp"
#include "qml_material/control/item_delegate.hpp"
#include "qml_material/control/text_field.hpp"
#include "qml_material/control/slider.hpp"

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
} // namespace

class StateLifetimeTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        m_engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        m_window.setGeometry(0, 0, 800, 600);
        m_window.create();
    }

    void stateDuringPageDestruction_data() {
        QTest::addColumn<bool>("reuse");
        QTest::addColumn<bool>("scroll");
        QTest::addColumn<QString>("delegateFile");
        for (const auto& file :
             { QStringLiteral("list_item_state.qml"), QStringLiteral("control_states.qml") }) {
            for (bool reuse : { true, false }) {
                for (bool scroll : { true, false }) {
                    const auto name =
                        file + (reuse ? "-reused" : "-not-reused") + (scroll ? "-scrolled" : "");
                    QTest::newRow(qPrintable(name)) << reuse << scroll << file;
                }
            }
        }
    }

    void stateDuringPageDestruction() {
        QFETCH(bool, reuse);
        QFETCH(bool, scroll);
        QFETCH(QString, delegateFile);
        QTest::failOnWarning(QRegularExpression(".*TypeError.*"));
        QQmlComponent delegate(&m_engine,
                               QUrl::fromLocalFile(QStringLiteral(QM_LIST_ITEM_STATE_QML))
                                   .resolved(QUrl(delegateFile)));
        QVERIFY2(delegate.isReady(), qPrintable(delegate.errorString()));
        QQmlComponent component(&m_engine);
        component.setData(R"(
            pragma ComponentBehavior: Bound
            import QtQuick
            import Qcm.Material as MD
            Item {
                id: root
                width: 400; height: 300
                property Component rowDelegate
                property bool reuse: true
                property var page: null
                function openPage() { page = factory.createObject(root) }
                function hidePage() { page.visible = false }
                function closePage() { page.destroy(); page = null }
                Component {
                    id: factory
                    MD.Page {
                        width: 400; height: 300
                        MD.PageVerticalListView {
                            objectName: "list"
                            anchors.fill: parent
                            model: 80
                            reuseItems: root.reuse
                            cacheBuffer: 0
                            delegate: root.rowDelegate
                        }
                    }
                }
            }
        )",
                          QUrl("qrc:/tests/list-item-state-destruction.qml"));
        std::unique_ptr<QObject> object(component.createWithInitialProperties(
            { { "rowDelegate", QVariant::fromValue(&delegate) }, { "reuse", reuse } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(root);
        root->setParentItem(m_window.contentItem());
        QVERIFY(QMetaObject::invokeMethod(root, "openPage"));
        settle(root);
        auto* list = root->findChild<QQuickItem*>("list");
        QVERIFY(list);
        if (scroll) {
            for (int i = 1; i <= 5; ++i) {
                QVERIFY(list->setProperty("contentY", i * 300));
                settle(root);
            }
        }
        QList<QPointer<QQuickItem>> trackedRows;
        for (int i = 0; i < list->property("count").toInt(); ++i) {
            QQuickItem* item = nullptr;
            QVERIFY(QMetaObject::invokeMethod(
                list, "itemAtIndex", Q_RETURN_ARG(QQuickItem*, item), Q_ARG(int, i)));
            if (item) trackedRows.append(item);
        }
        QVERIFY(! trackedRows.isEmpty());
        QPointer<QObject> page(root->property("page").value<QObject*>());
        QVERIFY(page);
        QVERIFY(QMetaObject::invokeMethod(root, "hidePage"));
        for (const auto& row : trackedRows) {
            if (row) QVERIFY(! row->isEnabled());
        }
        QVERIFY(QMetaObject::invokeMethod(root, "closePage"));
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QTRY_VERIFY(page.isNull());
        QTRY_VERIFY(std::all_of(trackedRows.cbegin(), trackedRows.cend(), [](const auto& row) {
            return row.isNull();
        }));
    }

    void listItemStateOutlivesInput() {
        QProperty<QColor>           color(Qt::red);
        qml_material::ListItemState state;
        auto                        item = std::make_unique<qml_material::ItemDelegate>();
        state.setItem(item.get());
        state.bindableBackgroundColor().setBinding([&] {
            return color.value();
        });
        item->setEnabled(false);
        QCOMPARE(state.contentOpacity(), state.stateTokens().disabled_content);
        QSignalSpy itemChanged(&state, &qml_material::ButtonInteractionState::itemChanged);
        QSignalSpy stateChanged(&state, &qml_material::CommonState::contentOpacityChanged);
        item.reset();
        QVERIFY(! state.item());
        QVERIFY(! state.target());
        QVERIFY(! state.ctx());
        QVERIFY(! state.colors());
        QCOMPARE(itemChanged.count(), 1);
        QCOMPARE(stateChanged.count(), 1);
        QCOMPARE(state.contentOpacity(), 1);
        QCOMPARE(state.backgroundColor(), QColor(Qt::red));
        color = Qt::blue;
        QCOMPARE(state.backgroundColor(), QColor(Qt::blue));
        auto replacement = std::make_unique<qml_material::ItemDelegate>();
        state.setItem(replacement.get());
        QCOMPARE(state.item(), replacement.get());
        QVERIFY(state.ctx());
        QVERIFY(state.colors());
        replacement->setEnabled(false);
        QCOMPARE(state.contentOpacity(), state.stateTokens().disabled_content);
        replacement->setEnabled(true);
        QCOMPARE(state.contentOpacity(), 1);
        QCOMPARE(state.backgroundColor(), QColor(Qt::blue));
    }

    void controlStateOutlivesInput() {
        const auto verify =
            []<typename State, typename Input>(
                State& state, std::unique_ptr<Input> item, auto bindable, auto colorValue) {
                QProperty<QColor> color(Qt::red);
                state.setItem(item.get());
                std::invoke(bindable, state).setBinding([&] {
                    return color.value();
                });
                item->setEnabled(false);
                QCOMPARE(state.metaObject()->indexOfProperty("state"), -1);
                QSignalSpy changed(&state, &qml_material::CommonState::targetChanged);
                item.reset();
                QVERIFY(! state.item());
                QVERIFY(! state.target());
                QVERIFY(! state.ctx());
                QVERIFY(! state.colors());
                QCOMPARE(changed.size(), 1);
                QCOMPARE(std::invoke(colorValue, state), QColor(Qt::red));
                color = Qt::blue;
                QCOMPARE(std::invoke(colorValue, state), QColor(Qt::blue));
                auto replacement = std::make_unique<Input>();
                state.setItem(replacement.get());
                QVERIFY(state.ctx());
                replacement->setEnabled(false);
                replacement->setEnabled(true);
                QCOMPARE(std::invoke(colorValue, state), QColor(Qt::blue));
            };
        qml_material::TextFieldState field;
        verify(field,
               std::make_unique<qml_material::TextField>(),
               &qml_material::TextFieldState::bindablePlaceholderColor,
               &qml_material::TextFieldState::placeholderColor);
        qml_material::SliderState slider;
        verify(slider,
               std::make_unique<qml_material::Slider>(),
               &qml_material::CommonState::bindableBackgroundColor,
               &qml_material::CommonState::backgroundColor);
        qml_material::DragHandleState handle;
        verify(handle,
               std::make_unique<QQuickItem>(),
               &qml_material::CommonState::bindableTextColor,
               &qml_material::CommonState::textColor);
    }

    void stateOutlivesContext() {
        qml_material::MaterialState state;
        auto                        target = std::make_unique<QObject>();
        state.setTarget(target.get());
        QVERIFY(state.ctx());
        QVERIFY(state.colors());
        QSignalSpy targetChanged(&state, &qml_material::CommonState::targetChanged);
        QSignalSpy ctxChanged(&state, &qml_material::CommonState::ctxChanged);
        QSignalSpy colorsChanged(&state, &qml_material::CommonState::colorsChanged);
        target.reset();
        QVERIFY(! state.target());
        QVERIFY(! state.ctx());
        QVERIFY(! state.colors());
        QCOMPARE(targetChanged.size(), 1);
        QCOMPARE(ctxChanged.size(), 1);
        QCOMPARE(colorsChanged.size(), 1);

        auto  contextTarget = std::make_unique<QObject>();
        auto* context       = qobject_cast<qml_material::Theme*>(
            qmlAttachedPropertiesObject<qml_material::Theme>(contextTarget.get(), true));
        state.setCtx(context);
        QCOMPARE(state.ctx(), context);
        ctxChanged.clear();
        colorsChanged.clear();
        contextTarget.reset();
        QVERIFY(! state.ctx());
        QVERIFY(! state.colors());
        QCOMPARE(ctxChanged.size(), 1);
        QCOMPARE(colorsChanged.size(), 1);

        auto palette = std::make_unique<qml_material::MdColorMgr>();
        state.setColors(palette.get());
        colorsChanged.clear();
        palette.reset();
        QVERIFY(! state.colors());
        QCOMPARE(colorsChanged.size(), 1);
    }

private:
    QQmlEngine   m_engine;
    QQuickWindow m_window;
};

int run_state_lifetime(int argc, char** argv) {
    StateLifetimeTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "state_lifetime.moc"
