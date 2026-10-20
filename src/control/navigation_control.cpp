#include "qml_material/control/navigation_control.hpp"
#include "qml_material/util/qt.hpp"

namespace qml_material
{
void NavigationControl::requestSelection(NavigationItem* item) {
    if (! item || item->navigation() != this || item->navigationIndex() < 0 || ! isEnabled() ||
        ! isVisible() || ! item->isEnabled() || ! item->isVisible())
        return;
    Q_EMIT currentIndexChangeRequested(item->navigationIndex());
}

NavigationItem::NavigationItem(QQuickItem* parent): SelectionButton(parent) {
    connect(this, &SelectionButton::selectionRequested, this, [this] {
        if (auto* owner = navigation()) owner->requestSelection(this);
    });
}
NavigationItem::~NavigationItem() { utils::disconnectAll(m_navigationConnections); }
bool NavigationItem::canActivate() const {
    return SelectionButton::canActivate() &&
           (! m_navigation || (m_navigation->isEnabled() && m_navigation->isVisible()));
}
void NavigationItem::setNavigation(NavigationControl* value) {
    if (m_navigation == value) return;
    utils::disconnectAll(m_navigationConnections);
    m_navigation = value;
    if (value) {
        const auto availabilityChanged = [this] {
            if (! canActivate()) cancelInput();
        };
        m_navigationConnections.append(
            connect(value, &QQuickItem::enabledChanged, this, availabilityChanged));
        m_navigationConnections.append(
            connect(value, &QQuickItem::visibleChanged, this, availabilityChanged));
        m_navigationConnections.append(connect(value, &QObject::destroyed, this, [this] {
            utils::disconnectAll(m_navigationConnections);
            m_navigation = nullptr;
            const QPointer<NavigationItem> guard(this);
            cancelActivation();
            if (guard) Q_EMIT navigationChanged();
        }));
    }
    const QPointer<NavigationItem> guard(this);
    cancelActivation();
    if (guard) Q_EMIT navigationChanged();
}
void NavigationItem::setNavigationIndex(int value) {
    if (m_navigationIndex == value) return;
    m_navigationIndex = value;
    const QPointer<NavigationItem> guard(this);
    cancelActivation();
    if (guard) Q_EMIT navigationIndexChanged();
}
} // namespace qml_material
