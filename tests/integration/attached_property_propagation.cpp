#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QRegularExpression>
#include <QtQml/qqml.h>
#include <QtTest>
#include <memory>

#include "qml_material/style/theme.hpp"

namespace
{

void addImportPaths(QQmlEngine& engine) {
    engine.addImportPath(QCoreApplication::applicationDirPath() +
                         QStringLiteral("/../qml_modules"));
    const auto envPath = qgetenv("QML_IMPORT_PATH");
    if (envPath.isEmpty()) return;

#if defined(Q_OS_WIN)
    const auto parts = envPath.split(';');
#else
    const auto parts = envPath.split(':');
#endif
    for (const auto& part : parts) {
        if (! part.isEmpty()) engine.addImportPath(QString::fromLocal8Bit(part));
    }
}

std::unique_ptr<QObject> create(QQmlEngine& engine, const QByteArray& source, const QString& name) {
    QQmlComponent component(&engine);
    component.setData(source, QUrl(QStringLiteral("qrc:/tests/") + name));
    if (component.isError()) qWarning().noquote() << component.errorString();
    return std::unique_ptr<QObject>(component.create());
}

qml_material::Theme* theme(QObject* target, bool create = true) {
    return qobject_cast<qml_material::Theme*>(
        qmlAttachedPropertiesObject<qml_material::Theme>(target, create));
}

} // namespace

class CountingAttached final : public qml_material::AttachedPropertyNode {
    Q_OBJECT

public:
    explicit CountingAttached(QObject* target)
        : AttachedPropertyNode(target, &CountingAttached::staticMetaObject) {
        initializeAttachedProperty();
    }

    int  updateCount() const { return m_update_count; }
    void resetUpdateCount() { m_update_count = 0; }

protected:
    void updateInheritedValues() override { ++m_update_count; }

private:
    int m_update_count { 0 };
};

class AttachedPropertyPropagationTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void visualInheritanceAndReset() {
        QQmlEngine engine;
        addImportPaths(engine);
        auto root = create(engine,
                           R"(
            import QtQuick
            import Qcm.Material as MD

            Item {
                objectName: "root"
                MD.MProp.elevation: 2
                Item {
                    objectName: "child"
                    property int observedElevation: MD.MProp.elevation
                }
            }
        )",
                           QStringLiteral("visual-inheritance.qml"));
        QVERIFY(root);

        auto* child = root->findChild<QQuickItem*>(QStringLiteral("child"));
        QVERIFY(child);
        auto* rootTheme  = theme(root.get(), false);
        auto* childTheme = theme(child, false);
        QVERIFY(rootTheme);
        QVERIFY(childTheme);
        QCOMPARE(child->property("observedElevation").toInt(), 2);

        rootTheme->set_elevation(4);
        QCOMPARE(child->property("observedElevation").toInt(), 4);

        childTheme->set_elevation(7);
        rootTheme->set_elevation(5);
        QCOMPARE(child->property("observedElevation").toInt(), 7);

        childTheme->reset_elevation();
        QCOMPARE(child->property("observedElevation").toInt(), 5);
    }

    void lateAncestorCreation() {
        QQmlEngine engine;
        addImportPaths(engine);
        auto root = create(engine,
                           R"(
            import QtQuick
            import Qcm.Material as MD

            Item {
                objectName: "root"
                Item {
                    objectName: "child"
                    property int observedElevation: MD.MProp.elevation
                }
            }
        )",
                           QStringLiteral("late-ancestor.qml"));
        QVERIFY(root);

        auto* child = root->findChild<QQuickItem*>(QStringLiteral("child"));
        QVERIFY(child);
        QVERIFY(theme(child, false));
        QVERIFY(! theme(root.get(), false));
        QCOMPARE(child->property("observedElevation").toInt(), 0);

        auto* rootTheme = theme(root.get());
        QVERIFY(rootTheme);
        rootTheme->set_elevation(9);
        QCOMPARE(child->property("observedElevation").toInt(), 9);
    }

    void reparenting() {
        QQmlEngine engine;
        addImportPaths(engine);
        auto root = create(engine,
                           R"(
            import QtQuick
            import Qcm.Material as MD

            Item {
                Item {
                    id: first
                    objectName: "first"
                    MD.MProp.elevation: 1
                    Item {
                        id: child
                        objectName: "child"
                        property int observedElevation: MD.MProp.elevation
                    }
                }
                Item {
                    id: second
                    objectName: "second"
                    MD.MProp.elevation: 2
                }
            }
        )",
                           QStringLiteral("reparenting.qml"));
        QVERIFY(root);

        auto* child  = root->findChild<QQuickItem*>(QStringLiteral("child"));
        auto* second = root->findChild<QQuickItem*>(QStringLiteral("second"));
        QVERIFY(child);
        QVERIFY(second);
        QCOMPARE(child->property("observedElevation").toInt(), 1);

        child->setParentItem(second);
        QCOMPARE(child->property("observedElevation").toInt(), 2);

        child->setParentItem(nullptr);
        QCOMPARE(child->property("observedElevation").toInt(), 0);
    }

    void reparentingOnlyRefreshesAffectedNode() {
        QQmlEngine engine;
        auto       root = create(engine,
                                 R"(
            import QtQuick

            Item {
                Item {
                    objectName: "first"
                    Item { objectName: "moving" }
                }
                Item {
                    objectName: "second"
                    Item { objectName: "unrelated" }
                }
            }
        )",
                                 QStringLiteral("incremental-reparenting.qml"));
        QVERIFY(root);

        auto* first         = root->findChild<QQuickItem*>(QStringLiteral("first"));
        auto* moving        = root->findChild<QQuickItem*>(QStringLiteral("moving"));
        auto* second        = root->findChild<QQuickItem*>(QStringLiteral("second"));
        auto* unrelated     = root->findChild<QQuickItem*>(QStringLiteral("unrelated"));
        auto* firstNode     = new CountingAttached(first);
        auto* movingNode    = new CountingAttached(moving);
        auto* secondNode    = new CountingAttached(second);
        auto* unrelatedNode = new CountingAttached(unrelated);
        QCOMPARE(movingNode->attachedParent(), firstNode);
        QCOMPARE(unrelatedNode->attachedParent(), secondNode);

        firstNode->resetUpdateCount();
        movingNode->resetUpdateCount();
        secondNode->resetUpdateCount();
        unrelatedNode->resetUpdateCount();

        moving->setParentItem(second);
        QCOMPARE(movingNode->attachedParent(), secondNode);
        QCOMPARE(movingNode->updateCount(), 1);
        QCOMPARE(firstNode->updateCount(), 0);
        QCOMPARE(secondNode->updateCount(), 0);
        QCOMPARE(unrelatedNode->updateCount(), 0);
    }

    void windowFallback() {
        QQmlEngine engine;
        addImportPaths(engine);
        auto window = create(engine,
                             R"(
            import QtQuick
            import QtQuick.Window
            import Qcm.Material as MD

            Window {
                visible: false
                MD.MProp.elevation: 6
                Item {
                    objectName: "child"
                    property int observedElevation: MD.MProp.elevation
                }
            }
        )",
                             QStringLiteral("window-fallback.qml"));
        QVERIFY(window);

        auto* child = window->findChild<QQuickItem*>(QStringLiteral("child"));
        QVERIFY(child);
        QCOMPARE(child->property("observedElevation").toInt(), 6);
    }

    void logicalParentAndDestruction() {
        QQmlEngine engine;
        addImportPaths(engine);
        auto root = create(engine,
                           R"(
            import QtQuick
            import Qcm.Material as MD

            Item {
                Item {
                    id: visualParent
                    objectName: "visualParent"
                    MD.MProp.elevation: 1
                    Item {
                        objectName: "child"
                        MD.MProp.inheritFrom: logicalParent
                        property int observedElevation: MD.MProp.elevation
                    }
                }
                Item {
                    id: logicalParent
                    objectName: "logicalParent"
                    MD.MProp.elevation: 2
                }
            }
        )",
                           QStringLiteral("logical-parent.qml"));
        QVERIFY(root);

        auto* child         = root->findChild<QQuickItem*>(QStringLiteral("child"));
        auto* logicalParent = root->findChild<QQuickItem*>(QStringLiteral("logicalParent"));
        QVERIFY(child);
        QVERIFY(logicalParent);
        auto* childTheme = theme(child, false);
        QVERIFY(childTheme);
        QCOMPARE(child->property("observedElevation").toInt(), 2);

        childTheme->resetInheritFrom();
        QCOMPARE(child->property("observedElevation").toInt(), 1);

        childTheme->setInheritFrom(logicalParent);
        QCOMPARE(child->property("observedElevation").toInt(), 2);
        delete logicalParent;
        QCOMPARE(child->property("observedElevation").toInt(), 1);
        QCOMPARE(childTheme->inheritFrom(), nullptr);
    }

    void popupContentBridge() {
        QQmlEngine engine;
        addImportPaths(engine);
        auto window = create(engine,
                             R"(
            import QtQuick
            import QtQuick.Window
            import Qcm.Material as MD

            Window {
                visible: false
                MD.MProp.elevation: 2
                Item {
                    id: firstHost
                    objectName: "firstHost"
                    anchors.fill: parent
                    MD.MProp.elevation: 3
                }
                Item {
                    id: secondHost
                    objectName: "secondHost"
                    anchors.fill: parent
                    MD.MProp.elevation: 4
                }
                MD.Popup {
                    id: popup
                    objectName: "popup"
                    parent: firstHost
                    contentItem: Item {
                        objectName: "popupContent"
                        property int observedElevation: MD.MProp.elevation
                    }
                }
            }
        )",
                             QStringLiteral("popup-content.qml"));
        QVERIFY(window);
        QCoreApplication::processEvents();

        auto* popup      = window->findChild<QObject*>(QStringLiteral("popup"));
        auto* content    = window->findChild<QQuickItem*>(QStringLiteral("popupContent"));
        auto* firstHost  = window->findChild<QQuickItem*>(QStringLiteral("firstHost"));
        auto* secondHost = window->findChild<QQuickItem*>(QStringLiteral("secondHost"));
        QVERIFY(popup);
        QVERIFY(content);
        QVERIFY(firstHost);
        QVERIFY(secondHost);
        QCOMPARE(popup->property("contentItem").value<QQuickItem*>(), content);
        auto* popupTheme   = theme(popup, false);
        auto* contentTheme = theme(content, false);
        QVERIFY(popupTheme);
        QVERIFY(contentTheme);
        QCOMPARE(popupTheme->attachedParent(), theme(firstHost, false));
        QCOMPARE(contentTheme->attachedParent(), popupTheme);
        QCOMPARE(content->property("observedElevation").toInt(), 3);

        QVERIFY(popup->setProperty("parent", QVariant::fromValue(secondHost)));
        QCOMPARE(content->property("observedElevation").toInt(), 4);

        popupTheme->set_elevation(8);
        QCOMPARE(content->property("observedElevation").toInt(), 8);
    }

    void rejectsCycle() {
        QQmlEngine engine;
        addImportPaths(engine);
        auto root = create(engine,
                           R"(
            import QtQuick
            import Qcm.Material as MD

            Item {
                Item { objectName: "first"; MD.MProp.elevation: 1 }
                Item { objectName: "second"; MD.MProp.elevation: 2 }
            }
        )",
                           QStringLiteral("cycle.qml"));
        QVERIFY(root);

        auto* first  = root->findChild<QQuickItem*>(QStringLiteral("first"));
        auto* second = root->findChild<QQuickItem*>(QStringLiteral("second"));
        QVERIFY(first);
        QVERIFY(second);
        auto* firstTheme  = theme(first, false);
        auto* secondTheme = theme(second, false);
        QVERIFY(firstTheme);
        QVERIFY(secondTheme);

        firstTheme->setInheritFrom(second);
        QTest::ignoreMessage(
            QtWarningMsg,
            QRegularExpression(
                QStringLiteral(".*inheritFrom creates an attached property propagation cycle.*")));
        secondTheme->setInheritFrom(first);
        QCOMPARE(firstTheme->attachedParent(), secondTheme);
        QCOMPARE(secondTheme->attachedParent(), nullptr);
    }

    void rejectsCrossEngineParent() {
        QQmlEngine firstEngine;
        QQmlEngine secondEngine;
        addImportPaths(firstEngine);
        addImportPaths(secondEngine);

        auto firstRoot  = create(firstEngine,
                                 R"(
            import QtQuick
            import Qcm.Material as MD

            Item {
                MD.MProp.elevation: 1
                Item {
                    objectName: "child"
                    property int observedElevation: MD.MProp.elevation
                }
            }
        )",
                                 QStringLiteral("first-engine.qml"));
        auto secondRoot = create(secondEngine,
                                 R"(
            import QtQuick
            import Qcm.Material as MD

            Item { MD.MProp.elevation: 9 }
        )",
                                 QStringLiteral("second-engine.qml"));
        QVERIFY(firstRoot);
        QVERIFY(secondRoot);

        auto* child = firstRoot->findChild<QQuickItem*>(QStringLiteral("child"));
        QVERIFY(child);
        auto* childTheme = theme(child, false);
        QVERIFY(childTheme);
        QCOMPARE(child->property("observedElevation").toInt(), 1);

        QTest::ignoreMessage(QtWarningMsg,
                             QRegularExpression(QStringLiteral(
                                 ".*inheritFrom cannot cross QQmlEngine boundaries.*")));
        childTheme->setInheritFrom(secondRoot.get());
        QCOMPARE(child->property("observedElevation").toInt(), 0);

        childTheme->resetInheritFrom();
        QCOMPARE(child->property("observedElevation").toInt(), 1);
    }
};

QTEST_MAIN(AttachedPropertyPropagationTest)

#include "attached_property_propagation.moc"
