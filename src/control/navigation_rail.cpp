#include "qml_material/control/navigation_rail.hpp"
#include "qml_material/util/qt.hpp"

namespace qml_material
{
NavigationRail::NavigationRail(QQuickItem* parent)
    : NavigationControl(parent), m_action(new Action(this)) {
    connect(m_action, &Action::triggered, this, [this] {
        if (isEnabled() && isVisible()) toggle();
    });
    const auto unavailable = [this] {
        const QPointer<NavigationRail> guard(this);
        cancelHeader();
        if (guard && (! isEnabled() || ! isVisible())) close();
    };
    connect(this, &QQuickItem::enabledChanged, this, unavailable);
    connect(this, &QQuickItem::visibleChanged, this, unavailable);
}
NavigationRail::~NavigationRail() {
    utils::disconnectAll(m_drawerConnections);
    utils::disconnectAll(m_headerConnections);
    utils::disconnectAll(m_siteConnections);
}
void NavigationRail::cancelHeader() {
    if (m_header) m_header->cancelActivation();
}
void NavigationRail::setRequested(bool value) {
    if (m_requested == value) return;
    m_requested = value;
    const QPointer<NavigationRail> guard(this);
    const auto                     revision = ++m_revision;
    cancelHeader();
    if (guard && revision == m_revision) Q_EMIT drawerRequestedChanged();
}
void NavigationRail::setUseEmbed(bool value) {
    if (m_embed == value) return;
    const bool modal = useModal();
    m_embed          = value;
    const QPointer<NavigationRail> guard(this);
    cancelHeader();
    if (! guard) return;
    Q_EMIT useEmbedChanged();
    if (! guard) return;
    if (modal != useModal()) {
        close();
        if (! guard) return;
        Q_EMIT useModalChanged();
    }
    if (guard && m_autoExpand) setExpanded(m_embed);
}
void NavigationRail::setForceModal(bool value) {
    if (m_forceModal == value) return;
    const bool modal = useModal();
    m_forceModal     = value;
    const QPointer<NavigationRail> guard(this);
    cancelHeader();
    if (! guard) return;
    Q_EMIT forceModalChanged();
    if (guard && modal != useModal()) {
        if (m_drawer) m_drawer->close();
        if (guard) setRequested(false);
        if (guard) Q_EMIT useModalChanged();
    }
}
void NavigationRail::setExpanded(bool value) {
    if (m_expanded == value) return;
    m_expanded = value;
    const QPointer<NavigationRail> guard(this);
    cancelHeader();
    if (guard) Q_EMIT expandedChanged();
}
void NavigationRail::setAutoExpand(bool value) {
    if (m_autoExpand == value) return;
    m_autoExpand = value;
    const QPointer<NavigationRail> guard(this);
    Q_EMIT autoExpandChanged();
    if (guard && m_autoExpand) setExpanded(m_embed);
}
void NavigationRail::setDrawer(Drawer* value) {
    if (m_drawer == value && (value || m_drawerConnections.isEmpty())) return;
    const auto old = m_drawer;
    utils::disconnectAll(m_drawerConnections);
    m_drawer = value;
    if (value) {
        m_drawerConnections.append(connect(value, &Popup::aboutToShow, this, [this] {
            setRequested(true);
        }));
        m_drawerConnections.append(connect(value, &Popup::aboutToHide, this, [this] {
            setRequested(false);
        }));
        m_drawerConnections.append(connect(value, &QObject::destroyed, this, [this] {
            setDrawer(nullptr);
        }));
    }
    const QPointer<NavigationRail> guard(this);
    cancelHeader();
    if (old) old->close();
    if (! guard || m_drawer != value) return;
    setRequested(value && value->isVisible() && ! value->closing());
    if (guard && m_drawer == value) Q_EMIT drawerChanged();
}
void NavigationRail::setHeaderControl(CommandButton* value) {
    if (m_header == value && (value || m_headerConnections.isEmpty())) return;
    const auto old = m_header;
    utils::disconnectAll(m_headerConnections);
    m_header = value;
    if (value)
        m_headerConnections.append(connect(value, &QObject::destroyed, this, [this] {
            setHeaderControl(nullptr);
        }));
    const QPointer<NavigationRail> guard(this);
    if (old) old->cancelActivation();
    if (! guard || m_header != value) return;
    cancelHeader();
    if (guard && m_header == value) Q_EMIT headerControlChanged();
}
void NavigationRail::setPopupSite(PresentationSite* value) {
    if (m_site == value && (value || m_siteConnections.isEmpty())) return;
    utils::disconnectAll(m_siteConnections);
    m_site = value;
    if (value) {
        m_siteConnections.append(
            connect(value, &PresentationSite::activationRequested, this, &NavigationRail::open));
        m_siteConnections.append(connect(value, &QObject::destroyed, this, [this] {
            setPopupSite(nullptr);
        }));
    }
    const QPointer<NavigationRail> guard(this);
    cancelHeader();
    if (guard) Q_EMIT popupSiteChanged();
}
void NavigationRail::open() {
    if (! isEnabled() || ! isVisible()) return;
    if (! useModal()) {
        setExpanded(true);
        return;
    }
    const QPointer<NavigationRail> guard(this);
    const auto                     target = m_drawer;
    if (! target) return;
    setRequested(true);
    if (guard && m_drawer == target && useModal() && m_requested) target->open();
}
void NavigationRail::close() {
    const QPointer<NavigationRail> guard(this);
    if (m_drawer) m_drawer->close();
    if (! guard) return;
    setRequested(false);
    if (guard && ! useModal()) setExpanded(false);
}
void NavigationRail::toggle() {
    if (useModal() ? m_requested : m_expanded)
        close();
    else
        open();
}
void NavigationRail::componentComplete() {
    const QPointer<NavigationRail> guard(this);
    NavigationControl::componentComplete();
    if (guard && m_autoExpand) setExpanded(m_embed);
}
} // namespace qml_material
