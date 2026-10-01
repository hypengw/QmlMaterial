#include "progress_indicator_shape.hpp"
#include "qml_material/anim/linear_indicator_updator.hpp"
#include <QtQml/QQmlParserStatus>
#include <QtQml/QJSValue>

namespace qml_material
{
ProgressIndicatorShape::ProgressIndicatorShape(QQuickItem* parent): QQuickShape(parent) {
    setPreferredRendererType(QQuickShape::CurveRenderer);
    setAsynchronous(false);
    connect(this,
            &ProgressIndicatorShape::appearanceChanged,
            this,
            &ProgressIndicatorShape::invalidate);
}
void ProgressIndicatorShape::setSource(ProgressIndicator* source) {
    if (m_source == source) return;
    for (const auto& connection : m_connections) disconnect(connection);
    m_connections.clear();
    m_source = source;
    if (source) {
        m_connections.push_back(connect(
            source, &ProgressIndicator::frameChanged, this, &ProgressIndicatorShape::invalidate));
        m_connections.push_back(connect(source, &QObject::destroyed, this, [this] {
            m_source = nullptr;
            invalidate();
            emit sourceChanged();
        }));
    }
    invalidate();
    emit sourceChanged();
}
void ProgressIndicatorShape::setIndicators(QVariantList indicators) {
    if (m_indicators == indicators) return;
    for (const auto& connection : m_indicatorConnections) disconnect(connection);
    m_indicatorConnections.clear();
    m_indicators = std::move(indicators);
    for (const auto& value : m_indicators) {
        if (auto* object = qobject_cast<LinearActiveIndicatorData*>(value.value<QObject*>())) {
            m_indicatorConnections.push_back(connect(object,
                                                     &LinearActiveIndicatorData::updated,
                                                     this,
                                                     &ProgressIndicatorShape::invalidate));
            m_indicatorConnections.push_back(
                connect(object, &QObject::destroyed, this, [this, object] {
                    m_indicators.removeIf([object](const QVariant& entry) {
                        return entry.value<QObject*>() == object;
                    });
                    invalidate();
                    emit appearanceChanged();
                }));
        }
    }
    invalidate();
    emit appearanceChanged();
}
void ProgressIndicatorShape::invalidate() {
    m_dirty = true;
    polish();
}
QPointF ProgressIndicatorShape::drawLine(qreal startFraction, qreal endFraction,
                                         qreal gapSize) const {
    return ProgressIndicatorGeometry::lineBounds(
        startFraction, endFraction, gapSize, m_stroke, width());
}
void ProgressIndicatorShape::componentComplete() {
    QQuickShape::componentComplete();
    invalidate();
}
void ProgressIndicatorShape::geometryChange(const QRectF& now, const QRectF& old) {
    QQuickShape::geometryChange(now, old);
    if (now.size() != old.size()) invalidate();
}
ProgressIndicatorState ProgressIndicatorShape::standaloneState() const {
    ProgressIndicatorState s;
    s.circular       = m_circular;
    s.legacyRadius   = m_legacyRadius;
    s.legacyCenter   = m_legacyRadius;
    s.legacySegments = true;
    s.wavy           = m_wavy;
    s.strokeWidth    = m_stroke;
    s.waveLength     = m_wavelength;
    s.waveAmplitude  = m_amplitude;
    s.phase          = m_phase;
    s.trackColor     = m_track;
    s.stopVisible    = false;
    s.startAngle     = 0;
    if (! m_circular && m_indicators.empty()) s.strokeWidth = 0;
    if (m_circular) {
        s.segments.push_back({ m_start, m_end, m_color });
        s.explicitTrack = true;
        s.trackStart    = m_trackStart;
        s.trackSweep    = m_trackSweep;
    } else {
        for (const auto& value : m_indicators) {
            if (auto* data = qobject_cast<LinearActiveIndicatorData*>(value.value<QObject*>())) {
                s.segments.push_back({ data->startFraction(),
                                       data->endFraction(),
                                       data->getColor(),
                                       qreal(data->getGapSize()) });
            } else {
                const auto map = value.metaType() == QMetaType::fromType<QJSValue>()
                                     ? value.value<QJSValue>().toVariant().toMap()
                                     : value.toMap();
                s.segments.push_back({ map.value("startFraction").toReal(),
                                       map.value("endFraction").toReal(),
                                       map.value("color", m_color).value<QColor>(),
                                       map.value("gapSize", 4).toReal() });
            }
        }
    }
    return s;
}
void ProgressIndicatorShape::updatePaths() {
    QPointer<ProgressIndicatorShape> guard(this);
    const auto                       state = m_source ? m_source->renderState() : standaloneState();
    setOpacity(state.opacity);
    if (! guard) return;
    const auto& paths = m_geometry.render(state, { width(), height() });
    while (m_paths.size() < qsizetype(paths.size())) {
        auto* path   = new QQuickShapePath(this);
        auto* status = static_cast<QQmlParserStatus*>(path);
        status->classBegin();
        path->setCapStyle(QQuickShapePath::RoundCap);
        path->setJoinStyle(QQuickShapePath::RoundJoin);
        status->componentComplete();
        auto list = data();
        list.append(&list, path);
        if (! guard) return;
        m_paths.push_back(path);
    }
    for (qsizetype i = 0; i < m_paths.size(); ++i) {
        auto* path = m_paths[i];
        if (i < qsizetype(paths.size())) {
            const auto& draw = paths[i];
            path->setPath(draw.path);
            if (! guard) return;
            path->setStrokeWidth(draw.strokeWidth);
            if (! guard) return;
            path->setStrokeColor(draw.strokeWidth > 0 ? draw.color : QColor(Qt::transparent));
            if (! guard) return;
            path->setFillColor(draw.strokeWidth > 0 ? QColor(Qt::transparent) : draw.color);
            if (! guard) return;
        } else {
            path->setPath({});
            if (! guard) return;
        }
    }
}
void ProgressIndicatorShape::updatePolish() {
    QPointer<ProgressIndicatorShape> guard(this);
    if (m_dirty) {
        m_dirty = false;
        updatePaths();
        if (! guard) return;
    }
    QQuickShape::updatePolish();
}
} // namespace qml_material

#include "moc_progress_indicator_shape.cpp"
