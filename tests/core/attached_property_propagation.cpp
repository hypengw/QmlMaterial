#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QRegularExpression>
#include <QtQml/qqml.h>
#include <QtTest>
#include <memory>
#include <functional>
#include <utility>
#include <vector>

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
    QML_ATTACHED(CountingAttached)

public:
    explicit CountingAttached(QObject* target)
        : AttachedPropertyNode(target, &CountingAttached::staticMetaObject) {
        initializeAttachedProperty();
    }
    static CountingAttached* qmlAttachedProperties(QObject* target) {
        return new CountingAttached(target);
    }

    int                   updateCount() const { return m_update_count; }
    void                  resetUpdateCount() { m_update_count = 0; }
    std::function<void()> onUpdate;

protected:
    void updateInheritedValues() override {
        ++m_update_count;
        auto callback = onUpdate;
        if (callback) callback();
    }

private:
    int m_update_count { 0 };
};

CountingAttached* counting(QObject* target, bool create = true) {
    return qobject_cast<CountingAttached*>(
        qmlAttachedPropertiesObject<CountingAttached>(target, create));
}

class LogicalObject final : public QObject {
public:
    explicit LogicalObject(QQuickItem* parent): QObject(parent), m_parent(parent) {}
    CountingAttached* attach() {
        auto* node = counting(this);
        node->setInheritanceParent(m_parent);
        return node;
    }
    void setParentItem(QQuickItem* parent) {
        m_parent = parent;
        if (auto* node = counting(this, false)) node->setInheritanceParent(parent);
    }

private:
    QPointer<QQuickItem> m_parent;
};

class AttachedPropertyPropagationTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() {
        qmlRegisterUncreatableType<CountingAttached>(
            "Test.Propagation", 1, 0, "CountingAttached", "Attached only");
    }
    void logicalReparentingDoesNotRefreshUnrelatedNodes() {
        QQmlEngine engine;
        auto       root = create(engine,
                                 "import QtQuick\nItem { Item { objectName: 'second' } }",
                                 "logical-reparenting.qml");
        QVERIFY(root);
        auto* item   = qobject_cast<QQuickItem*>(root.get());
        auto* second = root->findChild<QQuickItem*>("second");
        counting(item);
        auto*         secondNode = counting(second);
        LogicalObject moving(item);
        LogicalObject unrelated(item);
        QQmlEngine::setContextForObject(&moving, engine.rootContext());
        QQmlEngine::setContextForObject(&unrelated, engine.rootContext());
        auto* movingNode    = moving.attach();
        auto* unrelatedNode = unrelated.attach();
        unrelatedNode->resetUpdateCount();
        moving.setParentItem(second);
        QCOMPARE(movingNode->attachedParent(), secondNode);
        QCOMPARE(unrelatedNode->updateCount(), 0);
    }
    void engineTeardown_data() {
        QTest::addColumn<bool>("owned");
        QTest::addColumn<bool>("pending");
        QTest::newRow("external-root") << false << false;
        QTest::newRow("engine-root") << true << false;
        QTest::newRow("external-root-pending") << false << true;
        QTest::newRow("engine-root-pending") << true << true;
    }
    void engineTeardown() {
        QFETCH(bool, owned);
        QFETCH(bool, pending);
        auto                     engine = std::make_unique<QQmlApplicationEngine>();
        const QByteArray         qml    = "import QtQuick\nItem { Item { objectName: 'second' } }";
        std::unique_ptr<QObject> external;
        QPointer<QObject>        root;
        if (owned) {
            engine->loadData(qml);
            QVERIFY(! engine->rootObjects().isEmpty());
            root = engine->rootObjects().first();
        } else {
            external = create(*engine, qml, "engine-teardown.qml");
            root     = external.get();
        }
        QVERIFY(root);
        auto*                      item       = qobject_cast<QQuickItem*>(root.data());
        auto*                      second     = root->findChild<QQuickItem*>("second");
        QPointer<CountingAttached> rootNode   = counting(item);
        auto*                      secondNode = counting(second);
        auto*                      moving     = new QQuickItem(item);
        QQmlEngine::setContextForObject(moving, engine->rootContext());
        QPointer<CountingAttached> movingNode = counting(moving);
        QCOMPARE(movingNode->attachedParent(), rootNode.data());
        QCoreApplication::sendPostedEvents();
        if (pending)
            QMetaObject::invokeMethod(
                movingNode,
                [moving, second] {
                    moving->setParentItem(second);
                },
                Qt::QueuedConnection);
        engine.reset();
        QCoreApplication::sendPostedEvents();
        QCOMPARE(root.isNull(), owned);
        if (! owned) {
            QVERIFY(rootNode && movingNode);
            QCOMPARE(movingNode->attachedParent(), pending ? secondNode : rootNode.data());
            QVERIFY(rootNode->attachedChildren().contains(secondNode));
        }
        external.reset();
        QVERIFY(! rootNode && ! movingNode);
    }
    void unrelatedRegistrationDoesNotSearchLogicalParents() {
        QQmlEngine engine;
        auto       root = create(engine,
                                 "import QtQuick\nItem { Item { objectName: 'branch' } }",
                                 "unrelated-registration.qml");
        QVERIFY(root);
        auto* item   = qobject_cast<QQuickItem*>(root.get());
        auto* branch = root->findChild<QQuickItem*>("branch");
        counting(item);
        std::vector<std::unique_ptr<LogicalObject>> unrelated;
        for (int i = 0; i < 64; ++i) {
            auto target = std::make_unique<LogicalObject>(item);
            QQmlEngine::setContextForObject(target.get(), engine.rootContext());
            target->attach();
            unrelated.push_back(std::move(target));
        }
        for (const auto& target : unrelated) counting(target.get())->resetUpdateCount();
        std::vector<std::unique_ptr<QQuickItem>> leaves;
        for (int i = 0; i < 64; ++i) {
            auto leaf = std::make_unique<QQuickItem>(branch);
            QQmlEngine::setContextForObject(leaf.get(), engine.rootContext());
            counting(leaf.get());
            leaves.push_back(std::move(leaf));
        }
        for (const auto& target : unrelated) QCOMPARE(counting(target.get())->updateCount(), 0);
    }
    void lateLogicalSource_data() {
        QTest::addColumn<bool>("visual");
        QTest::newRow("visual") << true;
        QTest::newRow("nonvisual") << false;
    }
    void lateLogicalSource() {
        QFETCH(bool, visual);
        QQmlEngine engine;
        auto       root = create(engine,
                                 "import QtQuick\nItem { Item { objectName: 'source' } }",
                                 "late-logical-source.qml");
        QVERIFY(root);
        auto*                    item     = qobject_cast<QQuickItem*>(root.get());
        auto*                    source   = root->findChild<QQuickItem*>("source");
        auto*                    rootNode = counting(item);
        std::unique_ptr<QObject> target(visual ? static_cast<QObject*>(new QQuickItem(item))
                                               : new LogicalObject(item));
        QQmlEngine::setContextForObject(target.get(), engine.rootContext());
        auto* child = counting(target.get());
        child->setInheritFrom(source);
        auto* sourceNode = counting(source, false);
        QVERIFY(sourceNode);
        QCOMPARE(child->attachedParent(), sourceNode);
        QCOMPARE(sourceNode->attachedParent(), rootNode);
        QCOMPARE(counting(source), sourceNode);
        target.reset();
        QVERIFY(sourceNode->attachedChildren().isEmpty());
    }
    void explicitNonvisualParent() {
        QQmlEngine engine;
        auto       root = create(engine,
                                 "import QtQuick\nItem { Item { objectName: 'first' } "
                                 "Item { objectName: 'second' } }",
                                 "late-content-root.qml");
        QVERIFY(root);
        auto*   item       = qobject_cast<QQuickItem*>(root.get());
        auto*   first      = root->findChild<QQuickItem*>("first");
        auto*   second     = root->findChild<QQuickItem*>("second");
        auto*   rootNode   = counting(item);
        auto*   firstNode  = counting(first);
        auto*   secondNode = counting(second);
        QObject holder;
        QQmlEngine::setContextForObject(&holder, engine.rootContext());
        auto* node = counting(&holder);
        QVERIFY(! node->attachedParent());
        holder.setParent(first);
        QVERIFY(! node->attachedParent());
        node->setInheritanceParent(first);
        QCOMPARE(node->attachedParent(), firstNode);
        holder.setParent(second);
        QCOMPARE(node->attachedParent(), firstNode);
        node->setInheritanceParent(second);
        QCOMPARE(node->attachedParent(), secondNode);
        node->setInheritanceParent(nullptr);
        QVERIFY(! node->attachedParent());
        holder.setParent(nullptr);
        QCOMPARE(firstNode->attachedParent(), rootNode);
    }
    void logicalSourceFollowsParent() {
        QQmlEngine engine;
        auto       root = create(
            engine,
            "import QtQuick\nItem { Item { objectName: 'first' } Item { objectName: 'second' } }",
            "source-move.qml");
        QVERIFY(root);
        auto* item       = qobject_cast<QQuickItem*>(root.get());
        auto* first      = root->findChild<QQuickItem*>("first");
        auto* second     = root->findChild<QQuickItem*>("second");
        auto* firstNode  = counting(first);
        auto* secondNode = counting(second);
        auto* source     = new QQuickItem(first);
        auto* target     = new QQuickItem(item);
        QQmlEngine::setContextForObject(source, engine.rootContext());
        QQmlEngine::setContextForObject(target, engine.rootContext());
        auto* child = counting(target);
        QVERIFY(! counting(source, false));
        child->setInheritFrom(source);
        auto* sourceNode = counting(source, false);
        QVERIFY(sourceNode);
        QCOMPARE(child->attachedParent(), sourceNode);
        QCOMPARE(sourceNode->attachedParent(), firstNode);
        source->setParentItem(second);
        QCOMPARE(child->attachedParent(), sourceNode);
        QCOMPARE(sourceNode->attachedParent(), secondNode);
    }
    void sourceNotificationMayDeleteTarget_data() {
        QTest::addColumn<bool>("destroySource");
        QTest::newRow("source-change") << false;
        QTest::newRow("source-destroyed") << true;
    }
    void sourceNotificationMayDeleteTarget() {
        QFETCH(bool, destroySource);
        QQmlEngine engine;
        auto       root = create(engine, "import QtQuick\nItem {}", "source-notification.qml");
        QVERIFY(root);
        auto* item   = qobject_cast<QQuickItem*>(root.get());
        auto* source = new QQuickItem(item);
        auto* target = new QQuickItem(item);
        QQmlEngine::setContextForObject(source, engine.rootContext());
        QQmlEngine::setContextForObject(target, engine.rootContext());
        QPointer<CountingAttached> child = counting(target);
        if (destroySource) child->setInheritFrom(source);
        connect(child, &CountingAttached::inheritFromChanged, &engine, [target] {
            delete target;
        });
        if (destroySource)
            delete source;
        else
            child->setInheritFrom(source);
        QVERIFY(! child);
    }
    void sourceResolutionMayDeleteTarget() {
        QQmlEngine engine;
        auto       root = create(engine, "import QtQuick\nItem {}", "source-resolution.qml");
        QVERIFY(root);
        auto* item   = qobject_cast<QQuickItem*>(root.get());
        auto* source = new QQuickItem(item);
        auto* target = new QQuickItem(item);
        QQmlEngine::setContextForObject(source, engine.rootContext());
        QQmlEngine::setContextForObject(target, engine.rootContext());
        QPointer<CountingAttached> child = counting(target);
        child->onUpdate                  = [target] {
            delete target;
        };
        child->setInheritFrom(source);
        QVERIFY(! child);
    }
    void sourceResolutionMayReplaceSource() {
        QQmlEngine engine;
        auto       root = create(
            engine,
            "import QtQuick\nItem { Item { objectName: 'first' } Item { objectName: 'second' } }",
            "source-replacement.qml");
        QVERIFY(root);
        auto* item   = qobject_cast<QQuickItem*>(root.get());
        auto* first  = root->findChild<QQuickItem*>("first");
        auto* second = root->findChild<QQuickItem*>("second");
        auto* target = new QQuickItem(item);
        QQmlEngine::setContextForObject(target, engine.rootContext());
        auto* child     = counting(target);
        bool  replaced  = false;
        child->onUpdate = [&] {
            if (std::exchange(replaced, true)) return;
            child->setInheritFrom(second);
        };
        QSignalSpy changed(child, &CountingAttached::inheritFromChanged);
        child->setInheritFrom(first);
        QVERIFY(replaced);
        QCOMPARE(child->inheritFrom(), second);
        QCOMPARE(child->attachedParent(), counting(second));
        QCOMPARE(changed.size(), 1);
    }
    void lateAdoptionMayDeleteSibling() {
        QQmlEngine engine;
        auto       root = create(
            engine,
            "import QtQuick\nItem { Item { objectName: 'first' } Item { objectName: 'second' } }",
            "adoption-callback.qml");
        QVERIFY(root);
        auto*                      first      = root->findChild<QQuickItem*>("first");
        auto*                      second     = root->findChild<QQuickItem*>("second");
        auto*                      firstNode  = counting(first);
        QPointer<CountingAttached> secondNode = counting(second);
        firstNode->onUpdate                   = [firstNode, second] {
            firstNode->onUpdate = {};
            delete second;
        };
        auto* owner = counting(root.get());
        QVERIFY(! secondNode);
        QCOMPARE(firstNode->attachedParent(), owner);
    }
    void descendantNotificationMayDeleteOwner() {
        QQmlEngine engine;
        addImportPaths(engine);
        auto root =
            create(engine,
                   "import QtQuick\nimport Qcm.Material as MD\nItem { MD.MProp.elevation: 1; Item "
                   "{ objectName: 'child'; property int inherited: MD.MProp.elevation } }",
                   "descendant-notification.qml");
        QVERIFY(root);
        QPointer<qml_material::Theme> owner = theme(root.get(), false);
        auto*                         child = theme(root->findChild<QQuickItem*>("child"), false);
        QVERIFY(owner && child);
        connect(child, &qml_material::Theme::elevationChanged, &engine, [&] {
            root.reset();
        });
        owner->set_elevation(5);
        QVERIFY(! owner);
        QVERIFY(! root);
    }
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
        auto* firstNode     = counting(first);
        auto* movingNode    = counting(moving);
        auto* secondNode    = counting(second);
        auto* unrelatedNode = counting(unrelated);
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
        QVERIFY(contentTheme->attachedParent());
        QCOMPARE(contentTheme->attachedParent()->inheritFrom(), popup);
        QCOMPARE(content->property("observedElevation").toInt(), 3);

        QVERIFY(popup->setProperty("parent", QVariant::fromValue(secondHost)));
        QCOMPARE(content->property("observedElevation").toInt(), 4);

        popupTheme->set_elevation(8);
        QCOMPARE(content->property("observedElevation").toInt(), 8);

        auto* replacement = new QQuickItem;
        QQmlEngine::setContextForObject(replacement, engine.rootContext());
        auto* replacementTheme = theme(replacement);
        QVERIFY(popup->setProperty("contentItem", QVariant::fromValue(replacement)));
        QCOMPARE(replacementTheme->elevation(), 8);
        popupTheme->reset_elevation();
        QCOMPARE(replacementTheme->elevation(), 4);
    }

    void popupFollowsLogicalContext() {
        QQmlEngine engine;
        addImportPaths(engine);
        auto root = create(engine,
                           R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                Item {
                    objectName: "first"
                    MD.MProp.elevation: 3
                    Item { id: anchor; objectName: "anchor" }
                }
                Item { objectName: "second"; MD.MProp.elevation: 7 }
                MD.Popup {
                    id: popup
                    objectName: "popup"
                    parent: anchor
                    contentItem: Item { objectName: "content"; property int inherited: MD.MProp.elevation }
                }
            }
        )",
                           "popup-context.qml");
        QVERIFY(root);
        auto* anchor  = root->findChild<QQuickItem*>("anchor");
        auto* second  = root->findChild<QQuickItem*>("second");
        auto* content = root->findChild<QQuickItem*>("content");
        QVERIFY(anchor && second && content);
        QCOMPARE(content->property("inherited").toInt(), 3);
        anchor->setParentItem(second);
        QCOMPARE(content->property("inherited").toInt(), 7);
    }

    void popupFollowsLateAncestor() {
        QQmlEngine engine;
        addImportPaths(engine);
        auto root = create(engine,
                           R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                Item {
                    objectName: "scope"
                    Item { id: anchor; objectName: "anchor" }
                }
                MD.Popup {
                    parent: anchor
                    contentItem: Item { objectName: "content"; property int inherited: MD.MProp.elevation }
                }
            }
        )",
                           "popup-late-ancestor.qml");
        QVERIFY(root);
        auto* scope   = root->findChild<QQuickItem*>("scope");
        auto* content = root->findChild<QQuickItem*>("content");
        QVERIFY(scope && content);
        QVERIFY(! theme(scope, false));
        QCOMPARE(content->property("inherited").toInt(), 0);
        theme(scope)->set_elevation(9);
        QCOMPARE(content->property("inherited").toInt(), 9);
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

int run_attached_property_propagation(int argc, char** argv) {
    QCoreApplication::setAttribute(Qt::AA_Use96Dpi, true);
    AttachedPropertyPropagationTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "attached_property_propagation.moc"
