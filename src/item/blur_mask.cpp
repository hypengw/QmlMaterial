#include "qml_material/item/blur_mask.hpp"

#include <QQuickWindow>
#include <QSGGeometry>
#include <QSGGeometryNode>

#include "qml_material/scenegraph/blur_mask_material.h"
#include "qml_material/scenegraph/geometry.h"

namespace qml_material
{
namespace sg
{
class BlurMaskNode : public QSGGeometryNode {
public:
    BlurMaskNode() {
        setGeometry(create_blur_mask_geometry().release());
        setMaterial(new BlurMaskMaterial {});
        setFlags(QSGNode::OwnsGeometry | QSGNode::OwnsMaterial);
    }

    void init(QQuickItem* item) {
        auto* mat = static_cast<BlurMaskMaterial*>(material());
        mat->init_profile_texture(item->window());
    }

    void update(QQuickItem* item) {
        auto* mat = static_cast<BlurMaskMaterial*>(material());
        mat->sigma     = static_cast<float>(sigma);
        mat->rect_size = QVector2D(rect.width(), rect.height());
        mat->radius    = radius;
        mat->style     = style;
        mat->init_corner_texture(item->window(), mat->sigma, mat->effective_radius());

        auto* verts = static_cast<BasicVertex*>(geometry()->vertexData());
        update_blur_mask_geometry(verts,
                                  QVector2D(rect.width(), rect.height()),
                                  mat->sigma,
                                  color.rgba(),
                                  radius);

        markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);
    }

    QRectF    rect;
    QColor    color;
    qreal     sigma { 0 };
    QVector4D radius { 0, 0, 0, 0 };
    BlurStyle style { BlurStyle::Normal };
};
} // namespace sg

BlurMask::BlurMask(QQuickItem* parent) : QQuickItem(parent) {
    setFlag(QQuickItem::ItemHasContents, true);
    connect(this, &BlurMask::sigmaChanged,   this, &BlurMask::update);
    connect(this, &BlurMask::radiusChanged,  this, &BlurMask::update);
    connect(this, &BlurMask::colorChanged,   this, &BlurMask::update);
    connect(this, &BlurMask::cornersChanged, this, &BlurMask::update);
    connect(this, &BlurMask::styleChanged,   this, &BlurMask::update);
}

BlurMask::~BlurMask() = default;

qreal BlurMask::sigma() const { return m_sigma; }
void  BlurMask::setSigma(qreal v) {
    if (qFuzzyCompare(m_sigma, v)) return;
    m_sigma = v;
    sigmaChanged(v);
}

qreal BlurMask::radius() const { return m_radius; }
void  BlurMask::setRadius(qreal v) {
    if (qFuzzyCompare(m_radius, v)) return;
    m_radius = v;
    setCorners(v);
    radiusChanged(v);
}

QColor BlurMask::color() const { return m_color; }
void   BlurMask::setColor(const QColor& c) {
    if (c == m_color) return;
    m_color = c;
    colorChanged(c);
}

auto BlurMask::corners() const -> const CornersGroup& { return m_corners; }
void BlurMask::setCorners(const CornersGroup& c) {
    m_corners = c;
    cornersChanged(c);
}

BlurMask::Style BlurMask::style() const { return m_style; }
void            BlurMask::setStyle(Style s) {
    if (s == m_style) return;
    m_style = s;
    styleChanged(s);
}

QSGNode* BlurMask::updatePaintNode(QSGNode* node, QQuickItem::UpdatePaintNodeData* /*data*/) {
    if (boundingRect().isEmpty()) {
        delete node;
        return nullptr;
    }
    auto* n = static_cast<sg::BlurMaskNode*>(node);
    if (!n) {
        n = new sg::BlurMaskNode {};
        n->init(this);
    }
    n->rect   = boundingRect();
    n->color  = m_color;
    n->sigma  = m_sigma;
    n->style  = static_cast<sg::BlurStyle>(m_style);
    {
        auto        vec   = m_corners.toVector4D();
        vec[0]            = std::exchange(vec[3], vec[0]);
        const float max_r = std::min<float>(n->rect.width(), n->rect.height()) * 0.5f;
        for (int i = 0; i < 4; ++i) vec[i] = std::clamp(vec[i], 0.0f, max_r);
        n->radius = vec;
    }
    n->update(this);
    return n;
}

} // namespace qml_material

#include "qml_material/item/moc_blur_mask.cpp"
