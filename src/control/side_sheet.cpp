#include "qml_material/control/side_sheet.hpp"
#include <algorithm>
#include <cmath>

namespace qml_material
{
SideSheet::SideSheet(QQuickItem* parent): Control(parent), m_sheet(new Panel(this)) {
    setClip(true);
    m_sheet->setVisible(false);
    m_sheet->setZ(1);
    connect(this, &Control::mirroredChanged, this, &SideSheet::configurationUpdated);
    connect(this, &Control::paddingChanged, this, &SideSheet::updateLayout);
    connect(this, &Control::contentItemChanged, this, &SideSheet::updateLayout);
    connect(this, &QQuickItem::enabledChanged, this, [this] {
        if (! isEnabled()) cancelDrag();
    });
}

void SideSheet::classBegin() {
    Control::classBegin();
    m_initializing = true;
}

void SideSheet::componentComplete() {
    QPointer<SideSheet> guard(this);
    Control::componentComplete();
    if (! guard) return;
    m_initializing = false;
    request(m_expanded, false);
}

Qt::Edge SideSheet::effectiveEdge() const {
    if (m_edge == Left || (m_edge == Start && ! mirrored()) || (m_edge == End && mirrored()))
        return Qt::LeftEdge;
    return Qt::RightEdge;
}

qreal SideSheet::effectiveSheetWidth() const {
    return std::min(m_sheetWidth, std::max<qreal>(0, width() - 2 * margin()));
}
qreal SideSheet::hiddenX() const {
    return effectiveEdge() == Qt::LeftEdge ? -effectiveSheetWidth() : width();
}
qreal SideSheet::expandedX() const {
    return effectiveEdge() == Qt::LeftEdge ? margin() : width() - margin() - effectiveSheetWidth();
}
qreal SideSheet::sheetX() const { return std::lerp(hiddenX(), expandedX(), m_position); }
qreal SideSheet::occupiedWidth() const {
    if (m_state == Hidden) return 0;
    return std::clamp(effectiveEdge() == Qt::LeftEdge ? sheetX() + effectiveSheetWidth()
                                                      : width() - sheetX(),
                      qreal(0),
                      std::max<qreal>(0, width()));
}
QRectF SideSheet::contentRect() const {
    auto rect = Control::contentRect();
    if (! m_coplanar) return rect;
    const auto inset = std::min(rect.width(), occupiedWidth());
    if (effectiveEdge() == Qt::LeftEdge)
        rect.setLeft(rect.left() + inset);
    else
        rect.setWidth(rect.width() - inset);
    return rect;
}

void SideSheet::geometryChange(const QRectF& next, const QRectF& previous) {
    QPointer<SideSheet> guard(this);
    Control::geometryChange(next, previous);
    if (guard) updateLayout();
}

void SideSheet::updateLayout() {
    m_layoutDirty = true;
    if (m_layoutBusy) return;
    m_layoutBusy = true;
    QPointer<SideSheet> guard(this);
    while (m_layoutDirty) {
        m_layoutDirty = false;
        m_sheet->setPosition({ sheetX(), margin() });
        if (! guard) return;
        m_sheet->setSize({ effectiveSheetWidth(), std::max<qreal>(0, height() - 2 * margin()) });
        if (! guard) return;
        m_sheet->setVisible(m_state != Hidden);
        if (! guard) return;
        layoutContentItem();
        if (! guard) return;
    }
    m_layoutBusy = false;
    emit sheetGeometryChanged();
}

void SideSheet::configurationUpdated() {
    QPointer<SideSheet> guard(this);
    cancelDrag();
    if (! guard) return;
    updateLayout();
    if (guard) emit configurationChanged();
}

void SideSheet::setEdge(Edge value) {
    if (value < Start || value > Right || value == m_edge) return;
    m_edge = value;
    configurationUpdated();
}
void SideSheet::setSheetWidth(qreal value) {
    if (! std::isfinite(value) || value < 0 || value == m_sheetWidth) return;
    m_sheetWidth = value;
    configurationUpdated();
}
void SideSheet::setDetached(bool value) {
    if (value == m_detached) return;
    m_detached = value;
    configurationUpdated();
}
void SideSheet::setCoplanar(bool value) {
    if (value == m_coplanar) return;
    m_coplanar = value;
    configurationUpdated();
}
void SideSheet::setDraggable(bool value) {
    if (value == m_draggable) return;
    m_draggable = value;
    configurationUpdated();
}
void SideSheet::setHideFriction(qreal value) {
    if (! std::isfinite(value) || value < 0 || value == m_hideFriction) return;
    m_hideFriction = value;
    emit configurationChanged();
}
void SideSheet::setPosition(qreal value) {
    if (! std::isfinite(value)) return;
    value = std::clamp(value, qreal(0), qreal(1));
    if (value == m_position) return;
    m_position = value;
    QPointer<SideSheet> guard(this);
    updateLayout();
    if (guard) emit positionChanged();
}

void SideSheet::setExpanded(bool value) {
    if (value == m_expanded && m_state != Dragging) return;
    request(value, ! m_initializing);
}
void SideSheet::request(bool value, bool animate) {
    const auto revision = ++m_revision;
    const bool changed  = m_expanded != value;
    m_expanded          = value;
    m_state             = animate ? Settling : value ? Expanded : Hidden;
    if (! animate) {
        m_lastStable = m_state;
        m_position   = value ? 1 : 0;
    }
    QPointer<SideSheet> guard(this);
    updateLayout();
    if (! guard || revision != m_revision) return;
    if (changed) emit expandedChanged();
    if (! guard || revision != m_revision) return;
    emit stateChanged();
    if (! guard || revision != m_revision) return;
    if (! animate) emit positionChanged();
    if (! guard || revision != m_revision) return;
    emit transitionRequested(revision, value ? 1 : 0, animate);
}

bool SideSheet::beginDrag() {
    if (! m_draggable || ! isEnabled() || m_state == Hidden || effectiveSheetWidth() <= 0)
        return false;
    ++m_revision;
    m_state = Dragging;
    QPointer<SideSheet> guard(this);
    emit                stateChanged();
    return guard && m_state == Dragging;
}
void SideSheet::dragBy(QPointF delta) {
    if (! dragging() || ! std::isfinite(delta.x())) return;
    const auto distance = expandedX() - hiddenX();
    if (distance != 0) setPosition(m_position + delta.x() / distance);
}
bool SideSheet::releaseOpens(QPointF velocity) const {
    const bool left = effectiveEdge() == Qt::LeftEdge;
    const auto x    = std::isfinite(velocity.x()) ? velocity.x() : 0;
    const auto y    = std::isfinite(velocity.y()) ? velocity.y() : 0;
    if (left ? x > 0 : x < 0) return true;
    const auto outer = sheetX() + (left ? 0 : effectiveSheetWidth());
    if (std::abs(outer + x * m_hideFriction) > 0.5) {
        const bool significant = std::abs(x) > std::abs(y) && std::abs(x) > 500;
        return ! significant && m_position >= 0.5;
    }
    if (x == 0 || std::abs(x) <= std::abs(y)) return m_position > 0.5;
    return false;
}
void SideSheet::releaseDrag(QPointF velocity) {
    if (dragging()) request(releaseOpens(velocity), true);
}
void SideSheet::cancelDrag() {
    if (dragging()) request(m_lastStable == Expanded, true);
}
void SideSheet::completeTransition(quint32 revision) {
    if (revision != m_revision || m_state != Settling) return;
    const auto          next = m_expanded ? Expanded : Hidden;
    QPointer<SideSheet> guard(this);
    setPosition(m_expanded ? 1 : 0);
    if (! guard || revision != m_revision) return;
    m_state = m_lastStable = next;
    updateLayout();
    if (! guard || revision != m_revision) return;
    emit stateChanged();
    if (! guard || revision != m_revision) return;
    if (next == Expanded)
        emit opened();
    else
        emit closed();
}
} // namespace qml_material
