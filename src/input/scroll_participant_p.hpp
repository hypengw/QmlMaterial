#pragma once

#include "qml_material/scrollable/flickable.hpp"
#include <QtQuick/private/qquickflickable_p.h>
#include <functional>

namespace qml_material
{
// Keeps native motion state and virtual view extents behind a typed adapter.
class ScrollParticipant {
public:
    enum class Activity
    {
        Scroll,
        Drag,
        Fling
    };
    explicit ScrollParticipant(QQuickItem* item);
    QQuickItem*      item() const;
    bool             available() const;
    int              pressDelay() const;
    QPointF          position() const;
    QPointF          bounded(QPointF) const;
    Qt::Orientations axes() const;
    bool             accepts(QPointF scenePosition) const;
    qreal            deceleration() const;
    qreal            maximumVelocity() const;
    bool             synchronousDrag() const;
    void             begin(Flickable::ScrollSource source);
    QPointF          consume(QPointF delta, Activity activity, QPointF velocity,
                             const std::function<bool()>& current);
    void             release(QPointF velocity);
    void             end(bool cancelled);

private:
    QPointer<Flickable>       m_owned;
    QPointer<QQuickFlickable> m_qt;
};
} // namespace qml_material
