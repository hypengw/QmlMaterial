#include "qml_material/control/progress_bar.hpp"
#include <algorithm>
#include <cmath>
#include <QtCore/QPointer>

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
    m_from                         = value;
    const auto            revision = ++m_revision;
    QPointer<ProgressBar> guard(this);
    Q_EMIT fromChanged();
    if (! guard || m_revision != revision) return;
    Q_EMIT positionChanged();
    if (! guard || m_revision != revision) return;
    Q_EMIT visualPositionChanged();
    if (! guard || m_revision != revision) return;
    if (isComponentComplete()) setValue(m_value);
}

void ProgressBar::setTo(qreal value) {
    if (! std::isfinite(value) || qFuzzyCompare(m_to, value)) return;
    m_to                           = value;
    const auto            revision = ++m_revision;
    QPointer<ProgressBar> guard(this);
    Q_EMIT toChanged();
    if (! guard || m_revision != revision) return;
    Q_EMIT positionChanged();
    if (! guard || m_revision != revision) return;
    Q_EMIT visualPositionChanged();
    if (! guard || m_revision != revision) return;
    if (isComponentComplete()) setValue(m_value);
}

void ProgressBar::setValue(qreal value) {
    if (! std::isfinite(value)) return;
    // QML may initialize value before the range endpoints.
    if (isComponentComplete())
        value = std::clamp(value, std::min(m_from, m_to), std::max(m_from, m_to));
    if (qFuzzyCompare(m_value, value)) return;
    m_value                        = value;
    const auto            revision = ++m_revision;
    QPointer<ProgressBar> guard(this);
    Q_EMIT valueChanged();
    if (! guard || m_revision != revision) return;
    Q_EMIT positionChanged();
    if (! guard || m_revision != revision) return;
    Q_EMIT visualPositionChanged();
}

void ProgressBar::setIndeterminate(bool value) {
    if (m_indeterminate == value) return;
    m_indeterminate = value;
    ++m_revision;
    Q_EMIT indeterminateChanged();
}

void ProgressBar::componentComplete() {
    QPointer<ProgressBar> guard(this);
    Control::componentComplete();
    if (! guard) return;
    setValue(m_value);
}

} // namespace qml_material
