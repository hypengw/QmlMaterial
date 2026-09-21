#include "qml_material/control/tab_bar.hpp"
#include "qml_material/util/qt.hpp"
#include <QKeyEvent>
#include <algorithm>

namespace qml_material
{
namespace
{
TabBarAttached* attached(TabButton* button) {
    return qobject_cast<TabBarAttached*>(qmlAttachedPropertiesObject<TabBar>(button, true));
}
} // namespace
TabBar::TabBar(QQuickItem* parent): Container(parent), m_group(new ButtonGroup(this)) {
    connect(this, &Control::availableWidthChanged, this, &QQuickItem::polish);
    connect(this, &Control::availableHeightChanged, this, &QQuickItem::polish);
    connect(this, &Control::spacingChanged, this, &QQuickItem::polish);
    connect(this, &Control::mirroredChanged, this, &QQuickItem::polish);
}
TabBar::~TabBar() {
    beginTeardown();
    for (auto& connections : m_connections) utils::disconnectAll(connections);
    const auto snapshot = items();
    for (auto item : snapshot)
        if (auto* button = qobject_cast<TabButton*>(item.data()))
            attached(button)->update(nullptr, -1);
}
TabBarAttached* TabBar::qmlAttachedProperties(QObject* object) {
    return new TabBarAttached(object);
}
bool TabBar::isContent(QQuickItem* item) const { return qobject_cast<TabButton*>(item); }
void TabBar::itemAdded(QQuickItem* item) {
    auto* button = qobject_cast<TabButton*>(item);
    if (! button) return;
    auto& connections = m_connections[button];
    connections.append(connect(button, &AbstractButton::checkedChanged, this, [this, button] {
        if (! m_selecting && isComponentComplete() && button->isChecked())
            setCurrentIndex(indexOf(button));
    }));
    connections.append(connect(button, &TabButton::fillWidthChanged, this, &QQuickItem::polish));
    connections.append(connect(button, &TabButton::fillHeightChanged, this, &QQuickItem::polish));
    connections.append(connect(button, &QObject::destroyed, this, [this, button] {
        m_connections.remove(button);
    }));
    m_group->addButton(button);
}
void TabBar::itemRemoved(QQuickItem* item) {
    auto connections = m_connections.take(item);
    utils::disconnectAll(connections);
    QPointer<TabBar>    guard(this);
    QPointer<TabButton> button(qobject_cast<TabButton*>(item));
    if (button) m_group->removeButton(button);
    if (guard && button && attached(button)->tabBar() == this)
        attached(button)->update(nullptr, -1);
}
void TabBar::itemsChanged() {
    QPointer<TabBar> guard(this);
    const auto       snapshot = items();
    for (int i = 0; i < snapshot.size(); ++i) {
        if (auto* button = qobject_cast<TabButton*>(snapshot[i].data());
            button && button->parentItem() == contentHost())
            attached(button)->update(this, i);
        if (! guard) return;
    }
}
void TabBar::currentItemChange() {
    if (m_selecting) {
        m_selection_dirty = true;
        return;
    }
    m_selecting = true;
    QPointer<TabBar> guard(this);
    do {
        m_selection_dirty   = false;
        const auto snapshot = items();
        for (auto item : snapshot) {
            if (auto* button = qobject_cast<TabButton*>(item.data());
                button && button->parentItem() == contentHost())
                button->setChecked(button == currentItem());
            if (! guard) return;
            if (m_selection_dirty) break;
        }
    } while (m_selection_dirty);
    m_selecting = false;
}
int TabBar::initialIndex() const {
    int index = Container::initialIndex();
    for (int i = 0; i < count(); ++i)
        if (itemAt(i)->isChecked()) index = i;
    return index;
}
void TabBar::setPosition(Position value) {
    if (m_position == value) return;
    m_position = value;
    QPointer<TabBar> guard(this);
    Q_EMIT positionChanged();
    if (! guard) return;
    const auto snapshot = items();
    for (auto item : snapshot) {
        if (auto* button = qobject_cast<TabButton*>(item.data()))
            Q_EMIT attached(button)->positionChanged();
        if (! guard) return;
    }
}
void TabBar::updatePolish() {
    QPointer<TabBar> guard(this);
    Container::updatePolish();
    if (! guard) return;
    const auto                 layoutRevision = revision();
    QList<QPointer<TabButton>> buttons;
    for (auto item : items()) buttons.append(qobject_cast<TabButton*>(item.data()));
    qreal preferred = std::max(0, count() - 1) * spacing(), reserved = preferred, height = 0;
    int   flexible = 0;
    for (auto button : buttons)
        if (button) {
            preferred += button->fillWidth() ? button->implicitWidth() : button->width();
            if (button->fillWidth())
                ++flexible;
            else
                reserved += button->width();
            height = std::max(height,
                              button->fillHeight() ? button->implicitHeight() : button->height());
        }
    const qreal width = std::max<qreal>(0, (availableWidth() - reserved) / std::max(1, flexible));
    qreal       total = reserved + flexible * width;
    qreal       x     = mirrored() ? total : 0;
    for (auto button : buttons)
        if (button) {
            if (button->fillWidth()) button->setWidth(width);
            if (! guard) return;
            if (layoutRevision != revision()) {
                polish();
                return;
            }
            if (! button) continue;
            if (button->fillHeight()) button->setHeight(height);
            if (! guard) return;
            if (layoutRevision != revision()) {
                polish();
                return;
            }
            if (! button) continue;
            if (mirrored()) x -= button->width();
            button->setPosition(QPointF(x, (height - button->height()) / 2));
            if (! guard) return;
            if (layoutRevision != revision()) {
                polish();
                return;
            }
            if (! button) continue;
            x += mirrored() ? -spacing() : button->width() + spacing();
        }
    contentHost()->setSize(QSizeF(total, height));
    if (! guard) return;
    setImplicitContentSize(QSizeF(preferred, height));
}
void TabBar::navigate(int delta) {
    for (int i = currentIndex() + delta; i >= 0 && i < count(); i += delta) {
        auto* button = itemAt(i);
        if (button && button->isEnabled() && button->isVisible()) {
            QPointer<TabButton> alive(button);
            setCurrentIndex(i);
            if (alive) alive->forceActiveFocus(Qt::TabFocusReason);
            return;
        }
    }
}
void TabBar::incrementCurrentIndex() { navigate(1); }
void TabBar::decrementCurrentIndex() { navigate(-1); }
void TabBar::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right) {
        navigate((event->key() == Qt::Key_Right) != mirrored() ? 1 : -1);
        event->accept();
    } else
        Control::keyPressEvent(event);
}
void TabBarAttached::update(TabBar* bar, int index) {
    const bool changedBar = m_bar != bar, changedIndex = m_index != index;
    m_bar   = bar;
    m_index = index;
    QPointer<TabBarAttached> guard(this);
    if (changedBar) Q_EMIT tabBarChanged();
    if (! guard) return;
    if (changedIndex) Q_EMIT indexChanged();
    if (guard && changedBar) Q_EMIT positionChanged();
}
} // namespace qml_material
