#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QStandardItemModel>
#include <QtQuick/private/qquickflickable_p.h>
#include "qml_material/carousel/carousel_item.hpp"
#include "qml_material/carousel/carousel_view.hpp"
#include "../input_test.hpp"

using namespace qml_material;

class CarouselPositionTest : public QObject {
    Q_OBJECT
private:
    std::unique_ptr<QQmlEngine> m_engine;
    QQuickWindow                m_window;
    QPointer<QQuickItem>        m_root;
    QPointer<CarouselView>      m_view;

    void create(const QVariantMap& properties = {}, bool wrapper = false) {
        QQmlComponent    component(m_engine.get());
        const QByteArray type = wrapper ? "Carousel" : "CarouselView";
        component.setData("import QtQuick\nimport Qcm.Material as MD\nMD." + type + R"( {
            model: 10
            initialItem: 3
            reduceMotion: true
            delegate: MD.CarouselItem {
                required property var model
                property string title: ""
                objectName: "item" + index
            }
        })",
                          QUrl());
        m_root = qobject_cast<QQuickItem*>(component.createWithInitialProperties(properties));
        QVERIFY2(m_root, qPrintable(component.errorString()));
        m_root->setParentItem(m_window.contentItem());
        m_view = wrapper ? m_root->findChild<CarouselView*>() : qobject_cast<CarouselView*>(m_root);
        QVERIFY(m_view);
    }

    qreal offset() const {
        auto* flick = qobject_cast<QQuickFlickable*>(m_view->flickable());
        return m_view->orientation() == Qt::Horizontal ? flick->contentX() : flick->contentY();
    }

    CarouselLayoutOutput layoutAt(qreal offset) const {
        CarouselLayoutInput input;
        input.layout      = m_view->layout();
        input.orientation = Qt::Orientation(m_view->orientation());
        input.viewport_size =
            input.orientation == Qt::Horizontal ? m_view->width() : m_view->height();
        input.cross_size = input.orientation == Qt::Horizontal ? m_view->height() : m_view->width();
        input.scroll_offset         = offset;
        input.item_extent           = m_view->itemExtent();
        input.spacing               = m_view->spacing();
        input.small_item_min        = m_view->minSmallItemWidth();
        input.small_item_max        = m_view->maxSmallItemWidth();
        input.content_padding_start = m_view->contentPaddingStart();
        input.content_padding_end   = m_view->contentPaddingEnd();
        input.content_padding_cross = m_view->contentPaddingCross();
        input.item_corner_radius    = m_view->itemCornerRadius();
        input.count                 = m_view->count();
        input.reduce_motion         = m_view->reduceMotion();
        input.parallax_ratio        = 0;
        input.item_aspects.fill(1, input.count);
        return CarouselStrategy::compute(input);
    }

    qreal snapOffset(int index) const {
        const auto output = layoutAt(0);
        return index == m_view->count() - 1 ? output.end_snap_offset
                                            : output.snap_offsets.value(index);
    }

    void verifyPosition(int index) {
        QCOMPARE(m_view->currentIndex(), index);
        QVERIFY(m_view->width() > 0 && m_view->height() > 0);
        const qreal expected = snapOffset(index);
        QVERIFY2(qAbs(offset() - expected) < 0.01,
                 qPrintable(QString("offset %1, expected %2").arg(offset()).arg(expected)));
        const auto output = layoutAt(expected);
        QVERIFY(! output.items.isEmpty());
        for (const auto& geometry : output.items) {
            CarouselItem* item = nullptr;
            for (auto* candidate : m_view->findChildren<CarouselItem*>())
                if (candidate->carousel() == m_view &&
                    candidate->objectName() == "item" + QString::number(geometry.index))
                    item = candidate;
            QVERIFY(item && item->isVisible());
            const qreal position = m_view->orientation() == Qt::Horizontal ? item->x() : item->y();
            QVERIFY(qAbs(position - geometry.position) < 0.01);
        }
    }

private slots:
    void focalGeometry_data() {
        QTest::addColumn<int>("layout");
        QTest::addColumn<int>("orientation");
        QTest::addColumn<bool>("mirrored");
        for (int layout : { CarouselLayoutId::MultiBrowse,
                            CarouselLayoutId::Hero,
                            CarouselLayoutId::HeroCenter })
            for (int orientation : { int(Qt::Horizontal), int(Qt::Vertical) })
                for (bool mirrored : { false, true })
                    QTest::newRow(
                        qPrintable(QString("%1-%2-%3").arg(layout).arg(orientation).arg(mirrored)))
                        << layout << orientation << mirrored;
    }
    void focalGeometry() {
        QFETCH(int, layout);
        QFETCH(int, orientation);
        QFETCH(bool, mirrored);
        create({ { "layout", layout },
                 { "orientation", orientation },
                 { "reduceMotion", false },
                 { "mirrored", mirrored },
                 { "width", 480 },
                 { "height", 480 } });
        QVERIFY(m_view);
        m_window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&m_window));
        for (const QSizeF size : { QSizeF(480, 480), QSizeF(720, 400), QSizeF(360, 360) }) {
            m_root->setSize(size);
            for (int index : { 0, 4, 9 }) {
                m_view->setCurrentIndex(index);
                verifyPosition(index);
                auto* item = m_view->findChild<CarouselItem*>("item" + QString::number(index));
                QVERIFY(item && item->isVisible());
                const auto rect = item->mapRectToItem(m_view, item->boundingRect());
                QVERIFY2(rect.left() >= -.01 && rect.right() <= m_view->width() + .01 &&
                             rect.top() >= -.01 && rect.bottom() <= m_view->height() + .01,
                         qPrintable(QString("target %1 outside viewport: %2,%3 %4x%5")
                                        .arg(index)
                                        .arg(rect.x())
                                        .arg(rect.y())
                                        .arg(rect.width())
                                        .arg(rect.height())));
                const auto output = layoutAt(offset());
                for (const auto& geometry : output.items) {
                    if (geometry.index != index) continue;
                    QCOMPARE(geometry.size_class, 2);
                    const qreal expected =
                        mirrored && orientation == Qt::Horizontal
                            ? m_view->width() - (geometry.position - offset()) - geometry.size
                            : geometry.position - offset();
                    QVERIFY(qAbs((orientation == Qt::Horizontal ? rect.x() : rect.y()) - expected) <
                            .01);
                }
                if (mirrored && orientation == Qt::Horizontal) {
                    const auto origin = item->mapToItem(m_view, QPointF());
                    const auto right  = item->mapToItem(m_view, QPointF(1, 0));
                    QVERIFY(right.x() > origin.x());
                }
            }
        }
    }
    void init() {
        QTest::failOnWarning(QRegularExpression(".*"));
        m_engine = std::make_unique<QQmlEngine>();
        m_engine->addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        m_window.resize(720, 480);
    }
    void cleanup() {
        delete m_root.data();
        flushInteractions();
        m_engine.reset();
    }
    void initialItem_data() {
        QTest::addColumn<int>("layout");
        QTest::addColumn<int>("orientation");
        QTest::addColumn<int>("index");
        QTest::addColumn<bool>("wrapper");
        for (int layout = 0; layout < 6; ++layout)
            for (int orientation : { int(Qt::Horizontal), int(Qt::Vertical) })
                for (int index : { 3, 9 })
                    for (bool wrapper : { false, true })
                        QTest::newRow(qPrintable(QString("%1-%2-%3-%4")
                                                     .arg(layout)
                                                     .arg(orientation)
                                                     .arg(index)
                                                     .arg(wrapper)))
                            << layout << orientation << index << wrapper;
    }
    void initialItem() {
        QFETCH(int, layout);
        QFETCH(int, orientation);
        QFETCH(int, index);
        QFETCH(bool, wrapper);
        create({ { "layout", layout }, { "orientation", orientation }, { "initialItem", index } },
               wrapper);
        QVERIFY(m_view);
        QCOMPARE(m_view->currentIndex(), index);
        m_root->setSize({ 640, 320 });
        m_window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&m_window));
        QTRY_VERIFY(m_view->width() > 0 && m_view->height() > 0);
        verifyPosition(index);
    }
    void resizeSnappedPosition_data() {
        QTest::addColumn<int>("layout");
        QTest::addColumn<int>("orientation");
        QTest::addColumn<bool>("wrapper");
        for (int layout :
             { CarouselLayoutId::UncontainedMultiAspect, CarouselLayoutId::FullScreen })
            for (int orientation : { int(Qt::Horizontal), int(Qt::Vertical) })
                for (bool wrapper : { false, true })
                    QTest::newRow(
                        qPrintable(QString("%1-%2-%3").arg(layout).arg(orientation).arg(wrapper)))
                        << layout << orientation << wrapper;
    }
    void resizeSnappedPosition() {
        QFETCH(int, layout);
        QFETCH(int, orientation);
        QFETCH(bool, wrapper);
        create({ { "layout", layout },
                 { "orientation", orientation },
                 { "width", 640 },
                 { "height", 320 } },
               wrapper);
        QVERIFY(m_view);
        m_window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&m_window));
        QTRY_COMPARE(m_view->width(), 640);
        QTRY_COMPARE(m_view->height(), 320);
        verifyPosition(3);
        m_root->setSize({ 580, 280 });
        QTRY_COMPARE(m_view->width(), 580);
        QTRY_COMPARE(m_view->height(), 280);
        verifyPosition(3);
    }
    void delayedModel() {
        create({ { "model", 0 }, { "width", 640 }, { "height", 240 } });
        QVERIFY(m_view);
        m_view->setModel(10);
        verifyPosition(3);
    }
    void requestBeforeGeometry() {
        create();
        QVERIFY(m_view);
        m_view->setCurrentIndex(5);
        m_root->setSize({ 640, 240 });
        verifyPosition(5);
        m_view->setInitialItem(7);
        verifyPosition(5);
    }
    void requestBeforeModel() {
        create({ { "model", 0 }, { "width", 640 }, { "height", 240 } });
        QVERIFY(m_view);
        m_view->setCurrentIndex(5);
        m_view->setModel(10);
        verifyPosition(5);
    }
    void explicitInitialPosition() {
        create({ { "currentIndex", 5 }, { "width", 640 }, { "height", 240 } });
        QVERIFY(m_view);
        verifyPosition(5);
    }
    void animatedRequestBeforeGeometry() {
        create();
        QVERIFY(m_view);
        m_view->setCurrentIndexAnimated(5);
        m_root->setSize({ 640, 240 });
        verifyPosition(5);
    }
    void initialPositionIsNotReapplied() {
        create({ { "width", 640 }, { "height", 240 } });
        QVERIFY(m_view);
        verifyPosition(3);
        m_view->setCurrentIndex(6);
        m_view->setModel(0);
        QCOMPARE(m_view->currentIndex(), 0);
        m_view->setModel(10);
        verifyPosition(0);
    }
    void initialPositionClampsToFirstModel() {
        create({ { "model", 0 }, { "width", 640 }, { "height", 240 }, { "initialItem", 30 } });
        QVERIFY(m_view);
        m_view->setModel(4);
        verifyPosition(3);
        m_view->setModel(10);
        verifyPosition(3);
    }
    void requestSurvivesModelReplacementBeforeGeometry() {
        create();
        QVERIFY(m_view);
        m_view->setCurrentIndex(7);
        m_view->setModel(4);
        QCOMPARE(m_view->currentIndex(), 3);
        m_view->setModel(10);
        QCOMPARE(m_view->currentIndex(), 7);
        m_root->setSize({ 640, 240 });
        verifyPosition(7);
    }
    void requestSurvivesItemModelChanges() {
        QStandardItemModel model;
        model.setItemRoleNames({ { Qt::DisplayRole, "title" } });
        create({ { "model", QVariant::fromValue<QObject*>(&model) },
                 { "width", 640 },
                 { "height", 240 } });
        QVERIFY(m_view);
        for (int i = 0; i < 8; ++i) model.appendRow(new QStandardItem(QString::number(i)));
        // The first nonempty layout consumes and clamps the initial request.
        verifyPosition(0);
        m_root->setSize({ 0, 0 });
        m_view->setCurrentIndex(5);
        model.insertRow(0, new QStandardItem("new"));
        m_root->setSize({ 640, 240 });
        verifyPosition(6);
    }
    void resizeDoesNotSnapFreePosition() {
        create({ { "layout", CarouselLayoutId::FullScreen }, { "width", 640 }, { "height", 240 } });
        QVERIFY(m_view);
        verifyPosition(3);
        auto*       flick      = qobject_cast<QQuickFlickable*>(m_view->flickable());
        const qreal freeOffset = offset() + 35;
        flick->setContentX(freeOffset);
        m_root->setSize({ 720, 280 });
        QCOMPARE(offset(), freeOffset);
    }
    void modelIndexNotificationRetiresOldItems() {
        QStandardItemModel model;
        model.setItemRoleNames({ { Qt::DisplayRole, "title" } });
        for (int i = 0; i < 8; ++i) model.appendRow(new QStandardItem(QString::number(i)));
        create({ { "model", QVariant::fromValue<QObject*>(&model) },
                 { "width", 640 },
                 { "height", 240 } });
        QVERIFY(m_view);
        verifyPosition(3);
        QPointer<CarouselItem> old;
        for (auto* candidate : m_view->findChildren<CarouselItem*>())
            if (candidate->carousel() == m_view && candidate->objectName() == "item3")
                old = candidate;
        QVERIFY(old);
        QSignalSpy clicks(old, &CommandButton::clicked), forwarded(m_view, &CarouselView::clicked);
        bool       notified = false;
        connect(m_view, &CarouselView::currentIndexChanged, this, [&] {
            notified = true;
            if (old) old->click();
        });
        model.insertRow(0, new QStandardItem("new"));
        QVERIFY(notified);
        QVERIFY(clicks.isEmpty() && forwarded.isEmpty());
        verifyPosition(4);
    }
    void reentrantRequest_data() {
        QTest::addColumn<bool>("destroy");
        QTest::newRow("redirect") << false;
        QTest::newRow("destroy") << true;
    }
    void reentrantAnimatedRequest() {
        create({ { "width", 640 }, { "height", 240 } });
        QVERIFY(m_view);
        verifyPosition(3);
        m_view->setReduceMotion(false);
        const qreal before     = offset();
        bool        redirected = false;
        connect(m_view, &CarouselView::currentIndexChanged, this, [&] {
            if (redirected || m_view->currentIndex() != 5) return;
            redirected = true;
            m_view->setCurrentIndexAnimated(6);
        });
        m_view->setCurrentIndex(5);
        QVERIFY(redirected);
        QCOMPARE(offset(), before);
        QTRY_VERIFY(qAbs(offset() - snapOffset(6)) < 0.01);
        verifyPosition(6);
    }
    void reentrantRequest() {
        QFETCH(bool, destroy);
        create({ { "model", 0 }, { "width", 640 }, { "height", 240 } });
        QVERIFY(m_view);
        bool called = false;
        connect(m_view, &CarouselView::currentIndexChanged, this, [&] {
            if (called) return;
            called = true;
            if (destroy)
                delete m_root.data();
            else
                m_view->setCurrentIndex(6);
        });
        m_view->setModel(10);
        QVERIFY(called);
        if (destroy)
            QVERIFY(! m_view && ! m_root);
        else
            verifyPosition(6);
    }
};

int run_carousel_position(int argc, char** argv) {
    CarouselPositionTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "carousel_position.moc"
