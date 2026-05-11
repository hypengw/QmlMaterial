#pragma once

#include <QtQuick/private/qquickpath_p.h>

#include "qml_material/export.hpp"

namespace qml_material
{

// Wavy path segment between the previous element's endpoint and (x, y).
// Geometry mirrors material-components-android LinearDrawingDelegate
// (two cubic Beziers per cycle, smoothness = 0.48). The wavelength is
// constant in pixels; the path is truncated with de Casteljau so it lands
// exactly at the declared endpoint regardless of phase.
class QML_MATERIAL_API PathWave : public QQuickCurve {
    Q_OBJECT
    Q_PROPERTY(qreal amplitude READ amplitude WRITE set_amplitude NOTIFY amplitudeChanged FINAL)
    Q_PROPERTY(
        qreal waveLength READ wave_length WRITE set_wave_length NOTIFY waveLengthChanged FINAL)
    Q_PROPERTY(qreal phase READ phase WRITE set_phase NOTIFY phaseChanged FINAL)
    Q_PROPERTY(
        qreal smoothness READ smoothness WRITE set_smoothness NOTIFY smoothnessChanged FINAL)
    QML_NAMED_ELEMENT(PathWave)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit PathWave(QObject* parent = nullptr);
    ~PathWave() override;

    auto amplitude() const noexcept -> qreal;
    auto wave_length() const noexcept -> qreal;
    auto phase() const noexcept -> qreal;
    auto smoothness() const noexcept -> qreal;

    void set_amplitude(qreal);
    void set_wave_length(qreal);
    void set_phase(qreal);
    void set_smoothness(qreal);

    void addToPath(QPainterPath& path, const QQuickPathData& data) override;

Q_SIGNALS:
    void amplitudeChanged();
    void waveLengthChanged();
    void phaseChanged();
    void smoothnessChanged();

private:
    qreal m_amplitude { 4.0 };
    qreal m_wave_length { 30.0 };
    qreal m_phase { 0.0 };
    qreal m_smoothness { 0.48 };
};

} // namespace qml_material
