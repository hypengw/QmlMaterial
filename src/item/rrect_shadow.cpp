#include "qml_material/item/rrect_shadow.hpp"

#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSGGeometry>
#include <QSGFlatColorMaterial>

#include "qml_material/scenegraph/geometry.h"
#include "qml_material/scenegraph/shadow_material.h"

// Shadow constants — see Skia/Flutter references:
// https://github.com/flutter/engine/blob/3.24.3/display_list/skia/dl_sk_dispatcher.cc#L295
// https://github.com/google/skia/blob/canvaskit/0.38.2/src/gpu/ganesh/SurfaceDrawContext.cpp#L1077
constexpr float     kAmbientAlpha      = 0.039f;
constexpr float     kSpotAlpha         = 0.25f;
constexpr float     kShadowLightRadius = 800.0f;
constexpr float     kShadowLightHeight = 600.0f;
constexpr QVector3D kLightPos          = { 0, -1, 1 };

namespace qml_material
{
namespace sg
{
class RRectShadowNode : public QSGGeometryNode {
public:
    RRectShadowNode() {
        setGeometry(create_shadow_geometry().release());
        setMaterial(new ShadowMaterial {});
        setFlags(QSGNode::OwnsGeometry | QSGNode::OwnsMaterial);
    }

    void init(QQuickItem* item) {
        static_cast<ShadowMaterial*>(material())->init_fadeoff_texture(item->window());
    }

    void updateGeometry() {
        if (elevation == m_last_elevation && rect == m_last_rect && color == m_last_color &&
            radius == m_last_radius)
            return;

        sg::ShadowParams params;
        {
            params.z_plane_params = QVector3D(0, 0, elevation);
            params.light_pos      = kLightPos;
            params.light_radius   = kShadowLightRadius / kShadowLightHeight;
            params.radius         = radius;
            if (elevation == 0) {
                params.flags |= sg::ShadowFlags::TransparentOccluder_ShadowFlag;
            }
            params.flags |= sg::ShadowFlags::DirectionalLight_ShadowFlag;
            auto c = this->color;
            c.setAlphaF(kAmbientAlpha * this->color.alphaF());
            params.ambient_color = c.rgba();
            c.setAlphaF(kSpotAlpha * this->color.alphaF());
            params.spot_color = c.rgba();

            for (int i = 0; i < 4; i++) {
                params.radius[i] = std::min<float>(params.radius[i], rect.height() / 2.0f);
            }
        }
        update_shadow_geometry(geometry(), params, rect);

        m_last_elevation = elevation;
        m_last_rect      = rect;
        m_last_color     = color;
        m_last_radius    = radius;
        markDirty(QSGNode::DirtyGeometry);
    }

    double    elevation;
    QRectF    rect;
    QColor    color;
    QVector4D radius;

private:
    double    m_last_elevation { -1 };
    QRectF    m_last_rect;
    QColor    m_last_color;
    QVector4D m_last_radius;
};
} // namespace sg

RRectShadow::RRectShadow(QQuickItem* parentItem)
    : QQuickItem(parentItem), m_elevation(0), m_corners(), m_radius(0), m_color(Qt::black) {
    setFlag(QQuickItem::ItemHasContents, true);
    connect(this, &RRectShadow::elevationChanged, this, &RRectShadow::update);
    connect(this, &RRectShadow::colorChanged, this, &RRectShadow::update);
    connect(this, &RRectShadow::cornersChanged, this, &RRectShadow::update);
}

RRectShadow::~RRectShadow() {}

auto RRectShadow::elevation() const -> double { return m_elevation; }
void RRectShadow::setelevation(double l) {
    if (! qFuzzyCompare(l, m_elevation)) {
        m_elevation = l;
        elevationChanged(m_elevation);
    }
}
auto RRectShadow::corners() const -> const CornersGroup& { return m_corners; }
void RRectShadow::setCorners(const CornersGroup& c) {
    m_corners = c;
    cornersChanged(c);
}

qreal RRectShadow::radius() const { return m_radius; }

void RRectShadow::setRadius(qreal newRadius) {
    if (qFuzzyCompare(m_radius, newRadius)) {
        return;
    }
    m_radius = newRadius;
    setCorners(m_radius);
    radiusChanged(m_radius);
}

QColor RRectShadow::color() const { return m_color; }

void RRectShadow::setColor(const QColor& newColor) {
    if (newColor == m_color) {
        return;
    }

    m_color = newColor;
    colorChanged(m_color);
}

void RRectShadow::componentComplete() { QQuickItem::componentComplete(); }

void RRectShadow::itemChange(QQuickItem::ItemChange            change,
                             const QQuickItem::ItemChangeData& value) {
    Q_UNUSED(value);
    Q_UNUSED(change);
    QQuickItem::itemChange(change, value);
}

QSGNode* RRectShadow::updatePaintNode(QSGNode* node, QQuickItem::UpdatePaintNodeData* data) {
    Q_UNUSED(data);

    if (boundingRect().isEmpty()) {
        delete node;
        return nullptr;
    }
    auto shadowNode = static_cast<sg::RRectShadowNode*>(node);

    if (! shadowNode) {
        shadowNode = new sg::RRectShadowNode {};
        shadowNode->init(this);
    }
    shadowNode->rect      = boundingRect();
    shadowNode->elevation = m_elevation;
    shadowNode->color     = m_color.rgb();
    {
        auto        vec = m_corners.toVector4D();
        vec[0]          = std::exchange(vec[3], vec[0]);
        const float max_r =
            std::min<float>(shadowNode->rect.width(), shadowNode->rect.height()) * 0.5f;
        for (int i = 0; i < 4; ++i) vec[i] = std::clamp(vec[i], 0.0f, max_r);
        shadowNode->radius = vec;
    }

    shadowNode->updateGeometry();

    return shadowNode;
}
} // namespace qml_material

#include "qml_material/item/moc_rrect_shadow.cpp"
