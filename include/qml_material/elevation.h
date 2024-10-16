#pragma once

#include <QQuickItem>
#include <QQmlEngine>

#include "qml_material/corner.h"

namespace qml_material
{

class Elevation : public QQuickItem {
    Q_OBJECT
    QML_NAMED_ELEMENT(ElevationImpl)

    Q_PROPERTY(qint32 elevation READ elevation WRITE setelevation NOTIFY elevationChanged FINAL)
    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(CornersGroup corners READ corners WRITE setCorners NOTIFY cornersChanged FINAL)
public:
    Elevation(QQuickItem* parent = nullptr);
    ~Elevation() override;

    auto          elevation() const -> qint32;
    void          setelevation(qint32);
    Q_SIGNAL void elevationChanged();

    auto          corners() const -> const CornersGroup&;
    void          setCorners(const CornersGroup&);
    Q_SIGNAL void cornersChanged();

    qreal         radius() const;
    void          setRadius(qreal newRadius);
    Q_SIGNAL void radiusChanged();

    QColor        color() const;
    void          setColor(const QColor& newColor);
    Q_SIGNAL void colorChanged();

    void componentComplete() override;

protected:
    void     itemChange(QQuickItem::ItemChange            change,
                        const QQuickItem::ItemChangeData& value) override;
    QSGNode* updatePaintNode(QSGNode* node, QQuickItem::UpdatePaintNodeData* data) override;

private:
    qint32       m_elevation;
    CornersGroup m_corners;
    qreal        m_radius;
    QColor       m_color;
};
} // namespace qml_material