#include "qml_material/item/skeleton.hpp"
#include "shimmer_client_p.hpp"
#include "../scenegraph/shimmer_material_p.hpp"
#include <cmath>

namespace qml_material
{
Skeleton::Skeleton(QQuickItem* parent): QQuickItem(parent) {
    setFlag(ItemHasContents);
    m_client = std::make_unique<ShimmerClient>(
        this,
        [this] {
            Q_EMIT shimmerChanged();
        },
        [this] {
            Q_EMIT supportedChanged();
        });
    m_client->setReady(false);
    m_client->initialize();
}
Skeleton::~Skeleton() { m_client.reset(); }
Shimmer* Skeleton::shimmer() const { return m_client ? m_client->shimmer() : nullptr; }
void     Skeleton::setShimmer(Shimmer* value) {
    if (m_client) m_client->setShimmer(value);
}
bool Skeleton::supported() const { return m_client && m_client->supported(); }
void Skeleton::setColor(const QColor& value) {
    if (! value.isValid() || value == m_color) return;
    m_color = value;
    update();
    const QPointer<Skeleton> guard(this);
    m_client->setReady(value.alphaF() > 0);
    if (guard) Q_EMIT colorChanged();
}
void Skeleton::setRadius(qreal value) {
    if (! std::isfinite(value) || value < 0 || value == m_radius) return;
    m_radius = value;
    update();
    Q_EMIT radiusChanged();
}
void Skeleton::geometryChange(const QRectF& now, const QRectF& old) {
    QQuickItem::geometryChange(now, old);
    const QPointer<Skeleton> guard(this);
    if (m_client) m_client->refresh();
    if (guard && now.size() != old.size()) update();
}
QSGNode* Skeleton::updatePaintNode(QSGNode* old, UpdatePaintNodeData*) {
    if (! supported() || ! shimmer::validSize(size()) || m_color.alphaF() <= 0) {
        delete old;
        return nullptr;
    }
    auto* node = static_cast<sg::SkeletonNode*>(old);
    if (! node) node = new sg::SkeletonNode;
    node->setData(size(), m_color, m_radius, m_client->area(), m_client->frame());
    return node;
}
} // namespace qml_material
