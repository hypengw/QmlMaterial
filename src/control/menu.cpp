#include "qml_material/control/menu.hpp"
#include <QtQmlModels/private/qqmlobjectmodel_p.h>
#include <QQmlInfo>
#include <algorithm>

namespace qml_material
{
Menu::Menu(QObject* parent): Popup(parent), m_model(new QQmlObjectModel(this)) {
    setFocus(true);
    connect(this, &Popup::aboutToShow, this, [this] {
        if (focus() && contentItem()) contentItem()->forceActiveFocus(Qt::PopupFocusReason);
    });
}
Menu::~Menu() {
    disconnect(m_parentClosing);
    disconnect(m_parentDestroyed);
    for (const auto& connection : std::as_const(m_destroyConnections)) disconnect(connection);
    m_model->clear();
}
QObject*    Menu::contentModel() const { return m_model; }
QQuickItem* Menu::itemAt(int index) const { return m_items.value(index); }
int         Menu::indexOf(QQuickItem* item) const { return m_items.indexOf(item); }
void        Menu::setCurrentIndex(int value) {
    if (m_index == value) return;
    m_index = value;
    Q_EMIT currentIndexChanged();
}
void Menu::setTitle(const QString& value) {
    if (m_title == value) return;
    m_title = value;
    Q_EMIT titleChanged();
}
void Menu::setDelegate(QQmlComponent* value) {
    if (m_delegate == value) return;
    m_delegate = value;
    Q_EMIT delegateChanged();
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
    m_items.insert(index, item);
    m_destroyConnections.insert(item, connect(item, &QObject::destroyed, this, [this, item] {
                                    removeAt(indexOf(item));
                                }));
    QPointer<Menu> guard(this);
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
    auto item = m_items.takeAt(index);
    disconnect(m_destroyConnections.take(item));
    QPointer<Menu> guard(this);
    m_model->remove(index);
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
void Menu::popup(qreal x, qreal y) {
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
    child->setX(mirrored() ? -child->width() : item->width());
    child->setY(0);
    child->open();
}
} // namespace qml_material
