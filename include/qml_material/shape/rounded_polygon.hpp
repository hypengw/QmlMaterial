#pragma once

#include <QtGui/QPainterPath>
#include <QtGui/QTransform>
#include <vector>
#include <utility>
#include "qml_material/export.hpp"

namespace qml_material
{

struct CornerRounding {
    qreal radius    = 0;
    qreal smoothing = 0;
};

struct QML_MATERIAL_API Cubic {
    QPointF                 anchor0, control0, control1, anchor1;
    QPointF                 point(qreal t) const;
    std::pair<Cubic, Cubic> split(qreal t) const;
    Cubic                   reversed() const;
    Cubic                   transformed(const QTransform& transform) const;
    static Cubic            line(QPointF from, QPointF to);
    static Cubic            arc(QPointF center, QPointF from, QPointF to);
};

/** @ingroup qml_material
 * Closed cubic geometry with corner features retained for morph matching.
 */
class QML_MATERIAL_API RoundedPolygon {
public:
    struct Vertex {
        QPointF        position;
        CornerRounding rounding;
    };
    struct Feature {
        std::vector<Cubic> cubics;
        bool               corner = false;
        bool               convex = false;
    };

    RoundedPolygon() = default;
    explicit RoundedPolygon(const std::vector<Vertex>& vertices, QPointF center = {});
    const std::vector<Feature>& features() const { return m_features; }
    std::vector<Cubic>          cubics() const;
    QPointF                     center() const { return m_center; }
    QPainterPath                path() const;
    RoundedPolygon              transformed(const QTransform& transform) const;
    RoundedPolygon              normalized(bool radial = false) const;

private:
    std::vector<Feature> m_features;
    QPointF              m_center;
};

class QML_MATERIAL_API Morph {
public:
    Morph(const RoundedPolygon& from, const RoundedPolygon& to);
    QPainterPath       path(qreal progress) const;
    std::vector<Cubic> cubics(qreal progress) const;
    qsizetype          curveCount() const { return qsizetype(m_match.size()); }

private:
    std::vector<std::pair<Cubic, Cubic>> m_match;
};

} // namespace qml_material
