#include "material_shape_path.hpp"
#include <algorithm>
#include <cmath>

namespace qml_material
{

MaterialShapePath::MaterialShapePath(QObject* parent): QQuickShapePath(parent) { updateGeometry(); }
void MaterialShapePath::setShape(MaterialShape::Type value) {
    if (m_shape == value || ! MaterialShapes::isValid(value)) return;
    m_shape = value;
    updatePair();
    emit shapeChanged();
}
void MaterialShapePath::setToShape(MaterialShape::Type value) {
    if (m_toShape == value || (value != MaterialShape::None && ! MaterialShapes::isValid(value)))
        return;
    m_toShape = value;
    updatePair();
    emit toShapeChanged();
}
void MaterialShapePath::setProgress(qreal value) {
    if (! std::isfinite(value) || m_progress == value) return;
    m_progress = value;
    updateGeometry();
    emit progressChanged();
}
void MaterialShapePath::setSize(QSizeF value) {
    if (! std::isfinite(value.width()) || ! std::isfinite(value.height()) || value.width() < 0 ||
        value.height() < 0 || m_size == value)
        return;
    m_size = value;
    updateGeometry();
    emit sizeChanged();
}
void MaterialShapePath::setStartAngle(qreal value) {
    if (! std::isfinite(value) || m_startAngle == value) return;
    m_startAngle = value;
    updateGeometry();
    emit startAngleChanged();
}
void MaterialShapePath::setRadialNormalization(bool value) {
    if (m_radial == value) return;
    m_radial = value;
    updatePair();
    emit radialNormalizationChanged();
}
void MaterialShapePath::updatePair() {
    m_morph = m_toShape == MaterialShape::None || m_shape == m_toShape
                  ? nullptr
                  : MaterialShapes::morph(m_shape, m_toShape, m_radial);
    updateGeometry();
}
void MaterialShapePath::updateGeometry() {
    const auto path =
        m_morph ? m_morph->path(m_progress) : MaterialShapes::polygon(m_shape, m_radial).path();
    const auto side = std::min(m_size.width(), m_size.height());
    QTransform transform;
    transform.translate(m_size.width() / 2, m_size.height() / 2);
    transform.rotate(m_startAngle);
    transform.scale(side, side);
    transform.translate(-.5, -.5);
    setPath(transform.map(path));
}

} // namespace qml_material

#include "moc_material_shape_path.cpp"
