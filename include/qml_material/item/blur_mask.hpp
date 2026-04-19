#pragma once

#include <QColor>
#include <QQmlEngine>
#include <QQuickItem>

#include "qml_material/util/corner.hpp"

namespace qml_material
{

/// QQuickItem that draws a gaussian-blurred mask of a rect / rrect.
/// Thin shell around `sg::BlurMaskMaterial` + `sg::create_blur_mask_geometry` —
/// the decision whether to use this vs `RRectShadowImpl` belongs in the QML
/// wrapper layer (see qml/internal/Elevation.qml).
class BlurMask : public QQuickItem {
    Q_OBJECT
    QML_NAMED_ELEMENT(BlurMaskImpl)

public:
    enum Style {
        Normal = 0,
        Solid  = 1,
        Outer  = 2,
        Inner  = 3,
    };
    Q_ENUM(Style)

    Q_PROPERTY(qreal sigma READ sigma WRITE setSigma NOTIFY sigmaChanged FINAL)
    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(qml_material::CornersGroup corners READ corners WRITE setCorners NOTIFY
                   cornersChanged FINAL)
    Q_PROPERTY(Style style READ style WRITE setStyle NOTIFY styleChanged FINAL)

    BlurMask(QQuickItem* parent = nullptr);
    ~BlurMask() override;

    qreal         sigma() const;
    void          setSigma(qreal);
    Q_SIGNAL void sigmaChanged(qreal);

    qreal         radius() const;
    void          setRadius(qreal);
    Q_SIGNAL void radiusChanged(qreal);

    QColor        color() const;
    void          setColor(const QColor&);
    Q_SIGNAL void colorChanged(QColor);

    auto          corners() const -> const CornersGroup&;
    void          setCorners(const CornersGroup&);
    Q_SIGNAL void cornersChanged(CornersGroup);

    Style         style() const;
    void          setStyle(Style);
    Q_SIGNAL void styleChanged(Style);

protected:
    QSGNode* updatePaintNode(QSGNode* node, QQuickItem::UpdatePaintNodeData* data) override;

private:
    qreal        m_sigma { 0 };
    qreal        m_radius { 0 };
    QColor       m_color { Qt::black };
    CornersGroup m_corners {};
    Style        m_style { Normal };
};

} // namespace qml_material
