#include "qml_material/anim/loading_indicator_updator.hpp"
#include <algorithm>
#include <cmath>

namespace qml_material
{
namespace
{
double springValue(double t) {
    constexpr double omega0  = 14.142135623730951;
    constexpr double zeta    = .6;
    constexpr double omegaD  = 11.313708498984761;
    const double     seconds = t * .65;
    return 1 -
           std::exp(-zeta * omega0 * seconds) *
               (std::cos(omegaD * seconds) + zeta * omega0 / omegaD * std::sin(omegaD * seconds));
}
} // namespace

LoadingIndicatorUpdator::LoadingIndicatorUpdator(QObject* parent): QObject(parent) {
    const auto& sequence = MaterialShapes::loadingSequence();
    for (size_t i = 0; i < sequence.size(); ++i)
        MaterialShapes::morph(sequence[i], sequence[(i + 1) % sequence.size()], true);
}
MaterialShape::Type LoadingIndicatorUpdator::shape() const {
    return MaterialShapes::loadingSequence()[int(std::fmod(std::floor(m_progress), shapeCount()))];
}
MaterialShape::Type LoadingIndicatorUpdator::toShape() const {
    return MaterialShapes::loadingSequence()[(int(std::fmod(std::floor(m_progress), shapeCount())) +
                                              1) %
                                             shapeCount()];
}
void LoadingIndicatorUpdator::setProgress(double progress) {
    if (! std::isfinite(progress) || progress < 0 || m_progress == progress) return;
    m_progress = progress;
    updateInternal();
    emit updated();
}
void LoadingIndicatorUpdator::setColors(const QList<QColor>& colors) {
    if (m_colors == colors) return;
    m_colors = colors;
    updateInternal();
    emit colorsChanged();
    emit updated();
}
void LoadingIndicatorUpdator::updateInternal() {
    const auto base = std::floor(m_progress);
    // Finish exactly at the next shape while retaining the spring's overshoot.
    m_shapeProgress = springValue(m_progress - base) / springValue(1);
    m_rotation      = std::fmod(
        50 * std::fmod(m_progress, 36) + 90 * (std::fmod(base, 4) + m_shapeProgress), 360);
    m_color = Qt::transparent;
    if (! m_colors.isEmpty()) {
        const auto  index = int(std::fmod(base, m_colors.size()));
        const auto& a     = m_colors[index];
        const auto& b     = m_colors[(index + 1) % m_colors.size()];
        const auto  t     = std::clamp(m_shapeProgress, 0.0, 1.0);
        m_color           = QColor::fromRgbF(a.redF() * (1 - t) + b.redF() * t,
                                             a.greenF() * (1 - t) + b.greenF() * t,
                                             a.blueF() * (1 - t) + b.blueF() * t,
                                             a.alphaF() * (1 - t) + b.alphaF() * t);
    }
}
} // namespace qml_material

#include "qml_material/anim/moc_loading_indicator_updator.cpp"
