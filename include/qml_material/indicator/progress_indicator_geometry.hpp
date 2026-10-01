#pragma once

#include "qml_material/path/measured_path.hpp"
#include <QtGui/QColor>
#include <QtCore/QSizeF>

namespace qml_material
{
struct ProgressSegment {
    qreal  start = 0;
    qreal  end   = 0;
    QColor color;
    qreal  gapSize      = 0;
    qreal  trackGapSize = -1;
};

struct ProgressIndicatorState {
    bool                         circular          = false;
    bool                         indeterminate     = false;
    bool                         wavy              = false;
    bool                         mirrored          = false;
    bool                         legacyRadius      = false;
    bool                         legacyCenter      = false;
    bool                         legacySegments    = false;
    qreal                        position          = 0;
    qreal                        phase             = 0;
    qreal                        amplitudeFraction = 1;
    qreal                        rotation          = 0;
    qreal                        startAngle        = -90;
    qreal                        strokeWidth       = 4;
    qreal                        waveLength        = 40;
    qreal                        waveAmplitude     = 3;
    qreal                        gapSize           = 4;
    qreal                        gapAngle          = -1;
    qreal                        drain             = 0;
    qreal                        opacity           = 1;
    QColor                       trackColor;
    QColor                       stopColor;
    bool                         stopVisible = true;
    std::vector<ProgressSegment> segments;
    // Compatibility helpers supply an explicit inactive arc.
    bool  explicitTrack = false;
    qreal trackStart    = 0;
    qreal trackSweep    = 0;
};

struct ProgressDrawPath {
    QPainterPath path;
    QColor       color;
    qreal        strokeWidth = 0;
};

class QML_MATERIAL_API ProgressIndicatorGeometry {
public:
    static QPointF lineBounds(qreal start, qreal end, qreal gap, qreal stroke, qreal width);
    // The result is retained until the next render on this instance.
    const std::vector<ProgressDrawPath>& render(const ProgressIndicatorState& state, QSizeF size);
    quint64                              cacheRevision() const { return m_revision; }

private:
    void                          rebuild(const ProgressIndicatorState& state, QSizeF size);
    QSizeF                        m_size;
    bool                          m_circular         = false;
    bool                          m_wavy             = false;
    bool                          m_legacyRadius     = false;
    bool                          m_legacyCenter     = false;
    qreal                         m_stroke           = -1;
    qreal                         m_wavelength       = -1;
    qreal                         m_amplitude        = -1;
    qreal                         m_radius           = 0;
    qreal                         m_actualWavelength = 0;
    int                           m_cycles           = 0;
    MeasuredPath                  m_wave;
    MeasuredPath                  m_circle;
    quint64                       m_revision = 0;
    std::vector<ProgressDrawPath> m_result;
};
} // namespace qml_material
