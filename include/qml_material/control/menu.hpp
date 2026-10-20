#pragma once

#include "qml_material/control/popup.hpp"
#include "qml_material/control/selection_button.hpp"
#include "qml_material/control/toggle_button.hpp"
#include "qml_material/control/combined_command_button.hpp"

QT_BEGIN_NAMESPACE
class QQmlComponent;
class QQmlObjectModel;
QT_END_NAMESPACE

Q_MOC_INCLUDE("QtQml/QQmlComponent")

namespace qml_material
{
class QML_MATERIAL_API Menu : public Popup {
    Q_OBJECT
    QML_NAMED_ELEMENT(MenuBase)
    Q_PROPERTY(QObject* contentModel READ contentModel CONSTANT FINAL)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_PROPERTY(
        int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged FINAL)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged FINAL)
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    Q_PROPERTY(Menu* parentMenu READ parentMenu WRITE setParentMenu NOTIFY parentMenuChanged FINAL)
    Q_PROPERTY(bool autoClose READ autoClose WRITE setAutoClose NOTIFY autoCloseChanged FINAL)
public:
    explicit Menu(QObject* parent = nullptr);
    ~Menu() override;
    QObject*                contentModel() const;
    int                     count() const { return m_items.size(); }
    int                     currentIndex() const { return m_index; }
    void                    setCurrentIndex(int);
    QString                 title() const { return m_title; }
    void                    setTitle(const QString&);
    QQmlComponent*          delegate() const;
    void                    setDelegate(QQmlComponent*);
    Menu*                   parentMenu() const { return m_parentMenu; }
    void                    setParentMenu(Menu*);
    bool                    autoClose() const { return m_autoClose; }
    void                    setAutoClose(bool);
    Q_INVOKABLE QQuickItem* itemAt(int) const;
    Q_INVOKABLE int         indexOf(QQuickItem*) const;
    Q_INVOKABLE void        addItem(QQuickItem*);
    Q_INVOKABLE void        insertItem(int, QQuickItem*);
    Q_INVOKABLE void        moveItem(int, int);
    Q_INVOKABLE QQuickItem* takeItem(int);
    Q_INVOKABLE void        removeItem(QQuickItem*);
    Q_INVOKABLE void        dismiss();
    Q_INVOKABLE void        openSubMenu(Menu*, QQuickItem* anchor);
    Q_INVOKABLE void        popup(qreal x, qreal y);
    Q_INVOKABLE void        openFor(QQuickItem* anchor);
    Q_SIGNAL void           aboutToPrepare();
    Q_SIGNAL void           countChanged();
    Q_SIGNAL void           currentIndexChanged();
    Q_SIGNAL void           titleChanged();
    Q_SIGNAL void           delegateChanged();
    Q_SIGNAL void           parentMenuChanged();
    Q_SIGNAL void           autoCloseChanged();
    bool                    blocksScenePoint(const QPointF&) const override;

protected:
    void prepareForOpen() override;
    bool eventFilter(QObject*, QEvent*) override;

private:
    void                                        observeContent();
    void                                        moveCurrent(int);
    void                                        removeAt(int);
    QQmlObjectModel*                            m_model;
    QList<QQuickItem*>                          m_items;
    QHash<QQuickItem*, QMetaObject::Connection> m_destroyConnections;
    QPointer<QQmlComponent>                     m_delegate;
    QPointer<Menu>                              m_parentMenu;
    QPointer<Menu>                              m_childMenu;
    QPointer<QQuickItem>                        m_keyItem;
    QMetaObject::Connection                     m_parentClosing;
    QMetaObject::Connection                     m_parentDestroyed;
    QString                                     m_title;
    int                                         m_index     = -1;
    bool                                        m_autoClose = false;
};
class QML_MATERIAL_API MenuEntryController {
public:
    MenuEntryController(ButtonControl*, std::function<void()>, std::function<void()>);
    ~MenuEntryController();
    Menu* menu() const { return m_menu; }
    void  setMenu(Menu*);
    Menu* subMenu() const { return m_subMenu; }
    void  setSubMenu(Menu*);
    void  activate(std::function<void()>, std::function<void()>);

private:
    void                    observeHover();
    QPointer<ButtonControl> m_item;
    QPointer<Menu>          m_menu, m_subMenu;
    QMetaObject::Connection m_menuDestroyed, m_subMenuDestroyed, m_hoverConnection,
        m_sourceConnection;
    std::function<void()> m_menuChanged, m_subMenuChanged;
    quint64               m_revision = 0;
};
class QML_MATERIAL_API MenuEntry {
public:
    virtual ~MenuEntry()                                   = default;
    virtual MenuEntryController& entry()                   = 0;
    virtual bool                 closeOnActivation() const = 0;
};
class QML_MATERIAL_API MenuItem : public CombinedCommandButton, public MenuEntry {
    Q_OBJECT
    QML_NAMED_ELEMENT(MenuItemBase)
    Q_PROPERTY(Menu* menu READ menu WRITE setMenu NOTIFY menuChanged FINAL)
    Q_PROPERTY(Menu* subMenu READ subMenu WRITE setSubMenu NOTIFY subMenuChanged FINAL)
    Q_PROPERTY(bool closeOnActivation READ closeOnActivation WRITE setCloseOnActivation NOTIFY
                   closeOnActivationChanged FINAL)
public:
    explicit MenuItem(QQuickItem* parent = nullptr)
        : CombinedCommandButton(parent),
          m_entry(
              this,
              [this] {
                  Q_EMIT menuChanged();
              },
              [this] {
                  Q_EMIT subMenuChanged();
              }) {}
    MenuEntryController& entry() override { return m_entry; }
    Menu*                menu() const { return m_entry.menu(); }
    void                 setMenu(Menu* value) { m_entry.setMenu(value); }
    Menu*                subMenu() const { return m_entry.subMenu(); }
    void                 setSubMenu(Menu* value) { m_entry.setSubMenu(value); }
    bool                 closeOnActivation() const override { return m_closeOnActivation; }
    void                 setCloseOnActivation(bool value) {
        if (m_closeOnActivation == value) return;
        m_closeOnActivation = value;
        Q_EMIT closeOnActivationChanged();
    }
    Q_SIGNAL void menuChanged();
    Q_SIGNAL void subMenuChanged();
    Q_SIGNAL void triggered();
    Q_SIGNAL void closeOnActivationChanged();

protected:
    void activate() override {
        m_entry.activate(
            [this] {
                CombinedCommandButton::activate();
            },
            [this] {
                Q_EMIT triggered();
            });
    }

private:
    MenuEntryController m_entry;
    bool                m_closeOnActivation = true;
};
class QML_MATERIAL_API SelectionMenuItem : public SelectionButton, public MenuEntry {
    Q_OBJECT
    QML_NAMED_ELEMENT(SelectionMenuItemBase)
    Q_PROPERTY(Menu* menu READ menu WRITE setMenu NOTIFY menuChanged FINAL)
    Q_PROPERTY(Menu* subMenu READ subMenu WRITE setSubMenu NOTIFY subMenuChanged FINAL)
    Q_PROPERTY(bool closeOnActivation READ closeOnActivation WRITE setCloseOnActivation NOTIFY
                   closeOnActivationChanged FINAL)
public:
    explicit SelectionMenuItem(QQuickItem* parent = nullptr)
        : SelectionButton(parent),
          m_entry(
              this,
              [this] {
                  Q_EMIT menuChanged();
              },
              [this] {
                  Q_EMIT subMenuChanged();
              }) {}
    MenuEntryController& entry() override { return m_entry; }
    Menu*                menu() const { return m_entry.menu(); }
    void                 setMenu(Menu* value) { m_entry.setMenu(value); }
    Menu*                subMenu() const { return m_entry.subMenu(); }
    void                 setSubMenu(Menu* value) { m_entry.setSubMenu(value); }
    bool                 closeOnActivation() const override { return m_closeOnActivation; }
    void                 setCloseOnActivation(bool value) {
        if (m_closeOnActivation == value) return;
        m_closeOnActivation = value;
        Q_EMIT closeOnActivationChanged();
    }
    Q_SIGNAL void menuChanged();
    Q_SIGNAL void subMenuChanged();
    Q_SIGNAL void triggered();
    Q_SIGNAL void closeOnActivationChanged();

protected:
    void activate() override {
        m_entry.activate(
            [this] {
                SelectionButton::activate();
            },
            [this] {
                Q_EMIT triggered();
            });
    }

private:
    MenuEntryController m_entry;
    bool                m_closeOnActivation = true;
};
class QML_MATERIAL_API ToggleMenuItem : public ToggleButton, public MenuEntry {
    Q_OBJECT
    QML_NAMED_ELEMENT(ToggleMenuItemBase)
    Q_PROPERTY(Menu* menu READ menu WRITE setMenu NOTIFY menuChanged FINAL)
    Q_PROPERTY(Menu* subMenu READ subMenu WRITE setSubMenu NOTIFY subMenuChanged FINAL)
    Q_PROPERTY(bool closeOnActivation READ closeOnActivation WRITE setCloseOnActivation NOTIFY
                   closeOnActivationChanged FINAL)
public:
    explicit ToggleMenuItem(QQuickItem* parent = nullptr)
        : ToggleButton(parent),
          m_entry(
              this,
              [this] {
                  Q_EMIT menuChanged();
              },
              [this] {
                  Q_EMIT subMenuChanged();
              }) {}
    MenuEntryController& entry() override { return m_entry; }
    Menu*                menu() const { return m_entry.menu(); }
    void                 setMenu(Menu* value) { m_entry.setMenu(value); }
    Menu*                subMenu() const { return m_entry.subMenu(); }
    void                 setSubMenu(Menu* value) { m_entry.setSubMenu(value); }
    bool                 closeOnActivation() const override { return m_closeOnActivation; }
    void                 setCloseOnActivation(bool value) {
        if (m_closeOnActivation == value) return;
        m_closeOnActivation = value;
        Q_EMIT closeOnActivationChanged();
    }
    Q_SIGNAL void menuChanged();
    Q_SIGNAL void subMenuChanged();
    Q_SIGNAL void triggered();
    Q_SIGNAL void closeOnActivationChanged();

protected:
    void activate() override {
        m_entry.activate(
            [this] {
                ToggleButton::activate();
            },
            [this] {
                Q_EMIT triggered();
            });
    }

private:
    MenuEntryController m_entry;
    bool                m_closeOnActivation = true;
};
} // namespace qml_material
