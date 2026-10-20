#include "qml_material/control/standard_drawer.hpp"
#include "qml_material/util/qt.hpp"

namespace qml_material
{
StandardDrawer::StandardDrawer(QQuickItem* parent)
    : NavigationControl(parent), m_defaultHeaderAction(new Action(this)) {
    connect(m_defaultHeaderAction, &Action::triggered, this, [this] {
        if (isEnabled() && isVisible()) toggle();
    });
    connect(this, &QQuickItem::enabledChanged, this, &StandardDrawer::invalidateHeader);
    connect(this, &QQuickItem::visibleChanged, this, &StandardDrawer::invalidateHeader);
}
StandardDrawer::~StandardDrawer() {
    utils::disconnectAll(m_drawerConnections);
    utils::disconnectAll(m_headerConnections);
}
void StandardDrawer::invalidateHeader() {
    if (m_header) m_header->cancelActivation();
}
void StandardDrawer::setUseEmbed(bool value) {
    if (m_useEmbed == value) return;
    m_useEmbed                = value;
    const bool notifyExpanded = m_expanded != value;
    m_expanded                = value;
    const QPointer<StandardDrawer> guard(this);
    const auto                     revision = ++m_revision;
    invalidateHeader();
    if (! guard) return;
    Q_EMIT useEmbedChanged();
    if (guard && revision == m_revision && notifyExpanded) Q_EMIT expandedChanged();
}
void StandardDrawer::setExpanded(bool value) {
    if (m_expanded == value) return;
    m_expanded = value;
    const QPointer<StandardDrawer> guard(this);
    const auto                     revision = ++m_revision;
    invalidateHeader();
    if (guard && revision == m_revision) Q_EMIT expandedChanged();
}
void StandardDrawer::setDrawer(Drawer* value) {
    if (m_drawer == value && (value || m_drawerConnections.isEmpty())) return;
    utils::disconnectAll(m_drawerConnections);
    m_drawer = value;
    if (value) {
        m_drawerConnections.append(
            connect(value, &Popup::aboutToShow, this, &StandardDrawer::invalidateHeader));
        m_drawerConnections.append(
            connect(value, &Popup::aboutToHide, this, &StandardDrawer::invalidateHeader));
        m_drawerConnections.append(connect(value, &QObject::destroyed, this, [this] {
            setDrawer(nullptr);
        }));
    }
    const QPointer<StandardDrawer> guard(this);
    invalidateHeader();
    if (guard) Q_EMIT drawerChanged();
}
void StandardDrawer::setHeaderControl(CommandButton* value) {
    if (m_header == value && (value || m_headerConnections.isEmpty())) return;
    const auto old = m_header;
    utils::disconnectAll(m_headerConnections);
    m_header = value;
    if (value) {
        m_headerConnections.append(connect(value, &QObject::destroyed, this, [this] {
            setHeaderControl(nullptr);
        }));
    }
    const QPointer<StandardDrawer> guard(this);
    if (old) old->cancelActivation();
    if (! guard) return;
    invalidateHeader();
    if (guard) Q_EMIT headerControlChanged();
}
void StandardDrawer::open() {
    if (m_useEmbed)
        setExpanded(true);
    else if (m_drawer)
        m_drawer->open();
}
void StandardDrawer::close() {
    if (m_useEmbed)
        setExpanded(false);
    else if (m_drawer)
        m_drawer->close();
}
void StandardDrawer::toggle() {
    if (m_useEmbed)
        setExpanded(! m_expanded);
    else if (m_drawer) {
        if (m_drawer->isVisible() && ! m_drawer->closing())
            m_drawer->close();
        else
            m_drawer->open();
    }
}
} // namespace qml_material
