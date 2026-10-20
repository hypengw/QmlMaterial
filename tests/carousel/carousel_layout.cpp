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

class CarouselLayoutTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        m_engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        m_window.setGeometry(0, 0, 800, 600);
        m_window.create();
    }

    void carouselSupportsCompactEmbedding() {
        const auto source = QByteArrayLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            MD.Carousel {
                property bool expanded: true
                minimumViewportHeight: expanded ? 120 : 72
                wheelNavigationEnabled: false
                model: 2
                delegate: Item {
                    required property int index
                    required property var model
                }
            }
        )");

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/carousel-compact.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* carousel = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(carousel);
        carousel->setParentItem(m_window.contentItem());
        settle(carousel);

        QCOMPARE(carousel->property("minimumViewportHeight").toReal(), 120.0);
        QCOMPARE(carousel->property("wheelNavigationEnabled").toBool(), false);
        QCOMPARE(carousel->implicitHeight(), 120.0);

        carousel->setProperty("expanded", false);
        settle(carousel);
        QCOMPARE(carousel->implicitHeight(), 72.0);
        QCOMPARE(carousel->height(), 72.0);
    }

    void carouselItemUsesCornerRadiusOverride() {
        const auto source = QByteArrayLiteral(R"(
            import QtQuick
            import Qcm.Material as MD

            MD.CarouselItem {
                property bool masked: false
                index: 0
                width: 72
                height: 72
                cornerRadius: 12
                maskEnd: masked ? 0.75 : 0
            }
        )");

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/carousel-item-corner.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        QCOMPARE(object->property("effectiveCornerRadius").toReal(), 12.0);

        object->setProperty("masked", true);
        settle(qobject_cast<QQuickItem*>(object.get()));
        QCOMPARE(object->property("effectiveCornerRadius").toReal(), 9.0);
    }

    void carouselAcceptsQmlVarItemModel() {
        QStandardItemModel model(1, 1);
        model.setItemRoleNames({ { Qt::DisplayRole, "title" } });
        model.setData(model.index(0, 0), QStringLiteral("Wallpaper"));

        const auto source = QByteArrayLiteral(R"(
            pragma ComponentBehavior: Bound
            import QtQuick
            import Qcm.Material as MD

            Item {
                id: root
                required property var suppliedModel
                property int delegateCount: 0
                width: 96
                height: 72

                MD.Carousel {
                    anchors.fill: parent
                    model: parent.suppliedModel
                    itemExtent: 72
                    minimumViewportHeight: 72
                    delegate: Item {
                        required property int index
                        required property var model
                        required property string title
                        Component.onCompleted: root.delegateCount += 1
                    }
                }
            }
        )");

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/carousel-var-model.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.createWithInitialProperties(
            { { QStringLiteral("suppliedModel"), QVariant::fromValue(&model) } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(root);
        root->setParentItem(m_window.contentItem());
        settle(root);

        QObject* view = nullptr;
        for (auto* child : root->findChildren<QObject*>()) {
            if (child->property("contentWidth").isValid() &&
                child->property("currentIndex").isValid() &&
                child->property("itemExtent").isValid()) {
                view = child;
                break;
            }
        }
        QVERIFY(view);
        QCOMPARE(view->property("count").toInt(), 1);
        QCOMPARE(root->property("delegateCount").toInt(), 1);
    }

    void carouselLayoutDoesNotRewriteDelegateModel() {
        const auto source = QByteArrayLiteral(R"(
            pragma ComponentBehavior: Bound
            import QtQuick
            import Qcm.Material as MD

            Item {
                id: root
                width: 480
                height: 160
                property int modelChangeCount: 0

                MD.Carousel {
                    id: carousel
                    objectName: "carousel"
                    anchors.fill: parent
                    layout: MD.Enum.CarouselHero
                    model: [{}, {}, {}]
                    delegate: Item {
                        required property int index
                        required property var model
                        property real itemAspectRatio: 1.5
                        onModelChanged: root.modelChangeCount += 1
                    }
                }
            }
        )");

        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/carousel-model-lifecycle.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(root);
        root->setParentItem(m_window.contentItem());
        settle(root);

        auto* carousel = root->findChild<QQuickItem*>(QStringLiteral("carousel"));
        QVERIFY(carousel);
        const int initialModelChangeCount = root->property("modelChangeCount").toInt();
        QVERIFY(initialModelChangeCount > 0);

        carousel->setProperty("itemExtent", carousel->property("itemExtent").toReal() + 1);
        settle(root);
        QCOMPARE(root->property("modelChangeCount").toInt(), initialModelChangeCount);
    }

private:
    QQmlEngine   m_engine;
    QQuickWindow m_window;
};

int run_carousel_layout(int argc, char** argv) {
    CarouselLayoutTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "carousel_layout.moc"
