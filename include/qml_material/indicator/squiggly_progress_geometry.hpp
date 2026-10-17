#pragma once

#include <QPainterPath>
#include <QSizeF>
#include "qml_material/export.hpp"

namespace qml_material
{
struct SquigglyProgressState {
    qreal progress          = 0;
    qreal phase             = 0;
    qreal amplitudeFraction = 0;
    qreal waveLength        = 20;
    qreal waveAmplitude     = 1.5;
    bool  transitionEnabled = true;
};
struct SquigglyProgressPaths {
    QPainterPath active;
    QPainterPath inactive;
    QPointF      startCap;
    bool         valid = false;
};
class QML_MATERIAL_API SquigglyProgressGeometry {
public:
    static SquigglyProgressPaths render(const SquigglyProgressState&, QSizeF);
};
} // namespace qml_material
