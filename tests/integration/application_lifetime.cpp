#include "qml_material/control/control.hpp"
#include "qml_material/control/label.hpp"
#include "qml_material/control/popup.hpp"
#include "qml_material/control/text_field.hpp"
#include <QGuiApplication>
#include <QtTest>
#include <memory>

class EnvironmentControl : public qml_material::Control {
public:
    using Control::Control;
    int fontUpdates = 0, localeUpdates = 0, directionUpdates = 0, hoverUpdates = 0;

    void inheritFont(const QFont& value) override {
        ++fontUpdates;
        Control::inheritFont(value);
    }
    void inheritLocale(const QLocale& value) override {
        ++localeUpdates;
        Control::inheritLocale(value);
    }
    void inheritLayoutDirection(Qt::LayoutDirection value) override {
        ++directionUpdates;
        Control::inheritLayoutDirection(value);
    }
    void inheritHoverEnabled(bool value) override {
        ++hoverUpdates;
        Control::inheritHoverEnabled(value);
    }
    void clearUpdates() { fontUpdates = localeUpdates = directionUpdates = hoverUpdates = 0; }
};

class ApplicationLifetimeTest : public QObject {
    Q_OBJECT
private slots:
    void propertyPropagation() {
        int                   argc   = 1;
        char                  name[] = "application_lifetime";
        char*                 argv[] = { name, nullptr };
        QGuiApplication       app(argc, argv);
        qml_material::Control root;
        root.setHoverEnabled(false);
        root.setLocale(QLocale("en_US"));
        root.setLayoutDirection(Qt::LeftToRight);
        QQuickItem              bridge(&root);
        qml_material::Label     label(&bridge);
        qml_material::TextField field(&label);
        EnvironmentControl      child(&field);
        EnvironmentControl      leaf(&child);
        child.clearUpdates();
        leaf.clearUpdates();
        QFont font;
        font.setPixelSize(37);
        root.setFont(font);
        QCOMPARE(label.font().pixelSize(), 37);
        QCOMPARE(field.font().pixelSize(), 37);
        QCOMPARE(leaf.font().pixelSize(), 37);
        QCOMPARE(child.fontUpdates, 1);
        QCOMPARE(leaf.fontUpdates, 1);
        QCOMPARE(child.localeUpdates, 0);
        QCOMPARE(child.directionUpdates, 0);
        QCOMPARE(child.hoverUpdates, 0);

        child.clearUpdates();
        root.setLocale(QLocale("de_DE"));
        QCOMPARE(leaf.locale(), QLocale("de_DE"));
        QCOMPARE(child.localeUpdates, 1);
        QCOMPARE(child.fontUpdates, 0);
        QCOMPARE(child.directionUpdates, 0);
        QCOMPARE(child.hoverUpdates, 0);

        child.clearUpdates();
        root.setLayoutDirection(Qt::RightToLeft);
        QCOMPARE(leaf.layoutDirection(), Qt::RightToLeft);
        QCOMPARE(child.directionUpdates, 1);
        QCOMPARE(child.fontUpdates, 0);
        QCOMPARE(child.localeUpdates, 0);
        QCOMPARE(child.hoverUpdates, 0);

        child.clearUpdates();
        root.setHoverEnabled(true);
        QVERIFY(field.hoverEnabled());
        QVERIFY(leaf.hoverEnabled());
        QCOMPARE(child.hoverUpdates, 1);
        QCOMPARE(child.fontUpdates, 0);
        QCOMPARE(child.localeUpdates, 0);
        QCOMPARE(child.directionUpdates, 0);

        child.setLocale(QLocale("fr_FR"));
        child.setLayoutDirection(Qt::LeftToRight);
        field.setHoverEnabled(false);
        QFont childFont;
        childFont.setPixelSize(19);
        child.setFont(childFont);
        leaf.clearUpdates();
        root.setLocale(QLocale("zh_CN"));
        root.setLayoutDirection(Qt::LeftToRight);
        root.setHoverEnabled(false);
        root.setHoverEnabled(true);
        QCOMPARE(leaf.localeUpdates, 0);
        QCOMPARE(leaf.directionUpdates, 0);
        QCOMPARE(leaf.hoverUpdates, 0);
        QCOMPARE(leaf.locale(), QLocale("fr_FR"));
        QCOMPARE(leaf.layoutDirection(), Qt::LeftToRight);
        QVERIFY(! leaf.hoverEnabled());
        font.setPixelSize(41);
        font.setBold(true);
        root.setFont(font);
        QCOMPARE(leaf.font().pixelSize(), 19);
        QVERIFY(leaf.font().bold());
        child.resetFont();
        child.resetLocale();
        child.resetLayoutDirection();
        field.resetHoverEnabled();
        QCOMPARE(leaf.font().pixelSize(), 41);
        QCOMPARE(leaf.locale(), root.locale());
        QCOMPARE(leaf.layoutDirection(), root.layoutDirection());
        QVERIFY(leaf.hoverEnabled());

        child.clearUpdates();
        QSignalSpy fontChanges(&label, &qml_material::Label::fontChanged);
        const auto oldMask = leaf.effectiveFont().resolveMask();
        font.setItalic(root.font().italic());
        root.setFont(font);
        QCOMPARE(fontChanges.count(), 0);
        QCOMPARE(child.fontUpdates, 1);
        QVERIFY(leaf.effectiveFont().resolveMask() != oldMask);
        QCOMPARE(leaf.effectiveFont().resolveMask(), root.effectiveFont().resolveMask());
        QCOMPARE(child.hoverUpdates, 0);
        QCOMPARE(child.localeUpdates, 0);
        QCOMPARE(child.directionUpdates, 0);
    }

    void popupLogicalEnvironment() {
        int                   argc   = 1;
        char                  name[] = "application_lifetime";
        char*                 argv[] = { name, nullptr };
        QGuiApplication       app(argc, argv);
        qml_material::Control root;
        root.setLocale(QLocale("en_US"));
        root.setHoverEnabled(false);
        root.setLayoutDirection(Qt::LeftToRight);
        qml_material::Popup popup;
        popup.setParentItem(&root);
        EnvironmentControl    child(popup.surfaceItem());
        qml_material::Control visualParent;
        popup.surfaceItem()->setParentItem(&visualParent);
        child.clearUpdates();
        QFont font;
        font.setPixelSize(37);
        root.setFont(font);
        QCOMPARE(child.font().pixelSize(), 37);
        QCOMPARE(child.fontUpdates, 1);
        QCOMPARE(child.localeUpdates, 0);
        QCOMPARE(child.directionUpdates, 0);
        QCOMPARE(child.hoverUpdates, 0);
        root.setLocale(QLocale("de_DE"));
        root.setLayoutDirection(Qt::RightToLeft);
        root.setHoverEnabled(true);
        QCOMPARE(child.locale(), root.locale());
        QCOMPARE(child.layoutDirection(), root.layoutDirection());
        QCOMPARE(child.hoverEnabled(), root.hoverEnabled());
        popup.setLocale(QLocale("fr_FR"));
        root.setLocale(QLocale("zh_CN"));
        QCOMPARE(child.locale(), QLocale("fr_FR"));
        popup.resetLocale();
        QCOMPARE(child.locale(), root.locale());
        popup.surfaceItem()->setParentItem(nullptr);
        auto logicalParent = std::make_unique<QQuickItem>(&root);
        popup.setParentItem(logicalParent.get());
        logicalParent.reset();
        QVERIFY(! popup.parentItem());
        child.clearUpdates();
        font.setPixelSize(41);
        root.setFont(font);
        QCOMPARE(child.font().pixelSize(), 37);
        QCOMPARE(child.fontUpdates, 0);
    }

    void lateEnvironmentUpdates() {
        std::unique_ptr<qml_material::Control> root;
        qml_material::Control*                 child = nullptr;
        qml_material::Label*                   label = nullptr;
        qml_material::TextField*               field = nullptr;
        qml_material::Popup*                   popup = nullptr;
        {
            int             argc   = 1;
            char            name[] = "application_lifetime";
            char*           argv[] = { name, nullptr };
            QGuiApplication app(argc, argv);
            root = std::make_unique<qml_material::Control>();
            QFont font;
            font.setPixelSize(37);
            root->setFont(font);
            root->setHoverEnabled(true);
            child = new qml_material::Control(root.get());
            label = new qml_material::Label(root.get());
            field = new qml_material::TextField(root.get());
            popup = new qml_material::Popup(root.get());
            popup->setParentItem(root.get());
            QCOMPARE(child->font().pixelSize(), 37);
            QCOMPARE(label->font().pixelSize(), 37);
            QCOMPARE(field->font().pixelSize(), 37);
            QCOMPARE(popup->font().pixelSize(), 37);
            QVERIFY(child->hoverEnabled());
            QVERIFY(field->hoverEnabled());
            QSignalSpy childFontChanges(child, &qml_material::Control::fontChanged);
            QSignalSpy labelFontChanges(label, &qml_material::Label::fontChanged);
            QSignalSpy fieldFontChanges(field, &qml_material::TextField::fontChanged);
            child->setParentItem(nullptr);
            label->setParentItem(nullptr);
            field->setParentItem(nullptr);
            popup->setParentItem(nullptr);
            QCOMPARE(child->font().pixelSize(), 37);
            QCOMPARE(label->font().pixelSize(), 37);
            QCOMPARE(field->font().pixelSize(), 37);
            QCOMPARE(popup->font().pixelSize(), 37);
            QVERIFY(child->hoverEnabled());
            QVERIFY(field->hoverEnabled());
            QCOMPARE(childFontChanges.count(), 0);
            QCOMPARE(labelFontChanges.count(), 0);
            QCOMPARE(fieldFontChanges.count(), 0);

            qml_material::Control other;
            font.setPixelSize(19);
            other.setFont(font);
            other.setHoverEnabled(false);
            child->setParentItem(&other);
            label->setParentItem(&other);
            field->setParentItem(&other);
            popup->setParentItem(&other);
            QCOMPARE(child->font().pixelSize(), 19);
            QCOMPARE(label->font().pixelSize(), 19);
            QCOMPARE(field->font().pixelSize(), 19);
            QCOMPARE(popup->font().pixelSize(), 19);
            QVERIFY(! child->hoverEnabled());
            QVERIFY(! field->hoverEnabled());
            child->setParentItem(root.get());
            label->setParentItem(root.get());
            field->setParentItem(root.get());
            popup->setParentItem(root.get());
            QCOMPARE(child->font().pixelSize(), 37);
        }
        QVERIFY(! QCoreApplication::instance());
        QVERIFY(QCoreApplication::closingDown());
        QTest::failOnWarning();
        child->setParentItem(nullptr);
        label->setParentItem(nullptr);
        field->setParentItem(nullptr);
        popup->setParentItem(nullptr);
        QCOMPARE(child->font().pixelSize(), 37);
        QCOMPARE(label->font().pixelSize(), 37);
        QCOMPARE(field->font().pixelSize(), 37);
        QCOMPARE(popup->font().pixelSize(), 37);
        QVERIFY(child->hoverEnabled());
        QVERIFY(field->hoverEnabled());
        root.reset();
    }
};

int run_application_lifetime(int argc, char** argv) {
    ApplicationLifetimeTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "application_lifetime.moc"
