#include "qml_material/control/menu_button.hpp"
#include "qml_material/util/qt.hpp"

namespace qml_material
{
MenuButton::~MenuButton() {
    utils::disconnectAll(m_menuConnections);
    disconnect(m_anchorDestroyed);
}
QQuickItem* MenuButton::menuAnchor() const {
    return m_anchor ? m_anchor.data() : const_cast<MenuButton*>(this);
}
bool MenuButton::isExpanded() const { return m_menu && m_menu->isVisible(); }
void MenuButton::setMenu(Menu* value) {
    if (m_menu == value && (value || m_menuConnections.isEmpty())) return;
    utils::disconnectAll(m_menuConnections);
    m_menu              = value;
    const auto revision = ++m_revision;
    if (value) {
        m_menuConnections.append(
            connect(value, &Popup::visibleChanged, this, &MenuButton::expandedChanged));
        m_menuConnections.append(connect(value, &QObject::destroyed, this, [this] {
            setMenu(nullptr);
        }));
    }
    const QPointer<MenuButton> guard(this);
    cancelActivation();
    if (! guard || revision != m_revision) return;
    Q_EMIT menuChanged();
    if (guard && revision == m_revision) Q_EMIT expandedChanged();
}
void MenuButton::setMenuAnchor(QQuickItem* value) {
    if (value == this) value = nullptr;
    if (m_anchor == value) return;
    disconnect(m_anchorDestroyed);
    m_anchor            = value;
    const auto revision = ++m_revision;
    if (value)
        m_anchorDestroyed = connect(value, &QObject::destroyed, this, [this] {
            const auto                 revision = ++m_revision;
            const QPointer<MenuButton> guard(this);
            cancelActivation();
            if (guard && revision == m_revision) Q_EMIT menuAnchorChanged();
        });
    const QPointer<MenuButton> guard(this);
    cancelActivation();
    if (guard && revision == m_revision) Q_EMIT menuAnchorChanged();
}
void MenuButton::activate() {
    if (! canActivate()) return;
    const QPointer<MenuButton> guard(this);
    const auto                 target = m_menu;
    const QPointer<QQuickItem> anchor(menuAnchor());
    const auto                 revision = m_revision;
    const bool                 closing  = target && target->isVisible();
    CommandButton::activate();
    if (! guard || revision != m_revision || ! target || ! anchor || ! canActivate()) return;
    if (closing)
        target->close();
    else
        target->openFor(anchor);
}
} // namespace qml_material
