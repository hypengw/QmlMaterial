#pragma once

#include "qml_material/scrollable/flickable.hpp"
#include <QtQuick/private/qquickflickable_p.h>
#include <type_traits>

namespace qml_material
{

class ScrollViewport {
public:
    explicit ScrollViewport(QObject* object)
        : m_owned(qobject_cast<Flickable*>(object)), m_qt(qobject_cast<QQuickFlickable*>(object)) {}

    QQuickItem* item() const {
        return m_owned ? static_cast<QQuickItem*>(m_owned.data()) : m_qt.data();
    }
    QObject* visibleArea() const {
        if (m_owned) return m_owned->visibleArea();
        return m_qt ? m_qt->property("visibleArea").value<QObject*>() : nullptr;
    }
    bool moving(bool horizontal) const {
        if (m_owned)
            return horizontal ? m_owned->isMovingHorizontally() : m_owned->isMovingVertically();
        return m_qt && (horizontal ? m_qt->isMovingHorizontally() : m_qt->isMovingVertically());
    }
    qreal offset(bool horizontal) const {
        if (m_owned) return horizontal ? m_owned->contentX() : m_owned->contentY();
        return m_qt ? (horizontal ? m_qt->contentX() : m_qt->contentY()) : 0;
    }
    bool pixelAligned() const {
        return m_owned ? m_owned->pixelAligned() : m_qt && m_qt->pixelAligned();
    }
    void setOffset(bool horizontal, qreal position) {
        if (m_owned) {
            if (horizontal)
                m_owned->setContentX(position);
            else
                m_owned->setContentY(position);
        } else if (m_qt) {
            if (horizontal)
                m_qt->setContentX(position);
            else
                m_qt->setContentY(position);
        }
    }
    template<typename F>
    void observeMoving(QObject* receiver, F callback) {
        auto connectOwner = [receiver, callback](auto* owner) {
            using Owner = std::remove_pointer_t<decltype(owner)>;
            QObject::connect(owner, &Owner::movingHorizontallyChanged, receiver, [callback] {
                callback(true);
            });
            QObject::connect(owner, &Owner::movingVerticallyChanged, receiver, [callback] {
                callback(false);
            });
        };
        if (m_owned)
            connectOwner(m_owned.data());
        else if (m_qt)
            connectOwner(m_qt.data());
    }

private:
    QPointer<Flickable>       m_owned;
    QPointer<QQuickFlickable> m_qt;
};

} // namespace qml_material
