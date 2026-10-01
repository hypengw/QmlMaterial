#pragma once

#include "qml_material/shape/rounded_polygon.hpp"

namespace qml_material
{

class QML_MATERIAL_API MeasuredPath {
public:
    struct Location {
        QPointF point;
        QPointF tangent;
    };
    explicit MeasuredPath(std::vector<Cubic> curves = {}, qreal tolerance = 0.001);
    qreal        length() const { return m_length; }
    Location     location(qreal distance) const;
    QPainterPath segment(qreal start, qreal end) const;
    // Input is a continuous closed contour; extent is limited to one lap.
    QPainterPath cyclicSegment(qreal start, qreal extent) const;

private:
    struct Sample {
        qreal t;
        qreal distance;
    };
    struct Entry {
        Cubic               curve;
        qreal               start;
        qreal               length;
        std::vector<Sample> samples;
    };
    qreal              parameter(const Entry& entry, qreal distance) const;
    std::vector<Entry> m_entries;
    qreal              m_length = 0;
};

} // namespace qml_material
