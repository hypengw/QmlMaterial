#pragma once

#include "../item/shimmer_geometry_p.hpp"
#include <QColor>
#include <QPointer>
#include <QSGGeometryNode>
#include <QSGMaterial>
#include <QSGTextureProvider>
#include <QSGTransformNode>
#include <functional>

QT_BEGIN_NAMESPACE
class QSGImageNode;
QT_END_NAMESPACE

namespace qml_material::sg
{
class ShimmerMaterial : public QSGMaterial {
public:
    explicit ShimmerMaterial(bool textured = false);
    QSGMaterialType*   type() const override;
    QSGMaterialShader* createShader(QSGRendererInterface::RenderMode) const override;
    int                compare(const QSGMaterial*) const override;
    shimmer::Frame     frame;
    QSGTexture*        texture = nullptr;

private:
    bool m_textured;
};
class SkeletonNode : public QSGGeometryNode {
public:
    SkeletonNode();
    void setData(QSizeF, QColor, qreal radius, const shimmer::Area&, const shimmer::Frame&);

private:
    QSizeF        m_size;
    QColor        m_color;
    qreal         m_radius = -1;
    shimmer::Area m_area;
};
class ShimmerTextureNode : public QObject, public QSGTransformNode {
public:
    ShimmerTextureNode(QQuickWindow*, QSGTextureProvider*, quint64 revision,
                       std::function<void(bool)> ready);
    ~ShimmerTextureNode() override;
    quint64             revision() const { return m_revision; }
    QSGTextureProvider* provider() const { return m_provider; }
    void                setData(QSizeF, const shimmer::Area&, const shimmer::Frame&);
    void                preprocess() override;

private:
    void                           updateGeometry();
    QPointer<QSGTextureProvider>   m_provider;
    QSGNode*                       m_draw;
    QSGImageNode*                  m_image    = nullptr;
    QSGGeometryNode*               m_geometry = nullptr;
    ShimmerMaterial*               m_material = nullptr;
    QSGTexture*                    m_texture  = nullptr;
    QList<QMetaObject::Connection> m_connections;
    std::function<void(bool)>      m_onReady;
    QSizeF                         m_size;
    QRectF                         m_uv;
    shimmer::Area                  m_area;
    quint64                        m_revision;
    bool                           m_ready = false;
};
} // namespace qml_material::sg
