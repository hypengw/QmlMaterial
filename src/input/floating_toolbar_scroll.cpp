#include "qml_material/input/floating_toolbar_scroll.hpp"
#include <cmath>

namespace qml_material
{
FloatingToolbarScroll::FloatingToolbarScroll(QObject* parent): QObject(parent) {}
void FloatingToolbarScroll::reset() {
    m_remaining = m_expanded ? m_collapseThreshold : -m_expandThreshold;
}
void FloatingToolbarScroll::setExpanded(bool value) {
    if (m_expanded == value) return;
    m_expanded = value;
    reset();
    emit expandedChanged();
}
void FloatingToolbarScroll::setEnabled(bool value) {
    if (m_enabled == value) return;
    m_enabled = value;
    reset();
    emit enabledChanged();
}
void FloatingToolbarScroll::setReverseLayout(bool value) {
    if (m_reverseLayout == value) return;
    m_reverseLayout = value;
    reset();
    emit reverseLayoutChanged();
}
void FloatingToolbarScroll::setExpandScrollThreshold(qreal value) {
    if (! std::isfinite(value) || value < 0 || m_expandThreshold == value) return;
    m_expandThreshold = value;
    reset();
    emit expandScrollThresholdChanged();
}
void FloatingToolbarScroll::setCollapseScrollThreshold(qreal value) {
    if (! std::isfinite(value) || value < 0 || m_collapseThreshold == value) return;
    m_collapseThreshold = value;
    reset();
    emit collapseScrollThresholdChanged();
}
void FloatingToolbarScroll::scrollBy(QPointF consumed) {
    if (! m_enabled || ! std::isfinite(consumed.y()) || consumed.y() == 0) return;
    const auto delta = m_reverseLayout ? -consumed.y() : consumed.y();
    m_remaining -= delta;
    if (delta > 0 && m_remaining <= 0) {
        m_remaining = -m_expandThreshold;
        if (m_expanded) emit collapseRequested();
    } else if (delta < 0 && m_remaining >= 0) {
        m_remaining = m_collapseThreshold;
        if (! m_expanded) emit expandRequested();
    }
}
} // namespace qml_material
