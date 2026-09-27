#pragma once

#include <QtQuickShapes/private/qquickshape_p.h>
#include "qml_material/shape/material_shapes.hpp"

namespace qml_material
{

/** @ingroup qml_material
 * Fits normalized Material geometry inside size, preserving aspect ratio.
 */
class MaterialShapePath : public QQuickShapePath {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(MaterialShape::Type shape READ shape WRITE setShape NOTIFY shapeChanged FINAL)
    Q_PROPERTY(
        MaterialShape::Type toShape READ toShape WRITE setToShape NOTIFY toShapeChanged FINAL)
    Q_PROPERTY(qreal progress READ progress WRITE setProgress NOTIFY progressChanged FINAL)
    Q_PROPERTY(QSizeF size READ size WRITE setSize NOTIFY sizeChanged FINAL)
    Q_PROPERTY(qreal startAngle READ startAngle WRITE setStartAngle NOTIFY startAngleChanged FINAL)
    Q_PROPERTY(bool radialNormalization READ radialNormalization WRITE setRadialNormalization NOTIFY
                   radialNormalizationChanged FINAL)
public:
    explicit MaterialShapePath(QObject* parent = nullptr);
    MaterialShape::Type shape() const { return m_shape; }
    MaterialShape::Type toShape() const { return m_toShape; }
    qreal               progress() const { return m_progress; }
    QSizeF              size() const { return m_size; }
    qreal               startAngle() const { return m_startAngle; }
    bool                radialNormalization() const { return m_radial; }
    void                setShape(MaterialShape::Type shape);
    void                setToShape(MaterialShape::Type shape);
    void                setProgress(qreal progress);
    void                setSize(QSizeF size);
    void                setStartAngle(qreal angle);
    void                setRadialNormalization(bool radial);
    Q_SIGNAL void       shapeChanged();
    Q_SIGNAL void       toShapeChanged();
    Q_SIGNAL void       progressChanged();
    Q_SIGNAL void       sizeChanged();
    Q_SIGNAL void       startAngleChanged();
    Q_SIGNAL void       radialNormalizationChanged();

private:
    void                         updateGeometry();
    void                         updatePair();
    MaterialShape::Type          m_shape    = MaterialShape::Circle;
    MaterialShape::Type          m_toShape  = MaterialShape::None;
    qreal                        m_progress = 0;
    QSizeF                       m_size { 1, 1 };
    qreal                        m_startAngle = 0;
    bool                         m_radial     = false;
    std::shared_ptr<const Morph> m_morph;
};

} // namespace qml_material
