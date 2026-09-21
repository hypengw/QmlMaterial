#include <QCoreApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QtTest>
#include <algorithm>
#include <memory>

#include "qml_material/layout/layout.hpp"

namespace
{

void settle(QQuickItem* item, int rounds = 5) {
    for (int round = 0; round < rounds; ++round) {
        item->ensurePolished();
        QCoreApplication::processEvents();
    }
}

QQuickItem* namedItem(QObject* root, const char* name) {
    const auto target_name = QString::fromLatin1(name);
    if (auto* item = qobject_cast<QQuickItem*>(root)) {
        if (item->objectName() == target_name) {
            return item;
        }
        for (auto* child : item->childItems()) {
            if (auto* match = namedItem(child, name)) {
                return match;
            }
        }
    }
    return root->findChild<QQuickItem*>(target_name);
}

QStringList* g_captured_warnings = nullptr;

void captureWarnings(QtMsgType type, const QMessageLogContext&, const QString& message) {
    if (type == QtWarningMsg && g_captured_warnings) {
        g_captured_warnings->push_back(message);
    }
}

class WarningCapture {
public:
    WarningCapture() {
        g_captured_warnings = &warnings;
        previous_handler    = qInstallMessageHandler(captureWarnings);
    }

    ~WarningCapture() {
        qInstallMessageHandler(previous_handler);
        g_captured_warnings = nullptr;
    }

    int countContaining(QLatin1StringView text) const {
        return int(
            std::count_if(warnings.cbegin(), warnings.cend(), [text](const QString& warning) {
                return warning.contains(text);
            }));
    }

    QStringList warnings;

private:
    QtMessageHandler previous_handler = nullptr;
};

} // namespace

class LiteLayoutTest : public QObject {
    Q_OBJECT

private:
    std::unique_ptr<QQuickItem> create(const QByteArray& source) {
        QQmlComponent component(&m_engine);
        component.setData(source, QUrl(QStringLiteral("qrc:/tests/lite-layout.qml")));
        if (component.isError()) {
            QStringList errors;
            for (const auto& error : component.errors()) {
                errors.push_back(error.toString());
            }
            m_error = errors.join(QLatin1Char('\n'));
            return {};
        }
        QObject* object = component.create();
        if (! object) {
            m_error = QStringLiteral("component.create() returned null");
            return {};
        }
        auto* item = qobject_cast<QQuickItem*>(object);
        if (! item) {
            m_error = QStringLiteral("root object is not a QQuickItem");
            delete object;
            return {};
        }
        m_error.clear();
        return std::unique_ptr<QQuickItem>(item);
    }

private Q_SLOTS:
    void strictImplicitAndActualContent() {
        auto root = create(R"(
            import QtQuick
            import Qcm.Material.Layouts as Lite
            Lite.Row {
                width: 300
                height: 60
                leftPadding: 5
                rightPadding: 7
                topPadding: 2
                bottomPadding: 3
                spacing: 10
                Item {
                    objectName: "first"
                    implicitWidth: 80
                    implicitHeight: 20
                    width: 240
                    height: 30
                }
                Item {
                    objectName: "second"
                    implicitWidth: 0
                    implicitHeight: 10
                    width: 20
                    height: 10
                }
            }
        )");
        QVERIFY2(root, qPrintable(m_error));
        settle(root.get());

        QCOMPARE(root->implicitWidth(), 102.0);
        QCOMPARE(root->implicitHeight(), 25.0);
        QCOMPARE(root->property("contentWidth").toReal(), 282.0);
        QCOMPARE(root->property("contentHeight").toReal(), 35.0);
        QCOMPARE(namedItem(root.get(), "first")->x(), 5.0);
        QCOMPARE(namedItem(root.get(), "second")->x(), 255.0);

        auto*      first = namedItem(root.get(), "first");
        QSignalSpy content_width_changed(root.get(), SIGNAL(contentWidthChanged()));
        first->setWidth(250);
        settle(root.get());
        QCOMPARE(root->property("contentWidth").toReal(), 292.0);
        QCOMPARE(content_width_changed.size(), 1);
        first->setWidth(250);
        settle(root.get());
        QCOMPARE(content_width_changed.size(), 1);

        first->setImplicitWidth(90);
        settle(root.get());
        QCOMPARE(root->implicitWidth(), 112.0);
        QVERIFY(! root->setProperty("implicitWidth", 999));
        QCOMPARE(root->implicitWidth(), 112.0);
    }

    void rowFillAndCrossAxisFill() {
        auto root = create(R"(
            import QtQuick
            import Qcm.Material.Layouts as Lite
            Lite.Row {
                width: 300
                height: 60
                leftPadding: 10
                rightPadding: 10
                topPadding: 5
                bottomPadding: 5
                spacing: 5
                Item {
                    objectName: "first"
                    width: 50
                    height: 10
                    Lite.Layout.fillHeight: true
                }
                Item {
                    objectName: "filler"
                    implicitWidth: 20
                    width: 20
                    height: 15
                    Lite.Layout.fillWidth: true
                }
                Item {
                    objectName: "last"
                    width: 30
                    height: 20
                    Lite.Layout.fillHeight: true
                }
            }
        )");
        QVERIFY2(root, qPrintable(m_error));
        settle(root.get());

        auto* first  = namedItem(root.get(), "first");
        auto* filler = namedItem(root.get(), "filler");
        auto* last   = namedItem(root.get(), "last");
        QCOMPARE(first->height(), 50.0);
        QCOMPARE(filler->width(), 190.0);
        QCOMPARE(last->height(), 50.0);
        QCOMPARE(first->x(), 10.0);
        QCOMPARE(filler->x(), 65.0);
        QCOMPARE(last->x(), 260.0);
        QCOMPARE(root->property("contentWidth").toReal(), 300.0);
        QCOMPARE(root->property("contentHeight").toReal(), 60.0);

        auto* attached = static_cast<qml_material::LayoutAttached*>(
            qmlAttachedPropertiesObject<qml_material::Layout>(filler, false));
        QVERIFY(attached);
        attached->setFillWidth(false);
        root->setWidth(350);
        settle(root.get());
        QCOMPARE(filler->width(), 190.0);
        attached->setFillWidth(true);
        settle(root.get());
        QCOMPARE(filler->width(), 240.0);
    }

    void emptyZeroSizedAndLayerHelper() {
        auto row = create(R"(
            import QtQuick
            import Qcm.Material.Layouts as Lite
            Lite.Row {
                leftPadding: 3
                rightPadding: 4
                topPadding: 2
                bottomPadding: 5
                spacing: 5
                Item {
                    objectName: "zero"
                    implicitWidth: 0
                    implicitHeight: 0
                    width: 0
                    height: 0
                }
                Item {
                    objectName: "normal"
                    layer.enabled: true
                    implicitWidth: 10
                    implicitHeight: 8
                    width: 10
                    height: 8
                }
                Item {
                    visible: false
                    implicitWidth: 100
                    implicitHeight: 100
                    width: 100
                    height: 100
                }
            }
        )");
        QVERIFY2(row, qPrintable(m_error));
        settle(row.get());
        QVERIFY(row->childItems().size() > 3);
        QCOMPARE(row->implicitWidth(), 22.0);
        QCOMPARE(row->implicitHeight(), 15.0);
        QCOMPARE(row->property("contentWidth").toReal(), 22.0);
        QCOMPARE(row->property("contentHeight").toReal(), 15.0);
        QCOMPARE(namedItem(row.get(), "normal")->x(), 8.0);

        auto box = create(R"(
            import QtQuick
            import Qcm.Material.Layouts as Lite
            Lite.Box {
                leftPadding: 2
                rightPadding: 3
                topPadding: 4
                bottomPadding: 5
            }
        )");
        QVERIFY2(box, qPrintable(m_error));
        settle(box.get());
        QCOMPARE(box->implicitWidth(), 5.0);
        QCOMPARE(box->implicitHeight(), 9.0);
        QCOMPARE(box->property("contentWidth").toReal(), 5.0);
        QCOMPARE(box->property("contentHeight").toReal(), 9.0);
    }

    void multipleMainFillAndOverflow() {
        WarningCapture capture;
        auto           root = create(R"(
            import QtQuick
            import Qcm.Material.Layouts as Lite
            Lite.Row {
                width: 200
                height: 20
                spacing: 5
                Item {
                    objectName: "winner"
                    width: 10
                    height: 10
                    Lite.Layout.fillWidth: true
                }
                Item {
                    objectName: "other"
                    width: 20
                    height: 10
                    Lite.Layout.fillWidth: true
                }
                Item { width: 50; height: 10 }
            }
        )");
        QVERIFY2(root, qPrintable(m_error));
        auto* winner = namedItem(root.get(), "winner");
        auto* other  = namedItem(root.get(), "other");
        settle(root.get());
        QCOMPARE(winner->width(), 120.0);
        QCOMPARE(other->width(), 20.0);
        QCOMPARE(
            capture.countContaining(QLatin1StringView("LiteRow supports only one fillWidth child")),
            1);

        root->setWidth(210);
        settle(root.get());
        QCOMPARE(winner->width(), 130.0);
        QCOMPARE(
            capture.countContaining(QLatin1StringView("LiteRow supports only one fillWidth child")),
            1);

        auto* other_attached = static_cast<qml_material::LayoutAttached*>(
            qmlAttachedPropertiesObject<qml_material::Layout>(other, false));
        QVERIFY(other_attached);
        other_attached->setFillWidth(false);
        settle(root.get());
        other_attached->setFillWidth(true);
        settle(root.get());
        QCOMPARE(
            capture.countContaining(QLatin1StringView("LiteRow supports only one fillWidth child")),
            2);

        other_attached->setFillWidth(false);
        winner->setWidth(10);
        other->setWidth(60);
        root->setWidth(50);
        settle(root.get());
        QCOMPARE(winner->width(), 0.0);
        QVERIFY(root->property("contentWidth").toReal() > root->width());
    }

    void columnAndBoxFill() {
        auto column = create(R"(
            import QtQuick
            import Qcm.Material.Layouts as Lite
            Lite.Column {
                width: 100
                height: 200
                leftPadding: 10
                rightPadding: 10
                topPadding: 10
                bottomPadding: 10
                spacing: 5
                Item {
                    objectName: "first"
                    width: 20
                    height: 30
                    Lite.Layout.fillWidth: true
                }
                Item {
                    objectName: "filler"
                    width: 40
                    height: 10
                    Lite.Layout.fillHeight: true
                }
                Item { objectName: "last"; width: 15; height: 20 }
            }
        )");
        QVERIFY2(column, qPrintable(m_error));
        settle(column.get());
        QCOMPARE(namedItem(column.get(), "first")->width(), 80.0);
        QCOMPARE(namedItem(column.get(), "filler")->height(), 120.0);
        QCOMPARE(namedItem(column.get(), "first")->y(), 10.0);
        QCOMPARE(namedItem(column.get(), "filler")->y(), 45.0);
        QCOMPARE(namedItem(column.get(), "last")->y(), 170.0);
        QCOMPARE(column->property("contentWidth").toReal(), 100.0);
        QCOMPARE(column->property("contentHeight").toReal(), 200.0);

        auto box = create(R"(
            import QtQuick
            import Qcm.Material.Layouts as Lite
            Lite.Box {
                width: 100
                height: 80
                leftPadding: 10
                rightPadding: 10
                topPadding: 10
                bottomPadding: 10
                alignment: Qt.AlignRight | Qt.AlignBottom
                Item {
                    objectName: "filled"
                    width: 1
                    height: 1
                    Lite.Layout.fillWidth: true
                    Lite.Layout.fillHeight: true
                }
                Item { objectName: "corner"; width: 20; height: 10 }
            }
        )");
        QVERIFY2(box, qPrintable(m_error));
        settle(box.get());
        auto* filled = namedItem(box.get(), "filled");
        auto* corner = namedItem(box.get(), "corner");
        QCOMPARE(filled->width(), 80.0);
        QCOMPARE(filled->height(), 60.0);
        QCOMPARE(filled->x(), 10.0);
        QCOMPARE(filled->y(), 10.0);
        QCOMPARE(corner->x(), 70.0);
        QCOMPARE(corner->y(), 60.0);
    }

    void alignmentAndNestedPropagation() {
        auto row = create(R"(
            import QtQuick
            import Qcm.Material.Layouts as Lite
            Lite.Row {
                width: 100
                height: 40
                alignment: Qt.AlignHCenter | Qt.AlignVCenter
                Item { objectName: "first"; width: 10; height: 10 }
                Item { objectName: "second"; width: 20; height: 20 }
            }
        )");
        QVERIFY2(row, qPrintable(m_error));
        settle(row.get());
        QCOMPARE(namedItem(row.get(), "first")->x(), 35.0);
        QCOMPARE(namedItem(row.get(), "first")->y(), 15.0);
        QCOMPARE(namedItem(row.get(), "second")->x(), 45.0);
        QCOMPARE(namedItem(row.get(), "second")->y(), 10.0);

        auto column = create(R"(
            import QtQuick
            import Qcm.Material.Layouts as Lite
            Lite.Column {
                width: 100
                height: 100
                spacing: 10
                alignment: Qt.AlignHCenter | Qt.AlignVCenter
                Item { objectName: "first"; width: 10; height: 10 }
                Item { objectName: "second"; width: 20; height: 20 }
            }
        )");
        QVERIFY2(column, qPrintable(m_error));
        settle(column.get());
        QCOMPARE(namedItem(column.get(), "first")->x(), 45.0);
        QCOMPARE(namedItem(column.get(), "first")->y(), 30.0);
        QCOMPARE(namedItem(column.get(), "second")->x(), 40.0);
        QCOMPARE(namedItem(column.get(), "second")->y(), 50.0);

        auto nested = create(R"(
            import QtQuick
            import Qcm.Material.Layouts as Lite
            Lite.Row {
                objectName: "outer"
                spacing: 5
                Lite.Column {
                    objectName: "inner"
                    spacing: 2
                    Item { implicitWidth: 30; implicitHeight: 10 }
                    Item { implicitWidth: 20; implicitHeight: 15 }
                }
                Item { implicitWidth: 10; implicitHeight: 5 }
            }
        )");
        QVERIFY2(nested, qPrintable(m_error));
        auto* inner = namedItem(nested.get(), "inner");
        settle(inner);
        settle(nested.get());
        QCOMPARE(inner->implicitWidth(), 30.0);
        QCOMPARE(inner->implicitHeight(), 27.0);
        QCOMPARE(nested->implicitWidth(), 45.0);
        QCOMPARE(nested->implicitHeight(), 27.0);
        nested->setWidth(300);
        settle(nested.get());
        QCOMPARE(nested->implicitWidth(), 45.0);
        QCOMPARE(nested->implicitHeight(), 27.0);
    }

    void explicitVisibilityAndRestack() {
        auto root = create(R"(
            import QtQuick
            import Qcm.Material.Layouts as Lite
            Item {
                property alias firstVisible: first.visible
                Lite.Row {
                    objectName: "row"
                    spacing: 4
                    Item {
                        id: first
                        objectName: "first"
                        implicitWidth: 10
                        implicitHeight: 10
                        width: 10
                        height: 10
                    }
                    Item {
                        objectName: "second"
                        implicitWidth: 20
                        implicitHeight: 10
                        width: 20
                        height: 10
                    }
                }
            }
        )");
        QVERIFY2(root, qPrintable(m_error));
        QQuickWindow window;
        root->setParentItem(window.contentItem());
        auto*      row    = namedItem(root.get(), "row");
        auto*      first  = namedItem(root.get(), "first");
        auto*      second = namedItem(root.get(), "second");
        QSignalSpy row_visibility_changed(row, &QQuickItem::visibleChanged);
        settle(row);
        QVERIFY(root->isVisible());
        QVERIFY(row->isVisible());
        QCOMPARE(row->implicitWidth(), 34.0);

        root->setVisible(false);
        settle(row);
        QVERIFY(! row->isVisible());
        QCOMPARE(row->implicitWidth(), 34.0);
        first->setVisible(false);
        root->setVisible(true);
        settle(row);
        QVERIFY(row->isVisible());
        QVERIFY(! first->isVisible());
        QCOMPARE(row_visibility_changed.size(), 2);
        QCOMPARE(row->implicitWidth(), 20.0);
        QCOMPARE(second->x(), 0.0);

        root->setProperty("firstVisible", true);
        settle(row);
        second->stackBefore(first);
        settle(row);
        QCOMPARE(second->x(), 0.0);
        QCOMPARE(first->x(), 24.0);
    }

    void visibilitySourceControlsParticipation() {
        auto root = create(R"(
            import QtQuick
            import Qcm.Material.Layouts as Lite
            Item {
                property alias sourceVisible: source.visible
                Lite.Row {
                    objectName: "row"
                    spacing: 5
                    Item {
                        objectName: "first"
                        implicitWidth: 10
                        implicitHeight: 10
                        width: 10
                        height: 10
                    }
                    Item {
                        objectName: "wrapper"
                        implicitWidth: 20
                        implicitHeight: 10
                        width: 20
                        height: 10
                        Lite.Layout.visibilitySource: source
                        Item {
                            id: source
                            objectName: "source"
                        }
                    }
                    Item {
                        objectName: "last"
                        implicitWidth: 30
                        implicitHeight: 10
                        width: 30
                        height: 10
                    }
                }
            }
        )");
        QVERIFY2(root, qPrintable(m_error));
        auto* row     = namedItem(root.get(), "row");
        auto* wrapper = namedItem(root.get(), "wrapper");
        auto* last    = namedItem(root.get(), "last");
        settle(row);

        QCOMPARE(row->implicitWidth(), 70.0);
        QCOMPARE(row->property("contentWidth").toReal(), 70.0);
        QCOMPARE(wrapper->x(), 15.0);
        QCOMPARE(last->x(), 40.0);

        QVERIFY(root->setProperty("sourceVisible", false));
        settle(row);
        QCOMPARE(row->implicitWidth(), 45.0);
        QCOMPARE(row->property("contentWidth").toReal(), 45.0);
        QCOMPARE(last->x(), 15.0);

        QVERIFY(root->setProperty("sourceVisible", true));
        settle(row);
        QCOMPARE(row->implicitWidth(), 70.0);
        QCOMPARE(last->x(), 40.0);
    }

    void visibilitySourceRecoversAfterAncestorVisibilityChange() {
        auto root = create(R"(
            import QtQuick
            import Qcm.Material.Layouts as Lite
            Item {
                property alias sourceVisible: source.visible
                Lite.Row {
                    objectName: "row"
                    spacing: 5
                    Item {
                        objectName: "wrapper"
                        implicitWidth: 20
                        implicitHeight: 10
                        width: 20
                        height: 10
                        Lite.Layout.visibilitySource: source
                        Item { id: source }
                    }
                    Item {
                        implicitWidth: 30
                        implicitHeight: 10
                        width: 30
                        height: 10
                    }
                }
            }
        )");
        QVERIFY2(root, qPrintable(m_error));
        QQuickWindow window;
        root->setParentItem(window.contentItem());
        auto* row = namedItem(root.get(), "row");
        settle(row);
        QCOMPARE(row->implicitWidth(), 55.0);

        root->setVisible(false);
        settle(row);
        QVERIFY(root->setProperty("sourceVisible", false));
        root->setVisible(true);
        settle(row);
        QCOMPARE(row->implicitWidth(), 30.0);

        root->setVisible(false);
        settle(row);
        QVERIFY(root->setProperty("sourceVisible", true));
        root->setVisible(true);
        settle(row);
        QCOMPARE(row->implicitWidth(), 55.0);
    }

    void visibilitySourceNullReplacementAndReset() {
        auto root = create(R"(
            import QtQuick
            import Qcm.Material.Layouts as Lite
            Item {
                Lite.Row {
                    objectName: "row"
                    spacing: 4
                    Item {
                        implicitWidth: 10
                        implicitHeight: 10
                        width: 10
                        height: 10
                    }
                    Item {
                        id: wrapper
                        objectName: "wrapper"
                        property Item selectedSource: null
                        implicitWidth: 20
                        implicitHeight: 10
                        width: 20
                        height: 10
                        Lite.Layout.visibilitySource: selectedSource
                        Item { objectName: "firstSource" }
                        Item { objectName: "secondSource" }
                    }
                    Item {
                        implicitWidth: 30
                        implicitHeight: 10
                        width: 30
                        height: 10
                    }
                }
            }
        )");
        QVERIFY2(root, qPrintable(m_error));
        auto* row           = namedItem(root.get(), "row");
        auto* wrapper       = namedItem(root.get(), "wrapper");
        auto* first_source  = namedItem(root.get(), "firstSource");
        auto* second_source = namedItem(root.get(), "secondSource");
        settle(row);
        QCOMPARE(row->implicitWidth(), 44.0);

        QVERIFY(wrapper->setProperty("selectedSource", QVariant::fromValue(first_source)));
        settle(row);
        QCOMPARE(row->implicitWidth(), 68.0);

        first_source->setVisible(false);
        settle(row);
        QCOMPARE(row->implicitWidth(), 44.0);

        QVERIFY(wrapper->setProperty("selectedSource", QVariant::fromValue(second_source)));
        settle(row);
        QCOMPARE(row->implicitWidth(), 68.0);
        first_source->setVisible(true);
        settle(row);
        QCOMPARE(row->implicitWidth(), 68.0);

        second_source->setVisible(false);
        settle(row);
        QCOMPARE(row->implicitWidth(), 44.0);
        delete second_source;
        settle(row);
        QCOMPARE(row->implicitWidth(), 44.0);

        auto* attached = static_cast<qml_material::LayoutAttached*>(
            qmlAttachedPropertiesObject<qml_material::Layout>(wrapper, false));
        QVERIFY(attached);
        QVERIFY(attached->isVisibilitySourceSet());
        attached->resetVisibilitySource();
        settle(row);
        QVERIFY(! attached->isVisibilitySourceSet());
        QCOMPARE(row->implicitWidth(), 68.0);
    }

    void visibilitySourceRejectsNonChild() {
        WarningCapture capture;
        auto           root = create(R"(
            import QtQuick
            import Qcm.Material.Layouts as Lite
            Item {
                Item { id: outside }
                Lite.Row {
                    objectName: "row"
                    Item {
                        implicitWidth: 20
                        implicitHeight: 10
                        Lite.Layout.visibilitySource: outside
                    }
                }
            }
        )");
        QVERIFY2(root, qPrintable(m_error));
        auto* row = namedItem(root.get(), "row");
        settle(row);

        QCOMPARE(row->implicitWidth(), 0.0);
        QCOMPARE(capture.countContaining(
                     QLatin1StringView("Layout.visibilitySource must be a direct visual child")),
                 1);
    }

    void repeaterParticipationAndMove() {
        auto root = create(R"(
            import QtQuick
            import Qcm.Material.Layouts as Lite
            Lite.Row {
                spacing: 2
                function moveFirstToEnd() { entries.move(0, 2, 1) }
                function appendEntry() { entries.append({label: "d"}) }
                function removeLast() { entries.remove(entries.count - 1) }
                Repeater {
                    model: ListModel {
                        id: entries
                        ListElement { label: "a" }
                        ListElement { label: "b" }
                        ListElement { label: "c" }
                    }
                    delegate: Item {
                        required property string label
                        objectName: label
                        implicitWidth: 10
                        implicitHeight: 10
                        width: 10
                        height: 10
                    }
                }
            }
        )");
        QVERIFY2(root, qPrintable(m_error));
        settle(root.get());
        QCOMPARE(root->implicitWidth(), 34.0);
        auto* first  = namedItem(root.get(), "a");
        auto* second = namedItem(root.get(), "b");
        auto* third  = namedItem(root.get(), "c");
        QVERIFY(first);
        QVERIFY(second);
        QVERIFY(third);
        QCOMPARE(first->x(), 0.0);
        QCOMPARE(second->x(), 12.0);
        QCOMPARE(third->x(), 24.0);

        QVERIFY(QMetaObject::invokeMethod(root.get(), "moveFirstToEnd"));
        settle(root.get());
        QCOMPARE(root->implicitWidth(), 34.0);
        QCOMPARE(second->x(), 0.0);
        QCOMPARE(third->x(), 12.0);
        QCOMPARE(first->x(), 24.0);

        QVERIFY(QMetaObject::invokeMethod(root.get(), "appendEntry"));
        settle(root.get());
        QCOMPARE(root->implicitWidth(), 46.0);
        QCOMPARE(namedItem(root.get(), "d")->x(), 36.0);
        QVERIFY(QMetaObject::invokeMethod(root.get(), "removeLast"));
        settle(root.get());
        QCOMPARE(root->implicitWidth(), 34.0);
    }

    void mirroringAndLayoutDirection() {
        auto root = create(R"(
            import QtQuick
            import Qcm.Material.Layouts as Lite
            Item {
                LayoutMirroring.enabled: true
                LayoutMirroring.childrenInherit: true
                Lite.Row {
                    objectName: "row"
                    width: 100
                    height: 20
                    Item { objectName: "first"; width: 10; height: 10 }
                    Item { objectName: "second"; width: 20; height: 10 }
                }
            }
        )");
        QVERIFY2(root, qPrintable(m_error));
        auto* row    = namedItem(root.get(), "row");
        auto* first  = namedItem(root.get(), "first");
        auto* second = namedItem(root.get(), "second");
        settle(row);
        QCOMPARE(row->property("effectiveLayoutDirection").toInt(), int(Qt::RightToLeft));
        QCOMPARE(first->x(), 90.0);
        QCOMPARE(second->x(), 70.0);

        row->setProperty("alignment", int(Qt::AlignLeft));
        settle(row);
        QCOMPARE(first->x(), 20.0);
        QCOMPARE(second->x(), 0.0);

        row->setProperty("layoutDirection", int(Qt::RightToLeft));
        settle(row);
        QCOMPARE(row->property("effectiveLayoutDirection").toInt(), int(Qt::LeftToRight));
        QCOMPARE(first->x(), 0.0);
        QCOMPARE(second->x(), 10.0);
    }

    void textHeightForWidthConverges() {
        auto root = create(R"(
            import QtQuick
            import Qcm.Material.Layouts as Lite
            Lite.Row {
                width: 80
                Item { width: 20; height: 10 }
                Text {
                    objectName: "text"
                    text: "one two three four five six"
                    wrapMode: Text.WordWrap
                    Lite.Layout.fillWidth: true
                }
            }
        )");
        QVERIFY2(root, qPrintable(m_error));
        settle(root.get(), 8);
        auto* text = namedItem(root.get(), "text");
        QCOMPARE(text->width(), 60.0);
        QVERIFY(text->implicitHeight() > 20.0);
        QCOMPARE(root->implicitHeight(), text->implicitHeight());
    }

private:
    QQmlEngine m_engine;
    QString    m_error;
};

QTEST_MAIN(LiteLayoutTest)

#include "lite_layout.moc"
