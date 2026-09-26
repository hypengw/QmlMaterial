#include "qml_material/control/adaptive_presenter.hpp"
#include "qml_material/control/drawer.hpp"
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QtTest>
#include <memory>

using namespace qml_material;

namespace
{
struct Scene {
    QQmlEngine                  engine;
    QQuickWindow                window;
    std::unique_ptr<QQuickItem> root;
    AdaptivePresenter*          presenter = nullptr;
    PresentationSite *          side = nullptr, *sheetSite = nullptr, *drawerSite = nullptr;
    Popup*                      sheet   = nullptr;
    Drawer*                     drawer  = nullptr;
    QQuickItem*                 content = nullptr;
    Scene() {
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
Item {
    id: root
    width: 900; height: 700
    property Item detail: Item {
        parent: null
        objectName: "detail"
        implicitWidth: 140; implicitHeight: 180
        property int value: 42
    }
    property MD.AdaptivePresenter coordinator: MD.AdaptivePresenter {
        objectName: "presenter"
        content: root.detail
    }
    MD.PresentationSite {
        objectName: "side"
        presenter: root.coordinator
        width: 280; height: 600
    }
    property MD.PopupBase sheet: MD.PopupBase {
        objectName: "sheet"
        parent: root
        width: 400; height: 500
        deferredCompletion: true
        modal: true
        contentItem: MD.PresentationSite {
            objectName: "sheetSite"
            presenter: root.coordinator
            popup: root.sheet
        }
    }
    property MD.DrawerBase drawer: MD.DrawerBase {
        objectName: "drawer"
        parent: root
        width: 300; height: 700
        deferredCompletion: true
        dragMargin: 30
        contentItem: MD.PresentationSite {
            objectName: "drawerSite"
            presenter: root.coordinator
            popup: root.drawer
            autoOpen: false
        }
    }
})",
                          QUrl("qrc:/adaptive-presenter.qml"));
        root.reset(qobject_cast<QQuickItem*>(component.create()));
        if (! root) {
            qWarning() << component.errors();
            return;
        }
        window.resize(900, 700);
        root->setParentItem(window.contentItem());
        presenter  = root->findChild<AdaptivePresenter*>("presenter");
        content    = root->findChild<QQuickItem*>("detail");
        side       = root->findChild<PresentationSite*>("side");
        sheetSite  = root->findChild<PresentationSite*>("sheetSite");
        drawerSite = root->findChild<PresentationSite*>("drawerSite");
        sheet      = root->findChild<Popup*>("sheet");
        drawer     = root->findChild<Drawer*>("drawer");
    }
};
} // namespace

class AdaptivePresenterTest : public QObject {
    Q_OBJECT
private slots:
    void drawerActivationRequest() {
        Scene s;
        QVERIFY(s.root);
        s.presenter->setDestination(s.side);
        QTRY_COMPARE(s.presenter->currentSite(), s.side);
        s.drawerSite->setActivationEnabled(true);
        QSignalSpy requested(s.drawerSite, &PresentationSite::activationRequested);
        QVERIFY(s.drawer->acceptsDrag({ 1, 100 }));
        s.drawer->open();
        QCOMPARE(requested.count(), 1);
        QVERIFY(! s.drawer->isVisible());
        connect(s.drawerSite, &PresentationSite::activationRequested, s.presenter, [&] {
            s.presenter->setDestination(s.drawerSite);
        });
        s.drawer->pressDrag({ 1, 100 }, 100);
        QVERIFY(s.drawer->wantsDrag({ 40, 100 }));
        s.drawer->startDrag({ 40, 100 });
        QCOMPARE(s.presenter->currentSite(), s.drawerSite);
        s.drawer->moveDrag({ 100, 100 });
        QVERIFY(s.drawer->position() > 0);
        s.drawer->cancelDrag();
        s.drawer->completeExit();
        s.presenter->setDestination(s.side);
        QTRY_COMPARE(s.presenter->currentSite(), s.side);
        s.presenter->setEnabled(false);
        QVERIFY(! s.drawer->acceptsDrag({ 1, 100 }));
        s.drawer->open();
        QVERIFY(! s.drawer->isVisible());
    }
    void navigationRailDrawer() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.NavigationRail {
    width: implicitWidth; height: 700
    forceModal: true
    autoExpand: false
    model: []
})",
                          QUrl("qrc:/rail-presenter.qml"));
        std::unique_ptr<QQuickItem> rail(qobject_cast<QQuickItem*>(component.create()));
        QVERIFY2(rail, qPrintable(component.errorString()));
        QQuickWindow window;
        window.resize(900, 700);
        rail->setParentItem(window.contentItem());
        auto* presenter = rail->findChild<AdaptivePresenter*>();
        auto* drawer    = rail->findChild<Drawer*>();
        QVERIFY(presenter);
        QVERIFY(drawer);
        drawer->setDeferredCompletion(true);
        QTRY_VERIFY(presenter->currentSite());
        auto* inlineSite = presenter->currentSite();
        auto* content    = presenter->content();
        auto* owner      = content->parent();
        QCOMPARE(rail->width(), 96);
        QVERIFY(QMetaObject::invokeMethod(rail.get(), "open"));
        QVERIFY(drawer->entering());
        QVERIFY(presenter->currentSite() != inlineSite);
        QVERIFY(rail->property("drawerOpened").toBool());
        QCOMPARE(rail->width(), 96);
        QCOMPARE(content->parent(), owner);
        drawer->completeEnter();
        drawer->close();
        QVERIFY(presenter->currentSite() != inlineSite);
        drawer->completeExit();
        QTRY_COMPARE(presenter->currentSite(), inlineSite);
        QVERIFY(content->isVisible());
        QCOMPARE(content->parent(), owner);
        drawer->pressDrag({ 1, 100 }, 100);
        drawer->startDrag({ 40, 100 });
        drawer->moveDrag({ 100, 100 });
        QVERIFY(drawer->position() > 0);
        drawer->cancelDrag();
        drawer->completeExit();
        QTRY_COMPARE(presenter->currentSite(), inlineSite);
    }
    void activationOwnerDestroyed() {
        Scene s;
        QVERIFY(s.root);
        s.presenter->setDestination(s.side);
        QTRY_COMPARE(s.presenter->currentSite(), s.side);
        s.drawerSite->setActivationEnabled(true);
        connect(s.drawerSite, &PresentationSite::activationRequested, s.root.get(), [&] {
            delete s.presenter;
            s.presenter = nullptr;
        });
        s.drawer->open();
        QVERIFY(! s.drawer->isVisible());
    }
    void threeSitesAndOwnership() {
        Scene s;
        QVERIFY(s.root);
        auto*      owner = s.content->parent();
        QSignalSpy dismissed(s.presenter, &AdaptivePresenter::dismissed);
        QCOMPARE(s.presenter->currentSite(), nullptr);
        s.presenter->setDestination(s.side);
        QTRY_COMPARE(s.presenter->currentSite(), s.side);
        QCOMPARE(s.content->size(), s.side->size());
        QTRY_COMPARE(s.sheetSite->implicitWidth(), 140);
        QVERIFY(! s.drawer->acceptsDrag({ 1, 100 }));
        s.drawer->open();
        QVERIFY(! s.drawer->isVisible());
        s.presenter->setDestination(s.sheetSite);
        QTRY_COMPARE(s.presenter->status(), AdaptivePresenter::Opening);
        QVERIFY(! s.side->current());
        s.sheet->completeEnter();
        QCOMPARE(s.presenter->status(), AdaptivePresenter::Open);
        s.presenter->setDestination(s.drawerSite);
        QTRY_COMPARE(s.presenter->currentSite(), s.drawerSite);
        QCOMPARE(s.presenter->status(), AdaptivePresenter::Mounted);
        QVERIFY(! s.sheet->isVisible());
        QVERIFY(! s.drawer->isVisible());
        QVERIFY(s.drawer->acceptsDrag({ 1, 100 }));
        s.presenter->present();
        QTRY_VERIFY(s.drawer->entering());
        s.drawer->completeEnter();
        s.drawer->close();
        s.drawer->completeExit();
        QTRY_COMPARE(dismissed.count(), 1);
        QCOMPARE(s.presenter->currentSite(), s.drawerSite);
        QVERIFY(s.drawerSite->current());
        s.presenter->setDestination(s.side);
        QTRY_COMPARE(s.presenter->currentSite(), s.side);
        QCOMPARE(dismissed.count(), 1);
        QCOMPARE(s.content->parent(), owner);
        QCOMPARE(s.content->property("value").toInt(), 42);
        QVERIFY(! s.drawer->acceptsDrag({ 1, 100 }));
    }
    void closingReselection() {
        Scene s;
        QVERIFY(s.root);
        QSignalSpy dismissed(s.presenter, &AdaptivePresenter::dismissed);
        s.presenter->setDestination(s.sheetSite);
        QTRY_VERIFY(s.sheet->entering());
        s.sheet->completeEnter();
        s.presenter->dismiss();
        QTRY_VERIFY(s.sheet->closing());
        s.presenter->present();
        s.sheet->completeExit();
        QTRY_VERIFY(s.sheet->entering());
        QCOMPARE(dismissed.count(), 0);
        s.sheet->completeEnter();
        s.sheet->close();
        s.presenter->setDestination(s.side);
        s.sheet->completeExit();
        QTRY_COMPARE(s.presenter->currentSite(), s.side);
        QCOMPARE(dismissed.count(), 0);
        s.presenter->dismiss();
        QTRY_COMPARE(dismissed.count(), 1);
        QCOMPARE(s.presenter->currentSite(), nullptr);
        QCoreApplication::processEvents();
        QCOMPARE(s.presenter->currentSite(), nullptr);
        s.presenter->present();
        QTRY_COMPARE(s.presenter->currentSite(), s.side);
    }
    void drawerGestureCancellation() {
        Scene s;
        QVERIFY(s.root);
        QSignalSpy dismissed(s.presenter, &AdaptivePresenter::dismissed);
        s.presenter->setDestination(s.drawerSite);
        QTRY_COMPARE(s.presenter->currentSite(), s.drawerSite);
        s.drawer->pressDrag({ 1, 100 }, 100);
        s.drawer->startDrag({ 30, 100 });
        s.drawer->moveDrag({ 80, 100 });
        QCoreApplication::processEvents();
        QVERIFY(s.drawer->entering());
        QVERIFY(s.drawer->position() > 0 && s.drawer->position() < 1);
        s.drawer->cancelDrag();
        s.drawer->completeExit();
        QTRY_COMPARE(s.presenter->status(), AdaptivePresenter::Mounted);
        QCOMPARE(dismissed.count(), 0);
        s.drawer->open();
        s.drawer->completeEnter();
        s.drawer->close();
        s.drawer->completeExit();
        QTRY_COMPARE(dismissed.count(), 1);
    }
    void suspensionAndReentry() {
        Scene s;
        QVERIFY(s.root);
        QSignalSpy dismissed(s.presenter, &AdaptivePresenter::dismissed);
        s.presenter->setDestination(s.sheetSite);
        QTRY_VERIFY(s.sheet->entering());
        s.presenter->setEnabled(false);
        QTRY_COMPARE(s.presenter->currentSite(), nullptr);
        QVERIFY(! s.sheet->isVisible());
        QCOMPARE(dismissed.count(), 0);
        s.presenter->setEnabled(true);
        QTRY_VERIFY(s.sheet->entering());
        s.presenter->setEnabled(false);
        QVERIFY(! s.sheet->presentationAllowed());
        s.presenter->setEnabled(true);
        QTRY_VERIFY(s.sheet->entering());
        QVERIFY(s.sheet->presentationAllowed());
        s.sheet->completeEnter();
        auto connection =
            connect(s.sheet, &Popup::closed, s.presenter, &AdaptivePresenter::present);
        s.sheet->close();
        s.sheet->completeExit();
        QTRY_VERIFY(s.sheet->entering());
        QCOMPARE(dismissed.count(), 0);
        disconnect(connection);
        connect(s.presenter, &AdaptivePresenter::aboutToRelocate, s.root.get(), [&] {
            s.presenter->setDestination(s.side);
        });
        s.presenter->setDestination(s.drawerSite);
        QTRY_COMPARE(s.presenter->currentSite(), s.side);
        QCOMPARE(dismissed.count(), 0);
    }
    void conflictsAndInvalidDestinations() {
        Scene s;
        QVERIFY(s.root);
        s.presenter->setDestination(s.side);
        QTRY_COMPARE(s.presenter->currentSite(), s.side);
        PresentationSite foreign;
        s.presenter->setDestination(&foreign);
        QTRY_COMPARE(s.presenter->status(), AdaptivePresenter::Error);
        QCOMPARE(s.presenter->currentSite(), s.side);
        QCOMPARE(s.content->parentItem()->parentItem(), s.side);
        QObject owner;
        QVERIFY(! s.sheet->acquirePresentation(&owner));
        s.sheetSite->setPopup(nullptr);
        QVERIFY(s.sheet->acquirePresentation(&owner));
        s.sheetSite->setPopup(s.sheet);
        s.presenter->setDestination(s.sheetSite);
        QTRY_COMPARE(s.presenter->status(), AdaptivePresenter::Error);
        QCOMPARE(s.presenter->currentSite(), s.side);
        s.sheet->releasePresentation(&owner);
        QTRY_COMPARE(s.presenter->currentSite(), s.sheetSite);
        s.presenter->setDestination(s.side);
        QTRY_COMPARE(s.presenter->currentSite(), s.side);
        QQuickWindow     otherWindow;
        PresentationSite otherSite(otherWindow.contentItem());
        otherSite.setPresenter(s.presenter);
        s.presenter->setDestination(&otherSite);
        QTRY_COMPARE(s.presenter->status(), AdaptivePresenter::Error);
        QCOMPARE(s.presenter->currentSite(), s.side);
    }
    void dependencyDestruction_data() {
        QTest::addColumn<int>("kind");
        for (int i = 0; i < 4; ++i) QTest::newRow(qPrintable(QString::number(i))) << i;
    }
    void dependencyDestruction() {
        QFETCH(int, kind);
        Scene s;
        QVERIFY(s.root);
        QSignalSpy dismissed(s.presenter, &AdaptivePresenter::dismissed);
        s.presenter->setDestination(s.sheetSite);
        QTRY_VERIFY(s.sheet->entering());
        QPointer<QQuickItem> alive(s.content);
        if (kind == 0) delete s.content;
        if (kind == 1) delete s.sheetSite;
        if (kind == 2) delete s.sheet;
        if (kind == 3) delete s.presenter;
        QCoreApplication::processEvents();
        QCOMPARE(dismissed.count(), 0);
        if (kind != 0) QVERIFY(alive);
        if (kind != 3) QCOMPARE(s.presenter->currentSite(), nullptr);
    }
    void externalReparentAndCycle() {
        Scene s;
        QVERIFY(s.root);
        s.presenter->setDestination(s.side);
        QTRY_COMPARE(s.presenter->currentSite(), s.side);
        QTest::ignoreMessage(
            QtWarningMsg,
            QRegularExpression(".*target was reparented outside its controlling ItemProxy"));
        s.content->setParentItem(s.root.get());
        QTRY_COMPARE(s.presenter->status(), AdaptivePresenter::Error);
        QCOMPARE(s.presenter->currentSite(), nullptr);
        QCOMPARE(s.content->parentItem(), s.root.get());
        s.presenter->present();
        QTRY_COMPARE(s.presenter->currentSite(), s.side);
        PresentationSite cyclic(s.content);
        cyclic.setPresenter(s.presenter);
        s.presenter->setDestination(&cyclic);
        QTRY_COMPARE(s.presenter->status(), AdaptivePresenter::Error);
        QCOMPARE(s.presenter->currentSite(), s.side);
        QVERIFY(s.presenter->errorString().contains("cycle"));
    }
    void hostWindowAvailability() {
        Scene s;
        QVERIFY(s.root);
        s.root->setParentItem(nullptr);
        s.presenter->setDestination(s.sheetSite);
        s.presenter->present();
        QCoreApplication::processEvents();
        QCOMPARE(s.presenter->currentSite(), nullptr);
        s.root->setParentItem(s.window.contentItem());
        QTRY_VERIFY(s.sheet->entering());
        s.sheet->completeEnter();
        QSignalSpy dismissed(s.presenter, &AdaptivePresenter::dismissed);
        s.root->setParentItem(nullptr);
        QTRY_COMPARE(s.presenter->currentSite(), nullptr);
        QCOMPARE(dismissed.count(), 0);
        s.root->setParentItem(s.window.contentItem());
        QTRY_VERIFY(s.sheet->entering());
    }
    void zeroDurationAndDestructiveCallbacks() {
        Scene s;
        QVERIFY(s.root);
        s.sheet->setDeferredCompletion(false);
        s.presenter->setDestination(s.sheetSite);
        QTRY_COMPARE(s.presenter->status(), AdaptivePresenter::Open);
        QSignalSpy                  dismissed(s.presenter, &AdaptivePresenter::dismissed);
        QPointer<AdaptivePresenter> presenter(s.presenter);
        connect(s.presenter, &AdaptivePresenter::dismissed, s.root.get(), [&] {
            delete presenter;
        });
        s.presenter->dismiss();
        QTRY_VERIFY(! presenter);
        QCOMPARE(dismissed.count(), 1);
        QVERIFY(! s.sheet->isVisible());
        QVERIFY(! s.content->parentItem());
    }
    void destroyedWindowAndLeaseOwner() {
        Scene s;
        QVERIFY(s.root);
        auto window = std::make_unique<QQuickWindow>();
        s.root->setParentItem(window->contentItem());
        s.presenter->setDestination(s.drawerSite);
        s.presenter->present();
        QTRY_VERIFY(s.drawer->entering());
        QSignalSpy dismissed(s.presenter, &AdaptivePresenter::dismissed);
        window.reset();
        QTRY_COMPARE(s.presenter->currentSite(), nullptr);
        QCOMPARE(dismissed.count(), 0);
        s.drawerSite->setPresenter(nullptr);
        s.root->setParentItem(s.window.contentItem());
        auto owner = std::make_unique<QObject>();
        QVERIFY(s.drawer->acquirePresentation(owner.get()));
        s.drawer->open();
        QVERIFY(! s.drawer->isVisible());
        s.drawer->setPresentationAllowed(owner.get(), true);
        s.drawer->open();
        QVERIFY(s.drawer->isVisible());
        owner.reset();
        QVERIFY(! s.drawer->isVisible());
        QVERIFY(s.drawer->presentationAllowed());
        s.drawer->open();
        QVERIFY(s.drawer->isVisible());
    }
    void relocationRevalidatesTarget() {
        Scene s;
        QVERIFY(s.root);
        s.presenter->setDestination(s.side);
        QTRY_COMPARE(s.presenter->currentSite(), s.side);
        PresentationSite next(s.root.get());
        next.setPresenter(s.presenter);
        connect(s.presenter, &AdaptivePresenter::aboutToRelocate, &next, [&] {
            next.setParentItem(s.content);
        });
        s.presenter->setDestination(&next);
        QTRY_COMPARE(s.presenter->status(), AdaptivePresenter::Error);
        QCOMPARE(s.presenter->currentSite(), s.side);
    }
    void destructionDuringAcquisition() {
        Scene s;
        QVERIFY(s.root);
        QPointer<AdaptivePresenter> presenter(s.presenter);
        connect(s.side, &PresentationSite::currentChanged, s.root.get(), [&] {
            if (s.side->current()) delete presenter;
        });
        s.presenter->setDestination(s.side);
        QTRY_VERIFY(! presenter);
        QVERIFY(! s.side->current());
        QVERIFY(! s.content->parentItem());
    }
    void bottomSheetViewport() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
Item {
    id: root
    width: 900; height: 700
    property Item detail: Item { parent: null; property int value: 42 }
    property MD.AdaptivePresenter presenter: MD.AdaptivePresenter {
        content: root.detail
        destination: root.narrow ? sheetSite : side
    }
    property bool narrow: false
    MD.PresentationSite { id: side; presenter: root.presenter; width: 280; height: 700 }
    property MD.BottomSheet sheet: MD.BottomSheet {
        id: sheet
        parent: root
        maxSheetWidth: 400
        wideSideMargin: 24
        preferredContentHeight: root.height
        animationDuration: 0
        MD.PresentationSite {
            id: sheetSite
            presenter: root.presenter
            popup: sheet
            width: sheet.contentViewportWidth
            height: sheet.contentViewportHeight
        }
    }
})",
                          QUrl("qrc:/adaptive-sheet.qml"));
        std::unique_ptr<QQuickItem> root(qobject_cast<QQuickItem*>(component.create()));
        QVERIFY2(root, qPrintable(component.errorString()));
        QQuickWindow window;
        root->setParentItem(window.contentItem());
        window.resize(900, 700);
        auto* presenter = root->property("presenter").value<AdaptivePresenter*>();
        auto* sheet     = root->property("sheet").value<Popup*>();
        auto* detail    = root->property("detail").value<QQuickItem*>();
        QVERIFY(presenter && sheet && detail);
        QTRY_COMPARE(detail->width(), 280);
        root->setProperty("narrow", true);
        QTRY_VERIFY(sheet->isOpened());
        QCOMPARE(detail->width(), 400);
        QCOMPARE(detail->height(), sheet->property("contentViewportHeight").toReal());
        QSignalSpy dismissed(presenter, &AdaptivePresenter::dismissed);
        root->setProperty("narrow", false);
        QTRY_COMPARE(detail->width(), 280);
        QVERIFY(! sheet->isVisible());
        QCOMPARE(dismissed.count(), 0);
        QCOMPARE(detail->property("value").toInt(), 42);
    }
};

int run_adaptive_presenter(int argc, char** argv) {
    AdaptivePresenterTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "adaptive_presenter.moc"
