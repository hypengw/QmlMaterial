#include "qml_material/control/page_motion.hpp"
#include "qml_material/token/token.hpp"
#include <cmath>
namespace qml_material
{
PageMotionContext::PageMotionContext(QQuickItem* from, QQuickItem* to, int op, QObject* parent)
    : QObject(parent), m_source(from), m_target(to), m_operation(op) {
    for (auto* item : { from, to })
        if (item) {
            connect(
                item, &QQuickItem::implicitWidthChanged, this, &PageMotionContext::geometryChanged);
            connect(item,
                    &QQuickItem::implicitHeightChanged,
                    this,
                    &PageMotionContext::geometryChanged);
        }
}
QSizeF PageMotionContext::sourceSize() const {
    return m_source ? QSizeF(m_source->implicitWidth(), m_source->implicitHeight()) : QSizeF(0, 0);
}
QSizeF PageMotionContext::targetSize() const {
    return m_target ? QSizeF(m_target->implicitWidth(), m_target->implicitHeight()) : QSizeF(0, 0);
}
PageMotion::PageMotion(QObject* p): QObject(p) {
    m_enterOpacityTrack.setValues({ { "from", 0 }, { "to", 1 } });
    m_exitOpacityTrack.setValues({ { "from", 1 }, { "to", 0 } });
    m_enterScaleTrack.setValues({ { "from", .92 }, { "to", 1 } });
    m_exitScaleTrack.setValues({ { "from", 1 }, { "to", .92 } });
    for (auto* track :
         { &m_enterOpacityTrack, &m_exitOpacityTrack, &m_enterScaleTrack, &m_exitScaleTrack })
        m_default.addTrack(track);
    setSpec(Motion::tween(450, token::Easing {}.emphasized()));
    m_explicitSpec = false;
    setTimeline(&m_default);
}
PageMotion::~PageMotion() {
    disconnect(m_frame);
    disconnect(m_destroyed);
    for (auto* track :
         { &m_enterOpacityTrack, &m_exitOpacityTrack, &m_enterScaleTrack, &m_exitScaleTrack })
        disconnect(track, nullptr, &m_default, nullptr);
}
void PageMotion::setContext(PageMotionContext* c) {
    if (m_context == c) return;
    m_context = c;
    emit contextChanged();
}
void PageMotion::setTimeline(SeekableTransition* t) {
    if (m_timeline == t) return;
    disconnect(m_frame);
    disconnect(m_destroyed);
    const QPointer<PageMotion>         guard(this);
    const QPointer<SeekableTransition> candidate(t), previous(m_timeline);
    m_timeline = t;
    if (previous) previous->stop();
    if (! guard || m_timeline != candidate) return;
    t = candidate;
    if (t) {
        m_frame     = connect(t, &SeekableTransition::changed, this, &PageMotion::frameChanged);
        m_destroyed = connect(t, &QObject::destroyed, this, [this] {
            m_timeline = nullptr;
            emit timelineChanged();
        });
    }
    emit timelineChanged();
}
void PageMotion::setSpec(AnimationSpec s) {
    if (! s.valid()) return;
    m_explicitSpec = true;
    m_spec         = s;
    for (auto* track :
         { &m_enterOpacityTrack, &m_exitOpacityTrack, &m_enterScaleTrack, &m_exitScaleTrack })
        track->setSpec(s);
    emit specChanged();
}
void PageMotion::setSettings(MotionSettings* s) {
    if (! m_explicitSpec && s) {
        setSpec(s->page());
        m_explicitSpec = false;
    }
    if (m_timeline) m_timeline->setSettings(s);
}
qreal PageMotion::enterOpacity() const {
    return m_enterOpacity.value_or(m_timeline == &m_default ? m_enterOpacityTrack.value() : 1);
}
qreal PageMotion::exitOpacity() const {
    return m_exitOpacity.value_or(m_timeline == &m_default ? m_exitOpacityTrack.value() : 1);
}
qreal PageMotion::enterScale() const {
    return m_enterScale.value_or(m_timeline == &m_default ? m_enterScaleTrack.value() : 1);
}
qreal PageMotion::exitScale() const {
    return m_exitScale.value_or(m_timeline == &m_default ? m_exitScaleTrack.value() : 1);
}
#define QM_PAGE_SCALAR(Name, Field)                   \
    void PageMotion::set##Name(qreal v) {             \
        if (! std::isfinite(v) || Field == v) return; \
        Field = v;                                    \
        emit frameChanged();                          \
    }
QM_PAGE_SCALAR(EnterOpacity, m_enterOpacity)
QM_PAGE_SCALAR(ExitOpacity, m_exitOpacity)
QM_PAGE_SCALAR(EnterScale, m_enterScale)
QM_PAGE_SCALAR(ExitScale, m_exitScale)
#undef QM_PAGE_SCALAR
void PageMotion::setEnterOffset(QPointF v) {
    if (! std::isfinite(v.x()) || ! std::isfinite(v.y()) || m_enterOffset == v) return;
    m_enterOffset = v;
    emit frameChanged();
}
void PageMotion::setExitOffset(QPointF v) {
    if (! std::isfinite(v.x()) || ! std::isfinite(v.y()) || m_exitOffset == v) return;
    m_exitOffset = v;
    emit frameChanged();
}
} // namespace qml_material
