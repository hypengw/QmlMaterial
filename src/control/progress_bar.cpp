#include "qml_material/control/progress_bar.hpp"
#include <algorithm>
#include <cmath>

namespace qml_material
{

ProgressBar::ProgressBar(QQuickItem* parent): Control(parent) {
    connect(this, &Control::mirroredChanged, this, &ProgressBar::visualPositionChanged);
}

qreal ProgressBar::position() const {
    return qFuzzyCompare(m_from, m_to) ? 0 : (m_value - m_from) / (m_to - m_from);
}

qreal ProgressBar::visualPosition() const { return mirrored() ? 1 - position() : position(); }

void ProgressBar::setFrom(qreal value) {
    if (! std::isfinite(value) || qFuzzyCompare(m_from, value)) return;
    m_from = value;
    Q_EMIT fromChanged();
    Q_EMIT positionChanged();
    Q_EMIT visualPositionChanged();
    if (isComponentComplete()) setValue(m_value);
}

void ProgressBar::setTo(qreal value) {
    if (! std::isfinite(value) || qFuzzyCompare(m_to, value)) return;
    m_to = value;
    Q_EMIT toChanged();
    Q_EMIT positionChanged();
    Q_EMIT visualPositionChanged();
    if (isComponentComplete()) setValue(m_value);
}

void ProgressBar::setValue(qreal value) {
    if (! std::isfinite(value)) return;
    // QML may initialize value before the range endpoints.
    if (isComponentComplete())
        value = std::clamp(value, std::min(m_from, m_to), std::max(m_from, m_to));
    if (qFuzzyCompare(m_value, value)) return;
    m_value = value;
    Q_EMIT valueChanged();
    Q_EMIT positionChanged();
    Q_EMIT visualPositionChanged();
}

void ProgressBar::setIndeterminate(bool value) {
    if (m_indeterminate == value) return;
    m_indeterminate = value;
    Q_EMIT indeterminateChanged();
}

void ProgressBar::componentComplete() {
    Control::componentComplete();
    setValue(m_value);
}

} // namespace qml_material
