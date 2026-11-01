#include "shimmer_material_p.hpp"
#include <QSGImageNode>
#include <QSGMaterialShader>
#include <QSGTexture>
#include <algorithm>
#include <cstring>

namespace qml_material::sg
{
namespace
{
class ShimmerShader : public QSGMaterialShader {
public:
    explicit ShimmerShader(bool textured) {
        setShaderFileName(
            VertexStage,
            textured ? QStringLiteral(":/Qcm/Material/assets/shader/shimmer_texture.vert.qsb")
                     : QStringLiteral(":/Qcm/Material/assets/shader/skeleton.vert.qsb"));
        setShaderFileName(
            FragmentStage,
            textured ? QStringLiteral(":/Qcm/Material/assets/shader/shimmer_texture.frag.qsb")
                     : QStringLiteral(":/Qcm/Material/assets/shader/skeleton.frag.qsb"));
    }
    bool updateUniformData(RenderState& state, QSGMaterial* next, QSGMaterial* previous) override {
        auto* material = static_cast<ShimmerMaterial*>(next);
        auto* old      = static_cast<ShimmerMaterial*>(previous);
        auto* buffer   = state.uniformData();
        Q_ASSERT(buffer->size() >= 80);
        bool changed = false;
        if (state.isMatrixDirty()) {
            const auto matrix = state.combinedMatrix();
            std::memcpy(buffer->data(), matrix.constData(), 64);
            changed = true;
        }
        if (state.isOpacityDirty()) {
            const float opacity = state.opacity();
            std::memcpy(buffer->data() + 64, &opacity, 4);
            changed = true;
        }
        if (! old || old == material || old->frame != material->frame) {
            const float values[] = { material->frame.phase,
                                     material->frame.minimum,
                                     material->frame.maximum };
            std::memcpy(buffer->data() + 68, values, sizeof(values));
            changed = true;
        }
        return changed;
    }
    void updateSampledImage(RenderState&, int binding, QSGTexture** texture, QSGMaterial* next,
                            QSGMaterial*) override {
        if (binding == 1) *texture = static_cast<ShimmerMaterial*>(next)->texture;
    }
};
struct SkeletonVertex {
    float x, y;
    float localX, localY, halfWidth, halfHeight;
    float r, g, b, a;
    float projection, travel, radius;
};
const QSGGeometry::AttributeSet& skeletonAttributes() {
    static const QSGGeometry::Attribute attributes[] = {
        QSGGeometry::Attribute::create(0, 2, QSGGeometry::FloatType, true),
        QSGGeometry::Attribute::create(1, 4, QSGGeometry::FloatType),
        QSGGeometry::Attribute::create(2, 4, QSGGeometry::FloatType),
        QSGGeometry::Attribute::create(3, 3, QSGGeometry::FloatType)
    };
    static const QSGGeometry::AttributeSet set = { 4, sizeof(SkeletonVertex), attributes };
    return set;
}
struct TextureVertex {
    float x, y, u, v, projection, travel;
};
const QSGGeometry::AttributeSet& textureAttributes() {
    static const QSGGeometry::Attribute attributes[] = {
        QSGGeometry::Attribute::create(0, 2, QSGGeometry::FloatType, true),
        QSGGeometry::Attribute::create(1, 2, QSGGeometry::FloatType),
        QSGGeometry::Attribute::create(2, 2, QSGGeometry::FloatType)
    };
    static const QSGGeometry::AttributeSet set = { 3, sizeof(TextureVertex), attributes };
    return set;
}
} // namespace

ShimmerMaterial::ShimmerMaterial(bool textured): m_textured(textured) { setFlag(Blending, true); }
QSGMaterialType* ShimmerMaterial::type() const {
    static QSGMaterialType skeletonType, textureType;
    return m_textured ? &textureType : &skeletonType;
}
QSGMaterialShader* ShimmerMaterial::createShader(QSGRendererInterface::RenderMode) const {
    return new ShimmerShader(m_textured);
}
int ShimmerMaterial::compare(const QSGMaterial* other) const {
    const auto* rhs      = static_cast<const ShimmerMaterial*>(other);
    const auto  key      = texture ? texture->comparisonKey() : 0;
    const auto  otherKey = rhs->texture ? rhs->texture->comparisonKey() : 0;
    if (key != otherKey) return key < otherKey ? -1 : 1;
    const float a[] = { frame.phase, frame.minimum, frame.maximum };
    const float b[] = { rhs->frame.phase, rhs->frame.minimum, rhs->frame.maximum };
    for (int i = 0; i < 3; ++i)
        if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
    return 0;
}
SkeletonNode::SkeletonNode() {
    auto* mesh = new QSGGeometry(skeletonAttributes(), 4);
    mesh->setDrawingMode(QSGGeometry::DrawTriangleStrip);
    setGeometry(mesh);
    setMaterial(new ShimmerMaterial);
    setFlags(OwnsGeometry | OwnsMaterial);
}
void SkeletonNode::setData(QSizeF size, QColor color, qreal radius, const shimmer::Area& area,
                           const shimmer::Frame& frame) {
    auto* shader = static_cast<ShimmerMaterial*>(material());
    if (shader->frame != frame) {
        shader->frame = frame;
        markDirty(DirtyMaterial);
    }
    radius = std::clamp(radius, qreal(0), std::min(size.width(), size.height()) / 2);
    if (m_size == size && m_color == color && m_radius == radius && m_area == area) return;
    m_size               = size;
    m_color              = color;
    m_radius             = radius;
    m_area               = area;
    auto*       vertices = static_cast<SkeletonVertex*>(geometry()->vertexData());
    const float w = size.width(), h = size.height(), alpha = color.alphaF();
    for (int i = 0; i < 4; ++i) {
        const float x = i >= 2 ? w : 0, y = i % 2 ? h : 0;
        vertices[i] = { x,
                        y,
                        x - w / 2,
                        y - h / 2,
                        w / 2,
                        h / 2,
                        float(color.redF()) * alpha,
                        float(color.greenF()) * alpha,
                        float(color.blueF()) * alpha,
                        alpha,
                        area.at({ x, y }),
                        area.travel,
                        float(radius) };
    }
    markDirty(DirtyGeometry);
}

ShimmerTextureNode::ShimmerTextureNode(QQuickWindow* window, QSGTextureProvider* provider,
                                       quint64 revision, std::function<void(bool)> ready)
    : m_provider(provider), m_onReady(std::move(ready)), m_revision(revision) {
    setFlag(UsePreprocess, true);
    if (window->rendererInterface()->graphicsApi() == QSGRendererInterface::Software) {
        m_image = window->createImageNode();
        m_draw  = m_image;
    } else {
        m_geometry = new QSGGeometryNode;
        auto* mesh = new QSGGeometry(textureAttributes(), 4);
        mesh->setDrawingMode(QSGGeometry::DrawTriangleStrip);
        m_geometry->setGeometry(mesh);
        m_material = new ShimmerMaterial(true);
        m_geometry->setMaterial(m_material);
        m_geometry->setFlags(OwnsGeometry | OwnsMaterial);
        m_draw = m_geometry;
    }
    m_connections.append(connect(
        provider,
        &QSGTextureProvider::textureChanged,
        this,
        [this] {
            m_draw->markDirty(DirtyMaterial);
        },
        Qt::DirectConnection));
    m_connections.append(connect(
        provider,
        &QObject::destroyed,
        this,
        [this] {
            if (m_draw->parent()) removeChildNode(m_draw);
            if (m_image) m_image->setTexture(nullptr);
            m_texture = nullptr;
            if (m_material) m_material->texture = nullptr;
            if (m_ready) {
                m_ready = false;
                m_onReady(false);
            }
        },
        Qt::DirectConnection));
}
ShimmerTextureNode::~ShimmerTextureNode() {
    for (const auto& c : m_connections) disconnect(c);
    delete m_draw;
    if (m_ready) m_onReady(false);
}
void ShimmerTextureNode::setData(QSizeF size, const shimmer::Area& area,
                                 const shimmer::Frame& frame) {
    if (m_material && m_material->frame != frame) {
        m_material->frame = frame;
        m_draw->markDirty(DirtyMaterial);
    }
    if (size == m_size && area == m_area) return;
    m_size = size;
    m_area = area;
    updateGeometry();
}
void ShimmerTextureNode::updateGeometry() {
    if (m_image) {
        m_image->setRect(QRectF(QPointF(), m_size));
        if (m_texture) m_image->setSourceRect(QRectF(QPointF(), m_texture->textureSize()));
    } else {
        auto* vertices = static_cast<TextureVertex*>(m_geometry->geometry()->vertexData());
        for (int i = 0; i < 4; ++i) {
            const bool  right = i >= 2, bottom = i % 2;
            const float x = right ? m_size.width() : 0, y = bottom ? m_size.height() : 0;
            vertices[i] = { x,
                            y,
                            float(right ? m_uv.right() : m_uv.left()),
                            float(bottom ? m_uv.bottom() : m_uv.top()),
                            m_area.at({ x, y }),
                            m_area.travel };
        }
        m_draw->markDirty(DirtyGeometry);
    }
}
void ShimmerTextureNode::preprocess() {
    auto* texture = m_provider ? m_provider->texture() : nullptr;
    if (auto* dynamic = qobject_cast<QSGDynamicTexture*>(texture)) {
        if (dynamic->updateTexture()) m_draw->markDirty(DirtyMaterial);
    }
    const bool ready = texture && ! texture->textureSize().isEmpty();
    if (ready) {
        if (m_texture != texture) {
            m_texture = texture;
            if (m_material)
                m_material->texture = texture;
            else
                m_image->setTexture(texture);
            m_draw->markDirty(DirtyMaterial);
            updateGeometry();
        }
        const auto uv = texture->normalizedTextureSubRect();
        if (m_uv != uv) {
            m_uv = uv;
            updateGeometry();
        }
        // A transform root preserves software's cached inherited state for new children.
        if (! m_draw->parent()) appendChildNode(m_draw);
    } else {
        if (m_draw->parent()) removeChildNode(m_draw);
        if (m_image) m_image->setTexture(nullptr);
        m_texture = nullptr;
        if (m_material) m_material->texture = nullptr;
    }
    if (m_ready != ready) {
        m_ready = ready;
        m_onReady(ready);
    }
}
} // namespace qml_material::sg
