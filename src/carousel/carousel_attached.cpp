#include "qml_material/carousel/carousel_attached.hpp"

namespace qml_material
{

CarouselAttached::CarouselAttached(QObject* parent): QObject(parent) {}

qreal CarouselAttached::itemWidth() const { return m_width; }
qreal CarouselAttached::itemHeight() const { return m_height; }
qreal CarouselAttached::maskStart() const { return m_mask_start; }
qreal CarouselAttached::maskEnd() const { return m_mask_end; }
qreal CarouselAttached::parallaxShift() const { return m_parallax_shift; }
qreal CarouselAttached::visibleFraction() const
{
    return qMax(0.0, 1.0 - m_mask_start - m_mask_end);
}
int CarouselAttached::sizeClass() const { return m_size_class; }
int CarouselAttached::index() const { return m_index; }
bool CarouselAttached::isActive() const { return m_is_active; }

void CarouselAttached::setGeometry(qreal width, qreal height, qreal maskStart, qreal maskEnd,
                                   qreal parallaxShift, int sizeClass, int index, bool isActive)
{
    bool changed = !qFuzzyCompare(m_width, width) || !qFuzzyCompare(m_height, height)
        || !qFuzzyCompare(m_mask_start, maskStart) || !qFuzzyCompare(m_mask_end, maskEnd)
        || !qFuzzyCompare(m_parallax_shift, parallaxShift) || m_size_class != sizeClass
        || m_index != index || m_is_active != isActive;
    m_width          = width;
    m_height         = height;
    m_mask_start     = maskStart;
    m_mask_end       = maskEnd;
    m_parallax_shift = parallaxShift;
    m_size_class     = sizeClass;
    m_index          = index;
    m_is_active      = isActive;
    if (changed) {
        Q_EMIT geometryChanged();
    }
}

} // namespace qml_material
