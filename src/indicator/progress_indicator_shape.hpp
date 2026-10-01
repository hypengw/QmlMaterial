#pragma once

#include <QtQuickShapes/private/qquickshape_p.h>
#include <QtCore/QPointer>
#include "qml_material/control/progress_indicator.hpp"

namespace qml_material
{
class ProgressIndicatorShape : public QQuickShape {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(ProgressIndicator* source READ source WRITE setSource NOTIFY sourceChanged FINAL)
    Q_PROPERTY(bool circular MEMBER m_circular NOTIFY appearanceChanged FINAL)
    Q_PROPERTY(bool legacyRadius MEMBER m_legacyRadius NOTIFY appearanceChanged FINAL)
    Q_PROPERTY(bool wavy MEMBER m_wavy NOTIFY appearanceChanged FINAL)
    Q_PROPERTY(qreal strokeWidth MEMBER m_stroke NOTIFY appearanceChanged FINAL)
    Q_PROPERTY(QColor strokeColor MEMBER m_color NOTIFY appearanceChanged FINAL)
    Q_PROPERTY(QColor trackColor MEMBER m_track NOTIFY appearanceChanged FINAL)
    Q_PROPERTY(
        QVariantList indicators READ indicators WRITE setIndicators NOTIFY appearanceChanged FINAL)
    Q_PROPERTY(qreal startAngle MEMBER m_start NOTIFY appearanceChanged FINAL)
    Q_PROPERTY(qreal endAngle MEMBER m_end NOTIFY appearanceChanged FINAL)
    Q_PROPERTY(qreal inactiveStartAngle MEMBER m_trackStart NOTIFY appearanceChanged FINAL)
    Q_PROPERTY(qreal inactiveSweepAngle MEMBER m_trackSweep NOTIFY appearanceChanged FINAL)
    Q_PROPERTY(qreal waveLength MEMBER m_wavelength NOTIFY appearanceChanged FINAL)
    Q_PROPERTY(qreal waveAmplitude MEMBER m_amplitude NOTIFY appearanceChanged FINAL)
    Q_PROPERTY(qreal phase MEMBER m_phase NOTIFY appearanceChanged FINAL)
public:
    explicit ProgressIndicatorShape(QQuickItem* parent = nullptr);
    ProgressIndicator*  source() const { return m_source; }
    void                setSource(ProgressIndicator* source);
    QVariantList        indicators() const { return m_indicators; }
    void                setIndicators(QVariantList indicators);
    Q_INVOKABLE QPointF drawLine(qreal startFraction, qreal endFraction, qreal gapSize) const;
    Q_SIGNAL void       sourceChanged();
    Q_SIGNAL void       appearanceChanged();

protected:
    void componentComplete() override;
    void geometryChange(const QRectF& now, const QRectF& old) override;
    void updatePolish() override;

private:
    void                           invalidate();
    void                           updatePaths();
    ProgressIndicatorState         standaloneState() const;
    QPointer<ProgressIndicator>    m_source;
    QList<QMetaObject::Connection> m_connections;
    QList<QMetaObject::Connection> m_indicatorConnections;
    ProgressIndicatorGeometry      m_geometry;
    QList<QQuickShapePath*>        m_paths;
    bool                           m_dirty        = true;
    bool                           m_circular     = false;
    bool                           m_legacyRadius = false;
    bool                           m_wavy         = false;
    qreal                          m_stroke       = 4;
    QColor                         m_color        = Qt::black;
    QColor                         m_track        = Qt::transparent;
    QVariantList                   m_indicators;
    qreal                          m_start      = 0;
    qreal                          m_end        = 0;
    qreal                          m_trackStart = 0;
    qreal                          m_trackSweep = 0;
    qreal                          m_wavelength = 30;
    qreal                          m_amplitude  = 3;
    qreal                          m_phase      = 0;
};
} // namespace qml_material
