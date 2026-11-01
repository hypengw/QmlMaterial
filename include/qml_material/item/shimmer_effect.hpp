#pragma once

#include "qml_material/anim/shimmer.hpp"
#include <QPointer>
#include <QQuickItem>

namespace qml_material
{
class ShimmerClient;
/** Alpha modulation of an existing texture provider. @ingroup qml */
class QML_MATERIAL_API ShimmerEffect : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(Shimmer* shimmer READ shimmer WRITE setShimmer NOTIFY shimmerChanged FINAL)
    Q_PROPERTY(QQuickItem* source READ source WRITE setSource NOTIFY sourceChanged FINAL)
public:
    explicit ShimmerEffect(QQuickItem* parent = nullptr);
    ~ShimmerEffect() override;
    Shimmer*      shimmer() const;
    void          setShimmer(Shimmer*);
    QQuickItem*   source() const { return m_source; }
    void          setSource(QQuickItem*);
    Q_SIGNAL void shimmerChanged();
    Q_SIGNAL void sourceChanged();

protected:
    void     geometryChange(const QRectF&, const QRectF&) override;
    QSGNode* updatePaintNode(QSGNode*, UpdatePaintNodeData*) override;
    void     releaseResources() override;

private:
    void                           invalidateSource();
    std::unique_ptr<ShimmerClient> m_client;
    QPointer<QQuickItem>           m_source;
    QList<QMetaObject::Connection> m_sourceConnections;
    quint64                        m_sourceRevision = 0;
};
} // namespace qml_material
