#pragma once

#include "shimmer_geometry_p.hpp"
#include "../anim/shimmer_subscription_p.hpp"
#include "qml_material/core/item_activity.hpp"
#include <QPointer>
#include <functional>

namespace qml_material
{
class ShimmerClient {
public:
    ShimmerClient(QQuickItem*, std::function<void()> groupChanged,
                  std::function<void()> supportedChanged);
    ~ShimmerClient();
    void                  initialize();
    void                  setShimmer(Shimmer*);
    Shimmer*              shimmer() const { return m_subscription.group(); }
    bool                  supported() const { return m_supported; }
    void                  setReady(bool);
    void                  refresh();
    const shimmer::Area&  area() const { return m_area; }
    const shimmer::Frame& frame() const { return m_frame; }

private:
    void                           observeWindow();
    void                           updateFrame();
    QQuickItem*                    m_item;
    ShimmerSubscription            m_subscription;
    std::unique_ptr<ItemActivity>  m_activity;
    QList<QMetaObject::Connection> m_connections, m_lifetimeConnections;
    QMetaObject::Connection        m_windowInitialized;
    std::function<void()>          m_groupChanged, m_supportedChanged;
    shimmer::Area                  m_area;
    shimmer::Frame                 m_frame;
    bool                           m_ready = true, m_supported = false, m_initialized = false;
};
} // namespace qml_material
