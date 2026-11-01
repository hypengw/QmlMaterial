#include "shimmer_client_p.hpp"
#include <QSGRendererInterface>
#include <private/qqmldata_p.h>

namespace qml_material
{
ShimmerClient::ShimmerClient(QQuickItem* item, std::function<void()> groupChanged,
                             std::function<void()> supportedChanged)
    : m_item(item),
      m_subscription(item),
      m_activity(std::make_unique<ItemActivity>(item)),
      m_groupChanged(std::move(groupChanged)),
      m_supportedChanged(std::move(supportedChanged)) {
    m_lifetimeConnections.append(
        QObject::connect(m_activity.get(), &ItemActivity::refreshed, item, [this] {
            refresh();
        }));
    m_lifetimeConnections.append(QObject::connect(item, &QQuickItem::windowChanged, item, [this] {
        observeWindow();
    }));
}
ShimmerClient::~ShimmerClient() {
    for (const auto& c : m_connections) QObject::disconnect(c);
    for (const auto& c : m_lifetimeConnections) QObject::disconnect(c);
    QObject::disconnect(m_windowInitialized);
}
void ShimmerClient::initialize() {
    m_initialized = true;
    observeWindow();
}
void ShimmerClient::observeWindow() {
    QObject::disconnect(m_windowInitialized);
    if (m_item->window())
        m_windowInitialized = QObject::connect(
            m_item->window(),
            &QQuickWindow::sceneGraphInitialized,
            m_item,
            [this] {
                refresh();
            },
            Qt::QueuedConnection);
    refresh();
}
void ShimmerClient::setReady(bool value) {
    if (m_ready == value) return;
    m_ready = value;
    refresh();
}
void ShimmerClient::setShimmer(Shimmer* value) {
    if (shimmer() == value && (value || m_connections.isEmpty())) return;
    const QPointer<QQuickItem> guard(m_item);
    const QPointer<Shimmer>    target(value);
    for (const auto& c : m_connections) QObject::disconnect(c);
    m_connections.clear();
    m_subscription.setGroup(value);
    if (! guard || shimmer() != target) return;
    if (target) {
        m_connections.append(
            QObject::connect(target, &Shimmer::configurationChanged, m_item, [this] {
                refresh();
            }));
        m_connections.append(QObject::connect(target, &Shimmer::phaseChanged, m_item, [this] {
            updateFrame();
        }));
        m_connections.append(QObject::connect(target, &QObject::destroyed, m_item, [this] {
            setShimmer(nullptr);
        }));
    }
    const auto notify = m_groupChanged;
    refresh();
    if (guard && notify) notify();
}
void ShimmerClient::updateFrame() {
    shimmer::Frame frame;
    if (auto* group = shimmer(); group && group->animationsEnabled() && m_area.valid && m_supported)
        frame = { float(group->phase()),
                  float(group->minimumOpacity()),
                  float(group->maximumOpacity()) };
    if (m_frame == frame) return;
    m_frame = frame;
    m_item->update();
}
void ShimmerClient::refresh() {
    if (! m_initialized || QQmlData::wasDeleted(m_item)) return;
    const auto api = m_item->window() ? m_item->window()->rendererInterface()->graphicsApi()
                                      : QQuickWindow::graphicsApi();
    const bool supported =
        api != QSGRendererInterface::Software && api != QSGRendererInterface::Unknown;
    const bool supportChanged  = supported != m_supported;
    const auto area            = shimmer::areaFor(m_item, shimmer());
    const bool geometryChanged = area != m_area;
    m_supported                = supported;
    m_area                     = area;
    updateFrame();
    if (geometryChanged || supportChanged) m_item->update();
    const bool active = shimmer() && m_ready && m_supported && m_area.valid && m_activity->active();
    const QPointer<QQuickItem> guard(m_item);
    const auto                 notify = m_supportedChanged;
    m_subscription.setActive(active);
    if (guard && supportChanged && notify) notify();
}
} // namespace qml_material
