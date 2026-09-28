#include "qml_material/control/search_bar.hpp"
#include "qml_material/control/search_view.hpp"
#include "qml_material/control/panel.hpp"
#include "qml_material/control/text_field.hpp"
#include "qml_material/input/search_state.hpp"
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include <QtTest>
#include <memory>

using namespace qml_material;

namespace
{
struct Scene {
    QQmlEngine                  engine;
    QQuickWindow                window;
    std::unique_ptr<QQuickItem> root;
    SearchBar*                  bar  = nullptr;
    SearchView*                 view = nullptr;
    QString                     errors;
    Scene(bool initiallyVisible = false) {
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(QByteArray(R"(
import QtQuick
import Qcm.Material as MD
Item {
    width: 800; height: 600
    MD.SearchBar {
        id: bar; objectName: "bar"
        x: 40; y: 30; width: 320
        searchText: "initial"
    }
    property MD.SearchView view: MD.SearchView {
        objectName: "view"
        searchBar: bar
        deferredCompletion: true
        autoShowKeyboard: false
        visible: INITIAL_VISIBLE
        MD.Button {
            objectName: "result"
            text: "Result"; width: parent.width
            onClicked: view.searchText = "selected"
        }
    }
}
)")
                              .replace("INITIAL_VISIBLE", initiallyVisible ? "true" : "false"),
                          QUrl("qrc:/search-scene.qml"));
        root.reset(qobject_cast<QQuickItem*>(component.create()));
        errors = component.errorString();
        if (! root) return;
        window.resize(800, 600);
        root->setParentItem(window.contentItem());
        bar  = root->findChild<SearchBar*>("bar");
        view = root->findChild<SearchView*>("view");
    }
};
} // namespace

class SearchViewTest : public QObject {
    Q_OBJECT
private slots:
    void init() { QTest::failOnWarning(QRegularExpression(".*")); }
    void initialAndConstraints() {
        Scene s(true);
        QVERIFY2(s.root, qPrintable(s.errors));
        QVERIFY(s.view->entering());
        QCOMPARE(s.view->expansion(), 1);
        s.view->completeEnter();
        s.window.resize(180, 100);
        QVERIFY(s.view->width() <= 180);
        QVERIFY(s.view->height() <= 100);
        QVERIFY(s.view->presentationRect().right() <= 180);
        QVERIFY(s.view->presentationRect().bottom() <= 100);
        QVERIFY(s.view->contentItem()->height() >= 0);
        s.view->setPresentation(SearchView::FullScreen);
        QCOMPARE(s.view->surfaceItem()->size(), QSizeF(180, 100));
        s.bar->setVisible(false);
        QVERIFY(! s.view->isVisible());
    }
    void independentActionAndTouch() {
        Scene s;
        QVERIFY2(s.root, qPrintable(s.errors));
        s.window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&s.window));
        QTest::mouseClick(&s.window, Qt::LeftButton, Qt::NoModifier, QPoint(336, 58));
        QCOMPARE(s.bar->searchText(), "");
        QVERIFY(! s.view->isVisible());
        s.bar->setEnabled(false);
        QTest::mouseClick(&s.window, Qt::LeftButton, Qt::NoModifier, QPoint(190, 58));
        QVERIFY(! s.view->isVisible());
        s.bar->setEnabled(true);
        static auto device = QTest::createTouchDevice();
        auto        touch  = QTest::touchEvent(&s.window, device, false);
        touch.press(0, QPoint(190, 58), &s.window).commit();
        QQuickWindowPrivate::get(&s.window)->deliveryAgentPrivate()->flushFrameSynchronousEvents(
            &s.window);
        touch.release(0, QPoint(190, 58), &s.window).commit();
        QQuickWindowPrivate::get(&s.window)->deliveryAgentPrivate()->flushFrameSynchronousEvents(
            &s.window);
        QVERIFY(s.view->entering());
        s.view->completeEnter();
        auto result = s.root->findChild<AbstractButton*>("result");
        QVERIFY(result);
        QTest::mouseClick(
            &s.window,
            Qt::LeftButton,
            Qt::NoModifier,
            result->mapToScene(QPointF(result->width() / 2, result->height() / 2)).toPoint());
        QCOMPARE(s.view->searchText(), "selected");
        QVERIFY(s.view->isOpened());
        QTest::mouseClick(&s.window, Qt::LeftButton, Qt::NoModifier, QPoint(750, 550));
        QVERIFY(s.view->closing());
        s.view->completeExit();
        QVERIFY(! s.view->isVisible());
    }
    void associationLifetime() {
        Scene s;
        QVERIFY2(s.root, qPrintable(s.errors));
        SearchState replacement;
        replacement.setQuery("replacement");
        s.bar->setSearchState(&replacement);
        QCOMPARE(s.view->searchState(), &replacement);
        QCOMPARE(s.view->searchText(), "replacement");
        QSignalSpy submitted(s.bar, &SearchBar::accepted);
        s.view->submit();
        QCOMPARE(submitted.size(), 1);
        s.view->open();
        delete s.bar;
        s.bar = nullptr;
        QVERIFY(! s.view->isVisible());
        QVERIFY(! s.view->searchBar());
        QVERIFY(s.view->searchState());
        s.view->setSearchText("standalone");
        QCOMPARE(s.view->searchText(), "standalone");
    }
    void deletionOnNotification() {
        auto                 view = new SearchView;
        SearchBar            bar;
        QPointer<SearchView> guard(view);
        connect(&bar, &SearchBar::searchViewChanged, &bar, [&] {
            if (bar.searchView()) delete view;
        });
        view->setSearchBar(&bar);
        QVERIFY(! guard);
        QVERIFY(! bar.searchView());
    }
    void reassociationReentry() {
        SearchView view;
        SearchBar  first, second;
        view.setSearchBar(&first);
        connect(&first, &SearchBar::searchViewChanged, &first, [&] {
            if (! first.searchView()) view.setSearchBar(nullptr);
        });
        view.setSearchBar(&second);
        QVERIFY(! view.searchBar());
        QVERIFY(! first.searchView());
        QVERIFY(! second.searchView());
        view.setSearchText("local");
        QCOMPARE(view.searchText(), "local");
    }
    void transitionCompletionAndFocus() {
        Scene s;
        QVERIFY2(s.root, qPrintable(s.errors));
        s.window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&s.window));
        s.view->setDeferredCompletion(false);
        QSignalSpy opened(s.view, &Popup::opened);
        QSignalSpy closed(s.view, &Popup::closed);
        s.view->open();
        QTRY_VERIFY(s.view->isOpened());
        QCOMPARE(s.view->expansion(), 1);
        QCOMPARE(opened.size(), 1);
        auto result = s.root->findChild<AbstractButton*>("result");
        result->forceActiveFocus();
        s.view->setSearchText("updated");
        QVERIFY(result->hasActiveFocus());
        s.view->close();
        s.view->open();
        QTRY_VERIFY(s.view->isOpened());
        QCOMPARE(s.view->expansion(), 1);
        QCOMPARE(closed.size(), 0);
        s.view->close();
        QTRY_VERIFY(! s.view->isVisible());
        QCOMPARE(s.view->expansion(), 0);
        QCOMPARE(closed.size(), 1);
    }
    void presentation() {
        Scene s;
        QVERIFY2(s.root, qPrintable(s.errors));
        QVERIFY(s.bar && s.view);
        QCOMPARE(s.bar->searchView(), s.view);
        QCOMPARE(s.view->searchState(), s.bar->searchState());
        s.view->open();
        QVERIFY(s.view->entering());
        QCOMPARE(s.view->surfaceItem()->size(), s.bar->size());
        s.view->setExpansion(0.5);
        QCOMPARE(s.view->surfaceItem()->width(), 320);
        QCOMPARE(s.view->surfaceItem()->height(), (s.view->height() + s.bar->height()) / 2);
        s.view->completeEnter();
        QCOMPARE(s.view->expansion(), 1);
        QCOMPARE(s.view->surfaceItem()->position(), QPointF(40, 30));
        QCOMPARE(s.view->surfaceItem()->width(), 320);
        s.view->setPresentation(SearchView::FullScreen);
        QCOMPARE(s.view->surfaceItem()->size(), QSizeF(800, 600));
        QCOMPARE(s.view->surfaceItem()->position(), QPointF(0, 0));
        auto result = s.root->findChild<AbstractButton*>("result");
        QVERIFY(result);
        QCOMPARE(result->width(), 800);
        QMetaObject::invokeMethod(result, "clicked");
        QCOMPARE(s.bar->searchText(), "selected");
        QSignalSpy accepted(s.view, &SearchView::accepted);
        s.view->submit();
        QCOMPARE(accepted.size(), 1);
        QVERIFY(s.view->isOpened());
        s.view->close();
        s.view->setExpansion(0.4);
        s.view->open();
        QCOMPARE(s.view->expansion(), 0.4);
        s.view->completeEnter();
        QCOMPARE(s.view->expansion(), 1);
        s.view->close();
        s.view->completeExit();
        QCOMPARE(s.view->expansion(), 0);
        QVERIFY(! s.view->isVisible());
    }
    void focusAndInput() {
        Scene s;
        QVERIFY2(s.root, qPrintable(s.errors));
        s.window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&s.window));
        s.bar->forceActiveFocus();
        QVERIFY(! s.view->isVisible());
        QTest::mouseClick(&s.window, Qt::LeftButton, Qt::NoModifier, QPoint(190, 58));
        QVERIFY(s.view->entering());
        auto editor = qobject_cast<TextField*>(s.view->inputField());
        QVERIFY(editor);
        QVERIFY(editor->hasActiveFocus());
        s.view->completeEnter();
        editor->selectAll();
        for (const auto key : { Qt::Key_Q, Qt::Key_U, Qt::Key_E, Qt::Key_R, Qt::Key_Y })
            QTest::keyClick(&s.window, key);
        QCOMPARE(s.bar->searchText(), "query");
        QCOMPARE(s.view->searchText(), "query");
        s.view->clear();
        QCOMPARE(editor->text(), "");
        QVERIFY(editor->hasActiveFocus());
        QTest::keyClick(&s.window, Qt::Key_Escape);
        QVERIFY(s.view->closing());
        s.view->completeExit();
        QVERIFY(s.bar->hasActiveFocus());
        QVERIFY(! s.view->isVisible());
        QTest::keyClick(&s.window, Qt::Key_Space);
        QVERIFY(s.view->entering());
    }
    void sharedQuery() {
        SearchBar   first, second;
        SearchState state;
        first.setSearchState(&state);
        second.setSearchState(&state);
        QSignalSpy changed(&second, &SearchBar::searchTextChanged);
        first.setSearchText("query");
        QCOMPARE(second.searchText(), "query");
        QCOMPARE(changed.size(), 1);
        QProperty<QString> source("bound");
        state.bindableQuery().setBinding([&] {
            return source.value();
        });
        QCOMPARE(first.searchText(), "bound");
        source = "updated";
        QCOMPARE(second.searchText(), "updated");
        first.setSearchText("");
        QVERIFY(! state.bindableQuery().hasBinding());
        QCOMPARE(second.searchText(), "");
    }
    void stateLifetime() {
        SearchBar bar;
        auto      original = bar.searchState();
        bar.setSearchText("local");
        auto state = std::make_unique<SearchState>();
        state->setQuery("shared");
        bar.setSearchState(state.get());
        QCOMPARE(bar.searchText(), "shared");
        state.reset();
        QCOMPARE(bar.searchState(), original);
        QCOMPARE(bar.searchText(), "local");
        SearchState replacement;
        connect(&bar, &SearchBar::searchStateChanged, &bar, [&] {
            if (bar.searchState() == &replacement) bar.resetSearchState();
        });
        bar.setSearchState(&replacement);
        QCOMPARE(bar.searchState(), original);
    }
    void legacyInput() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.SearchBar { width: 320; searchText: "initial" }
)",
                          QUrl("qrc:/search-legacy.qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto bar = qobject_cast<SearchBar*>(object.get());
        QVERIFY(bar);
        auto field = bar->findChild<TextField*>();
        QVERIFY(field);
        QCOMPARE(field->text(), "initial");
        QSignalSpy accepted(bar, &SearchBar::accepted);
        QMetaObject::invokeMethod(field, "accepted");
        QCOMPARE(accepted.size(), 1);
        field->setText("edited");
        QMetaObject::invokeMethod(field, "textEdited");
        QCOMPARE(bar->searchText(), "edited");
        bar->setSearchText("external");
        QCOMPARE(field->text(), "external");
        bar->setSearchText("");
        QCOMPARE(field->text(), "");
        QCOMPARE(bar->implicitHeight(), 56);
    }
};

int run_search_view(int argc, char** argv) {
    SearchViewTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "search_view.moc"
