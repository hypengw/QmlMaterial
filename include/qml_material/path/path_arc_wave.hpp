#pragma once

#include <QtQuick/private/qquickpath_p.h>

#include "qml_material/export.hpp"

namespace qml_material
{

// Wavy circular arc segment. Geometry mirrors material-components-android
// CircularDrawingDelegate.createWavyPath: anchors every half-wavelength along
// the base arc, alternating ±amplitude radial offset, connected by cubic
// Beziers whose control points sit `smoothness * halfWl` along the arc tangent.
//
// The path is truncated with de Casteljau so it lands exactly at the
// requested sweepAngle regardless of phase. API follows PathAngleArc
// (qquickpath.cpp:2326): inherited x/y are ignored.
class QML_MATERIAL_API PathArcWave : public QQuickCurve {
    Q_OBJECT
    Q_PROPERTY(qreal centerX READ center_x WRITE set_center_x NOTIFY centerXChanged FINAL)
    Q_PROPERTY(qreal centerY READ center_y WRITE set_center_y NOTIFY centerYChanged FINAL)
    Q_PROPERTY(qreal radius READ radius WRITE set_radius NOTIFY radiusChanged FINAL)
    Q_PROPERTY(qreal startAngle READ start_angle WRITE set_start_angle NOTIFY startAngleChanged FINAL)
    Q_PROPERTY(qreal sweepAngle READ sweep_angle WRITE set_sweep_angle NOTIFY sweepAngleChanged FINAL)
    Q_PROPERTY(qreal amplitude READ amplitude WRITE set_amplitude NOTIFY amplitudeChanged FINAL)
    Q_PROPERTY(qreal waveLength READ wave_length WRITE set_wave_length NOTIFY waveLengthChanged FINAL)
    Q_PROPERTY(qreal phase READ phase WRITE set_phase NOTIFY phaseChanged FINAL)
    Q_PROPERTY(qreal smoothness READ smoothness WRITE set_smoothness NOTIFY smoothnessChanged FINAL)
    QML_NAMED_ELEMENT(PathArcWave)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit PathArcWave(QObject* parent = nullptr);
    ~PathArcWave() override;

    auto center_x() const noexcept -> qreal;
    auto center_y() const noexcept -> qreal;
    auto radius() const noexcept -> qreal;
    auto start_angle() const noexcept -> qreal;
    auto sweep_angle() const noexcept -> qreal;
    auto amplitude() const noexcept -> qreal;
    auto wave_length() const noexcept -> qreal;
    auto phase() const noexcept -> qreal;
    auto smoothness() const noexcept -> qreal;

    void set_center_x(qreal);
    void set_center_y(qreal);
    void set_radius(qreal);
    void set_start_angle(qreal);
    void set_sweep_angle(qreal);
    void set_amplitude(qreal);
    void set_wave_length(qreal);
    void set_phase(qreal);
    void set_smoothness(qreal);

    void addToPath(QPainterPath& path, const QQuickPathData& data) override;

Q_SIGNALS:
    void centerXChanged();
    void centerYChanged();
    void radiusChanged();
    void startAngleChanged();
    void sweepAngleChanged();
    void amplitudeChanged();
    void waveLengthChanged();
    void phaseChanged();
    void smoothnessChanged();

private:
    qreal m_center_x { 0.0 };
    qreal m_center_y { 0.0 };
    qreal m_radius { 16.0 };
    qreal m_start_angle { 0.0 };
    qreal m_sweep_angle { 360.0 };
    qreal m_amplitude { 4.0 };
    qreal m_wave_length { 30.0 };
    qreal m_phase { 0.0 };
    qreal m_smoothness { 0.48 };
};

} // namespace qml_material
