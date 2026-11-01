#include "qml_material/item/shimmer_effect.hpp"
#include "shimmer_client_p.hpp"
#include "../scenegraph/shimmer_material_p.hpp"

namespace qml_material
{
ShimmerEffect::ShimmerEffect(QQuickItem* parent): QQuickItem(parent) {
    setFlag(ItemHasContents);
    m_client = std::make_unique<ShimmerClient>(
        this,
        [this] {
            Q_EMIT shimmerChanged();
        },
        std::function<void()>());
    m_client->setReady(false);
    m_client->initialize();
    connect(this, &QQuickItem::windowChanged, this, &ShimmerEffect::invalidateSource);
}
ShimmerEffect::~ShimmerEffect() {
    disconnect(this, &QQuickItem::windowChanged, this, &ShimmerEffect::invalidateSource);
    for (const auto& c : m_sourceConnections) disconnect(c);
    m_client.reset();
}
Shimmer* ShimmerEffect::shimmer() const { return m_client ? m_client->shimmer() : nullptr; }
void     ShimmerEffect::setShimmer(Shimmer* value) {
    if (m_client) m_client->setShimmer(value);
}
void ShimmerEffect::invalidateSource() {
    ++m_sourceRevision;
    update();
    if (m_client) m_client->setReady(false);
}
void ShimmerEffect::setSource(QQuickItem* value) {
    if (value == this || (value && value->isAncestorOf(this)) || m_source == value) return;
    for (const auto& c : m_sourceConnections) disconnect(c);
    m_sourceConnections.clear();
    m_source = value;
    if (value) {
        m_sourceConnections.append(
            connect(value, &QQuickItem::windowChanged, this, &ShimmerEffect::invalidateSource));
        m_sourceConnections.append(connect(value, &QObject::destroyed, this, [this] {
            const QPointer<ShimmerEffect> guard(this);
            invalidateSource();
            if (guard) Q_EMIT sourceChanged();
        }));
    }
    const QPointer<ShimmerEffect> guard(this);
    invalidateSource();
    if (guard) Q_EMIT sourceChanged();
}
void ShimmerEffect::releaseResources() { invalidateSource(); }
void ShimmerEffect::geometryChange(const QRectF& now, const QRectF& old) {
    QQuickItem::geometryChange(now, old);
    const QPointer<ShimmerEffect> guard(this);
    if (m_client) m_client->refresh();
    if (guard && now.size() != old.size()) update();
}
QSGNode* ShimmerEffect::updatePaintNode(QSGNode* old, UpdatePaintNodeData*) {
    const QPointer<ShimmerEffect> guard(this);
    const auto                    revision    = m_sourceRevision;
    const auto                    reportReady = [guard, revision](bool ready) {
        if (guard)
            QMetaObject::invokeMethod(
                guard,
                [guard, revision, ready] {
                    if (! guard || ! guard->m_client || guard->m_sourceRevision != revision) return;
                    guard->m_client->setReady(ready);
                    if (guard && ready) guard->update();
                },
                Qt::QueuedConnection);
    };
    auto* provider = m_source && m_source->window() == window() && m_source->isTextureProvider()
                         ? m_source->textureProvider()
                         : nullptr;
    if (! provider || ! shimmer::validSize(size())) {
        delete old;
        reportReady(false);
        return nullptr;
    }
    auto* node = static_cast<sg::ShimmerTextureNode*>(old);
    if (node && (node->revision() != revision || node->provider() != provider)) {
        delete node;
        node = nullptr;
    }
    if (! node) node = new sg::ShimmerTextureNode(window(), provider, revision, reportReady);
    node->setData(size(), m_client->area(), m_client->frame());
    return node;
}
} // namespace qml_material
