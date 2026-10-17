#include "squiggly_progress.hpp"
#include "qml_material/anim/interpolator.hpp"
#include <QQuickWindow>
#include <private/qqmldata_p.h>
#include <algorithm>
#include <cmath>

namespace qml_material
{
SquigglyProgress::SquigglyProgress(QQuickItem* parent): QQuickShape(parent) {
    setPreferredRendererType(QQuickShape::CurveRenderer);
    setAsynchronous(false);
    m_activity = std::make_unique<ItemActivity>(this);
    connect(m_activity.get(), &ItemActivity::changed, this, &SquigglyProgress::schedule);
    // Shape bounds can become non-empty without an item geometry change.
    connect(this, &QQuickShape::boundingRectChanged, m_activity.get(), &ItemActivity::refresh);
    connect(this, &QQuickItem::windowChanged, this, &SquigglyProgress::bindWindow);
}
SquigglyProgress::~SquigglyProgress() {
    disconnect(m_frameConnection);
    m_activity.reset();
}
void SquigglyProgress::setProgress(qreal value) {
    if (! std::isfinite(value)) return;
    value = std::clamp(value, 0.0, 1.0);
    if (m_state.progress == value) return;
    m_state.progress = value;
    invalidate();
}
void SquigglyProgress::setAnimate(bool value) {
    if (m_animate == value) return;
    m_animate = value;
    m_from    = m_state.amplitudeFraction;
    m_elapsed = 0;
    m_clock.invalidate();
    m_heightAnimating = m_animations;
    if (! m_heightAnimating) m_state.amplitudeFraction = value ? 1 : 0;
    invalidate();
}
void SquigglyProgress::setAnimationsEnabled(bool value) {
    if (m_animations == value) return;
    m_animations = value;
    if (! value) {
        m_heightAnimating         = false;
        m_state.amplitudeFraction = m_animate ? 1 : 0;
    }
    invalidate();
}
void SquigglyProgress::setTransitionEnabled(bool value) {
    if (m_state.transitionEnabled == value) return;
    m_state.transitionEnabled = value;
    invalidate();
}
void SquigglyProgress::setColor(QColor value) {
    if (m_color == value || ! value.isValid()) return;
    m_color = value;
    invalidate();
}
#define QM_SQUIGGLY_SETTER(Name, Field, Minimum)                                            \
    void SquigglyProgress::set##Name(qreal value) {                                         \
        if (! std::isfinite(value) || value < Minimum || value > 1000000 || Field == value) \
            return;                                                                         \
        Field = value;                                                                      \
        invalidate();                                                                       \
    }
QM_SQUIGGLY_SETTER(WaveLength, m_state.waveLength, .01)
QM_SQUIGGLY_SETTER(WaveAmplitude, m_state.waveAmplitude, 0)
QM_SQUIGGLY_SETTER(WaveSpeed, m_speed, 0)
QM_SQUIGGLY_SETTER(StrokeWidth, m_stroke, 0)
#undef QM_SQUIGGLY_SETTER
void SquigglyProgress::componentComplete() {
    QPointer<SquigglyProgress> guard(this);
    QQuickShape::componentComplete();
    if (! guard || QQmlData::wasDeleted(this)) return;
    m_complete = true;
    if (! m_animations) m_state.amplitudeFraction = m_animate ? 1 : 0;
    bindWindow(window());
    if (guard && ! QQmlData::wasDeleted(this)) invalidate();
}
void SquigglyProgress::geometryChange(const QRectF& now, const QRectF& old) {
    QQuickShape::geometryChange(now, old);
    if (now.size() != old.size()) {
        m_dirty = true;
        polish();
    }
}
void SquigglyProgress::bindWindow(QQuickWindow* window) {
    disconnect(m_frameConnection);
    m_window = window;
    m_clock.invalidate();
    if (window)
        m_frameConnection = connect(window, &QQuickWindow::afterAnimating, this, [this] {
            if (! m_active || QQmlData::wasDeleted(this)) return;
            const auto elapsed = m_clock.isValid() ? m_clock.nsecsElapsed() / 1000000.0 : 0;
            m_clock.start();
            QPointer<SquigglyProgress> guard(this);
            advanceAnimations(elapsed);
            if (guard && ! QQmlData::wasDeleted(this)) polish();
        });
    schedule();
}
void SquigglyProgress::schedule() {
    if (QQmlData::wasDeleted(this)) return;
    const bool active = m_complete && m_animations && m_window && m_activity->active() &&
                        (m_heightAnimating ||
                         (m_animate && m_speed > 0 && m_state.waveAmplitude > 0 && m_stroke > 0));
    if (active) {
        if (! m_clock.isValid()) m_clock.start();
        if (m_window) m_window->update();
    } else
        m_clock.invalidate();
    if (m_active != active) {
        m_active = active;
        Q_EMIT frameChanged();
    }
}
void SquigglyProgress::invalidate() {
    if (QQmlData::wasDeleted(this)) return;
    m_dirty = true;
    polish();
    QPointer<SquigglyProgress> guard(this);
    Q_EMIT appearanceChanged();
    if (guard && ! QQmlData::wasDeleted(this)) {
        schedule();
        if (guard && ! QQmlData::wasDeleted(this)) Q_EMIT frameChanged();
    }
}
void SquigglyProgress::advanceAnimations(qreal ms) {
    if (! m_animations || ! std::isfinite(ms) || ms < 0) return;
    if (m_animate)
        m_state.phase =
            std::fmod(m_state.phase + std::fmod(ms, 1000 * waveLength() / std::max(.01, m_speed)) *
                                          m_speed / 1000,
                      waveLength());
    if (m_heightAnimating) {
        m_elapsed += ms;
        const auto delay    = m_animate ? 60.0 : 0.0;
        const auto duration = m_animate ? 800.0 : 550.0;
        const auto fraction = std::clamp((m_elapsed - delay) / duration, 0.0, 1.0);
        auto       easing   = anim::standard_decelerate();
        if (m_animate) {
            easing = QEasingCurve(QEasingCurve::BezierSpline);
            easing.addCubicBezierSegment({ .05, .7 }, { .1, 1 }, { 1, 1 });
        }
        m_state.amplitudeFraction =
            std::lerp(m_from, m_animate ? 1.0 : 0.0, easing.valueForProgress(fraction));
        m_heightAnimating = fraction < 1;
    }
    m_dirty = true;
    QPointer<SquigglyProgress> guard(this);
    schedule();
    if (guard && ! QQmlData::wasDeleted(this)) Q_EMIT frameChanged();
}
void SquigglyProgress::updatePaths() {
    QPointer<SquigglyProgress> guard(this);
    const auto geometry = SquigglyProgressGeometry::render(m_state, { width(), height() });
    while (m_paths.size() < 3) {
        auto* path   = new QQuickShapePath(this);
        auto* status = static_cast<QQmlParserStatus*>(path);
        status->classBegin();
        path->setCapStyle(QQuickShapePath::RoundCap);
        status->componentComplete();
        auto list = data();
        list.append(&list, path);
        if (! guard) return;
        m_paths.append(path);
    }
    auto inactive = m_color;
    inactive.setAlpha(m_color.alpha() * 77 / 255);
    const QPainterPath paths[] = { geometry.active, geometry.inactive };
    for (int i = 0; i < 2; ++i) {
        m_paths[i]->setPath(paths[i]);
        if (! guard) return;
        m_paths[i]->setCapStyle(i == 0 || m_state.transitionEnabled ? QQuickShapePath::FlatCap
                                                                    : QQuickShapePath::RoundCap);
        if (! guard) return;
        m_paths[i]->setFillColor(Qt::transparent);
        if (! guard) return;
        m_paths[i]->setStrokeWidth(m_stroke);
        if (! guard) return;
        m_paths[i]->setStrokeColor(i ? inactive : m_color);
        if (! guard) return;
    }
    QPainterPath cap;
    if (geometry.valid && m_stroke > 0)
        cap.addEllipse(geometry.startCap, m_stroke / 2, m_stroke / 2);
    m_paths[2]->setPath(cap);
    if (! guard) return;
    m_paths[2]->setStrokeWidth(0);
    if (! guard) return;
    m_paths[2]->setFillColor(m_color);
}
void SquigglyProgress::updatePolish() {
    if (QQmlData::wasDeleted(this)) return;
    QPointer<SquigglyProgress> guard(this);
    if (m_dirty) {
        m_dirty = false;
        updatePaths();
        if (! guard || QQmlData::wasDeleted(this)) return;
    }
    QQuickShape::updatePolish();
}
} // namespace qml_material
