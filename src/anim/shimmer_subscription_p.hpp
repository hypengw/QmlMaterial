#pragma once

#include "qml_material/anim/shimmer.hpp"
#include <QPointer>
#include <QQuickItem>

namespace qml_material
{
// Owned by the consumer; its item must outlive this registration.
class ShimmerSubscription {
public:
    explicit ShimmerSubscription(QQuickItem* item): m_item(item) {}
    ~ShimmerSubscription() { setGroup(nullptr); }
    ShimmerSubscription(const ShimmerSubscription&)            = delete;
    ShimmerSubscription& operator=(const ShimmerSubscription&) = delete;
    Shimmer*             group() const { return m_group; }
    void                 setGroup(Shimmer* value) {
        const auto old    = m_group;
        const auto active = m_active;
        m_group           = value;
        m_active          = false;
        if (old && active) old->setConsumerActive(m_item, false);
    }
    void setActive(bool value) {
        if (m_active == value) return;
        m_active = value;
        if (m_group) m_group->setConsumerActive(m_item, value);
    }

private:
    QQuickItem*       m_item;
    QPointer<Shimmer> m_group;
    bool              m_active = false;
};
} // namespace qml_material
