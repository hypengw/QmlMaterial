#pragma once

#include "qml_material/anim/shimmer.hpp"
#include <QColor>
#include <QQuickItem>

namespace qml_material
{
class ShimmerClient;
/** Texture-free rounded placeholder. @ingroup impl */
class QML_MATERIAL_API Skeleton : public QQuickItem {
    Q_OBJECT
    QML_NAMED_ELEMENT(SkeletonBase)
    Q_PROPERTY(Shimmer* shimmer READ shimmer WRITE setShimmer NOTIFY shimmerChanged FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged FINAL)
    Q_PROPERTY(bool supported READ supported NOTIFY supportedChanged FINAL)
public:
    explicit Skeleton(QQuickItem* parent = nullptr);
    ~Skeleton() override;
    Shimmer*      shimmer() const;
    void          setShimmer(Shimmer*);
    QColor        color() const { return m_color; }
    void          setColor(const QColor&);
    qreal         radius() const { return m_radius; }
    void          setRadius(qreal);
    bool          supported() const;
    Q_SIGNAL void shimmerChanged();
    Q_SIGNAL void colorChanged();
    Q_SIGNAL void radiusChanged();
    Q_SIGNAL void supportedChanged();

protected:
    void     geometryChange(const QRectF&, const QRectF&) override;
    QSGNode* updatePaintNode(QSGNode*, UpdatePaintNodeData*) override;

private:
    std::unique_ptr<ShimmerClient> m_client;
    QColor                         m_color  = Qt::transparent;
    qreal                          m_radius = 0;
};
} // namespace qml_material
