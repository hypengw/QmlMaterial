#include "qml_material/control/menu.hpp"
#include "qml_material/control/panel.hpp"
#include <QtQmlModels/private/qqmlobjectmodel_p.h>
#include <QtQuick/private/qquickitemview_p.h>
#include <QQmlInfo>
#include <QQmlComponent>
#include <QKeyEvent>
#include <algorithm>

namespace qml_material
{
Menu::Menu(QObject* parent): Popup(parent), m_model(new QQmlObjectModel(this)) {
    setModal(true);
    setDim(false);
    setFocus(true);
    connect(this, &Popup::aboutToShow, this, [this] {
        if (focus() && contentItem()) contentItem()->forceActiveFocus(Qt::PopupFocusReason);
    });
    connect(this, &Popup::contentItemChanged, this, &Menu::observeContent);
    observeContent();
}
Menu::~Menu() {
    disconnect(m_parentClosing);
    disconnect(m_parentDestroyed);
    for (const auto& connection : std::as_const(m_destroyConnections)) disconnect(connection);
    m_model->clear();
}
QObject*       Menu::contentModel() const { return m_model; }
QQmlComponent* Menu::delegate() const { return m_delegate; }
QQuickItem*    Menu::itemAt(int index) const { return m_items.value(index); }
int            Menu::indexOf(QQuickItem* item) const { return m_items.indexOf(item); }
void           Menu::setCurrentIndex(int value) {
    if (m_index == value) return;
    m_index = value;
    Q_EMIT currentIndexChanged();
}
void Menu::setTitle(const QString& value) {
    if (m_title == value) return;
    m_title = value;
    Q_EMIT titleChanged();
}
void Menu::setAutoClose(bool value) {
    if (m_autoClose == value) return;
    m_autoClose = value;
    Q_EMIT autoCloseChanged();
}
void Menu::observeContent() {
    if (m_keyItem) m_keyItem->removeEventFilter(this);
    m_keyItem = contentItem();
    if (m_keyItem) m_keyItem->installEventFilter(this);
}
void Menu::moveCurrent(int delta) {
    const QPointer<Menu> guard(this);
    for (int offset = 1; offset <= count(); ++offset) {
        const int index = (currentIndex() + delta * offset + count() * 2) % count();
        const QPointer<ButtonControl> item(qobject_cast<ButtonControl*>(itemAt(index)));
        if (! item || ! item->isEnabled() || ! item->isVisible()) continue;
        setCurrentIndex(index);
        if (! guard || ! item || itemAt(index) != item) return;
        if (auto* view = qobject_cast<QQuickItemView*>(contentItem()))
            view->positionViewAtIndex(index, QQuickItemView::Contain);
        if (guard && item) item->forceActiveFocus(Qt::TabFocusReason);
        return;
    }
}
bool Menu::eventFilter(QObject* object, QEvent* event) {
    if (object != m_keyItem || event->type() != QEvent::KeyPress)
        return Popup::eventFilter(object, event);
    const int key = static_cast<QKeyEvent*>(event)->key();
    if (key == Qt::Key_Down || key == Qt::Key_Up) {
        event->accept();
        moveCurrent(key == Qt::Key_Down ? 1 : -1);
        return true;
    }
    if (key == (mirrored() ? Qt::Key_Left : Qt::Key_Right)) {
        if (auto* item = dynamic_cast<MenuEntry*>(itemAt(currentIndex()));
            item && item->entry().subMenu()) {
            event->accept();
            if (auto* button = qobject_cast<ButtonControl*>(itemAt(currentIndex())))
                button->click();
            return true;
        }
    } else if (key == (mirrored() ? Qt::Key_Right : Qt::Key_Left)) {
        if (const QPointer<ButtonControl> item(qobject_cast<ButtonControl*>(parentItem())); item) {
            event->accept();
            close();
            if (item) item->forceActiveFocus(Qt::TabFocusReason);
            return true;
        }
    }
    return Popup::eventFilter(object, event);
}
void Menu::setDelegate(QQmlComponent* value) {
    if (m_delegate == value) return;
    m_delegate = value;
    Q_EMIT delegateChanged();
}
bool Menu::blocksScenePoint(const QPointF& point) const {
    // Cascading menus let the parent menu handle outside input.
    return Popup::blocksScenePoint(point) && (! m_parentMenu || ! m_parentMenu->isVisible());
}
void Menu::setParentMenu(Menu* value) {
    if (m_parentMenu == value) return;
    for (auto ancestor = value; ancestor; ancestor = ancestor->parentMenu()) {
        if (ancestor == this) {
            qmlWarning(this) << "Menu parent chain cannot contain a cycle";
            return;
        }
    }
    disconnect(m_parentClosing);
    disconnect(m_parentDestroyed);
    m_parentMenu = value;
    if (value) {
        m_parentClosing   = connect(value, &Popup::aboutToHide, this, &Popup::close);
        m_parentDestroyed = connect(value, &QObject::destroyed, this, [this] {
            QPointer<Menu> guard(this);
            dismissImmediately();
            if (guard) Q_EMIT parentMenuChanged();
        });
    }
    Q_EMIT parentMenuChanged();
}
void Menu::addItem(QQuickItem* item) { insertItem(count(), item); }
void Menu::insertItem(int index, QQuickItem* item) {
    if (! item || item == surfaceItem() || item == contentItem() ||
        item->isAncestorOf(surfaceItem()))
        return;
    if (index < 0 || index > count()) index = count();
    const int old = indexOf(item);
    if (old >= 0) {
        moveItem(old, old < index ? index - 1 : index);
        return;
    }
    QPointer<Menu>       guard(this);
    QPointer<QQuickItem> target(item);
    auto                 menuItem = dynamic_cast<MenuEntry*>(item);
    if (menuItem && menuItem->entry().menu() && menuItem->entry().menu() != this) {
        auto owner = menuItem->entry().menu();
        owner->takeItem(owner->indexOf(item));
        if (! guard || ! target) return;
    }
    m_items.insert(index, item);
    m_destroyConnections.insert(item, connect(item, &QObject::destroyed, this, [this, item] {
                                    removeAt(indexOf(item));
                                }));
    if (menuItem) menuItem->entry().setMenu(this);
    if (! guard || ! target) return;
    m_model->insert(index, item);
    if (! guard) return;
    if (m_index >= index) setCurrentIndex(m_index + 1);
    if (guard) Q_EMIT countChanged();
}
void Menu::moveItem(int from, int to) {
    if (from < 0 || from >= count()) return;
    if (to < 0 || to >= count()) to = count() - 1;
    if (from == to) return;
    auto selected = itemAt(m_index);
    m_items.move(from, to);
    QPointer<Menu> guard(this);
    m_model->move(from, to);
    if (! guard) return;
    if (selected) setCurrentIndex(indexOf(selected));
    if (guard) Q_EMIT countChanged();
}
void Menu::removeAt(int index) {
    if (index < 0 || index >= count()) return;
    QPointer<QQuickItem> item(m_items.takeAt(index));
    disconnect(m_destroyConnections.take(item));
    QPointer<Menu> guard(this);
    m_model->remove(index);
    if (! guard) return;
    if (auto menuItem = dynamic_cast<MenuEntry*>(item.data());
        menuItem && menuItem->entry().menu() == this)
        menuItem->entry().setMenu(nullptr);
    if (! guard) return;
    if (m_index == index)
        setCurrentIndex(-1);
    else if (m_index > index)
        setCurrentIndex(m_index - 1);
    if (guard) Q_EMIT countChanged();
}
QQuickItem* Menu::takeItem(int index) {
    QPointer<QQuickItem> item(itemAt(index));
    if (! item) return nullptr;
    removeAt(index);
    if (item) item->setParentItem(nullptr);
    return item;
}
void Menu::removeItem(QQuickItem* item) {
    if (auto taken = takeItem(indexOf(item))) taken->deleteLater();
}
void Menu::dismiss() {
    QPointer<Menu> current(this);
    while (current) {
        QPointer<Menu> parent(current->parentMenu());
        current->close();
        current = parent;
    }
}
void Menu::prepareForOpen() {
    QPointer<Menu> guard(this);
    Q_EMIT aboutToPrepare();
    if (! guard) return;
    if (auto view = qobject_cast<QQuickItemView*>(contentItem())) view->forceLayout();
}
void Menu::openFor(QQuickItem* anchor) {
    if (! anchor) {
        Q_EMIT openRejected(QStringLiteral("Menu requires an anchor"));
        return;
    }
    QPointer<Menu> guard(this);
    setAnchorItem(anchor);
    if (guard) open();
}
void Menu::popup(qreal x, qreal y) {
    if (anchorItem()) {
        Q_EMIT openRejected(
            QStringLiteral("Menu.popup coordinates cannot be combined with anchorItem"));
        return;
    }
    setX(x);
    setY(y);
    open();
}
void Menu::openSubMenu(Menu* child, QQuickItem* anchor) {
    if (! child || ! anchor || child == this) return;
    for (auto ancestor = parentMenu(); ancestor; ancestor = ancestor->parentMenu())
        if (ancestor == child) return;
    QPointer<Menu>       guard(this), target(child);
    QPointer<QQuickItem> item(anchor);
    if (m_childMenu && m_childMenu != child) m_childMenu->close();
    if (! guard || ! target || ! item) return;
    m_childMenu = child;
    child->setParentMenu(this);
    if (! guard || ! target || ! item) return;
    child->setParentItem(item);
    if (! guard || ! target || ! item) return;
    child->setPreferredPlacement(Trailing);
    if (! guard || ! target || ! item) return;
    child->setAnchorAlignment(Start);
    if (! guard || ! target || ! item) return;
    child->openFor(item);
}
MenuEntryController::MenuEntryController(ButtonControl* item, std::function<void()> menuChanged,
                                         std::function<void()> subMenuChanged)
    : m_item(item),
      m_menuChanged(std::move(menuChanged)),
      m_subMenuChanged(std::move(subMenuChanged)) {
    m_sourceConnection =
        QObject::connect(item, &ButtonControl::interactionSourceChanged, item, [this] {
            observeHover();
        });
}
MenuEntryController::~MenuEntryController() {
    QObject::disconnect(m_hoverConnection);
    QObject::disconnect(m_menuDestroyed);
    QObject::disconnect(m_subMenuDestroyed);
    QObject::disconnect(m_sourceConnection);
}
void MenuEntryController::observeHover() {
    QObject::disconnect(m_hoverConnection);
    const auto owner = m_menu;
    const auto item  = m_item;
    if (! owner || ! item) return;
    auto* source = item->interactionSource();
    if (! item || m_menu != owner) return;
    m_hoverConnection = QObject::connect(
        source,
        &InteractionSource::interaction,
        item,
        [this, owner, item](const Interaction& event) {
            if (! owner || ! item || m_menu != owner || ! item->isEnabled() ||
                ! item->isVisible() || event.originItem() != item ||
                event.kind != Interaction::Hover || event.phase != Interaction::Begin)
                return;
            const int index = owner->indexOf(item);
            if (index >= 0) owner->setCurrentIndex(index);
        });
}
void MenuEntryController::setMenu(Menu* value) {
    if (m_menu == value && (value || ! m_menuDestroyed)) return;
    QObject::disconnect(m_menuDestroyed);
    m_menu = value;
    if (value)
        m_menuDestroyed = QObject::connect(value, &QObject::destroyed, m_item, [this] {
            setMenu(nullptr);
        });
    const auto revision = ++m_revision;
    const auto item     = m_item;
    observeHover();
    if (! item || revision != m_revision) return;
    item->cancelActivation();
    if (item && revision == m_revision) {
        const auto notify = m_menuChanged;
        notify();
    }
}
void MenuEntryController::setSubMenu(Menu* value) {
    if (m_subMenu == value && (value || ! m_subMenuDestroyed)) return;
    QObject::disconnect(m_subMenuDestroyed);
    m_subMenu = value;
    if (value)
        m_subMenuDestroyed = QObject::connect(value, &QObject::destroyed, m_item, [this] {
            setSubMenu(nullptr);
        });
    const auto revision = ++m_revision;
    const auto item     = m_item;
    item->cancelActivation();
    if (item && revision == m_revision) {
        const auto notify = m_subMenuChanged;
        notify();
    }
}
void MenuEntryController::activate(std::function<void()> domain, std::function<void()> triggered) {
    const auto item = m_item;
    if (! item || ! item->canActivate()) return;
    const auto revision = m_revision;
    const auto owner = m_menu, child = m_subMenu;
    if (child) {
        if (owner)
            owner->openSubMenu(child, item);
        else {
            child->setParentItem(item);
            if (! item || ! child || revision != m_revision) return;
            child->setPreferredPlacement(Popup::Trailing);
            if (! item || ! child || revision != m_revision) return;
            child->setAnchorAlignment(Popup::Start);
            if (item && child && revision == m_revision) child->openFor(item);
        }
        return;
    }
    const bool close =
        owner && (owner->autoClose() || dynamic_cast<MenuEntry*>(item.data())->closeOnActivation());
    domain();
    if (! item || revision != m_revision) return;
    triggered();
    if (item && revision == m_revision && owner && close) owner->dismiss();
}
} // namespace qml_material
