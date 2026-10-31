#include "qml_material/item/backdrop_surface.hpp"
#include <QQuickWindow>
#include <cmath>

namespace qml_material
{
BackdropSurface::BackdropSurface(QQuickItem* parent): QQuickItem(parent) {
    m_activity = std::make_unique<ItemActivity>(this);
    connect(m_activity.get(), &ItemActivity::changed, this, &BackdropSurface::refresh);
    connect(this, &QQuickItem::windowChanged, this, &BackdropSurface::bindWindow);
    connect(this, &QQuickItem::parentChanged, this, &BackdropSurface::refresh);
    bindWindow();
}
BackdropSurface::~BackdropSurface() { disconnect(m_frameConnection); }
void BackdropSurface::setSourceItem(QQuickItem* source) {
    if (m_source == source) return;
    for (const auto& c : m_sourceConnections) disconnect(c);
    m_sourceConnections.clear();
    m_source = source;
    if (source) {
        m_sourceConnections.append(
            connect(source, &QQuickItem::widthChanged, this, &BackdropSurface::refresh));
        m_sourceConnections.append(
            connect(source, &QQuickItem::heightChanged, this, &BackdropSurface::refresh));
        m_sourceConnections.append(
            connect(source, &QQuickItem::windowChanged, this, &BackdropSurface::refresh));
        m_sourceConnections.append(
            connect(source, &QQuickItem::parentChanged, this, &BackdropSurface::refresh));
        m_sourceConnections.append(connect(source, &QObject::destroyed, this, [this] {
            refresh();
            Q_EMIT sourceItemChanged();
        }));
    }
    refresh();
    Q_EMIT sourceItemChanged();
}
void BackdropSurface::setActive(bool value) {
    if (m_active == value) return;
    m_active = value;
    refresh();
    Q_EMIT activeChanged();
}
void BackdropSurface::setSampleScale(qreal value) {
    if (! std::isfinite(value) || value <= 0 || value > 1 || m_scale == value) return;
    m_scale = value;
    refresh();
    Q_EMIT samplingChanged();
}
void BackdropSurface::setMaximumTextureSize(int value) {
    if (value < 1 || m_maximum == value) return;
    m_maximum = value;
    refresh();
    Q_EMIT samplingChanged();
}
void BackdropSurface::setCapturePadding(qreal value) {
    if (! std::isfinite(value) || value < 0 || m_padding == value) return;
    m_padding = value;
    refresh();
    Q_EMIT samplingChanged();
}
void BackdropSurface::bindWindow() {
    disconnect(m_frameConnection);
    if (window()) {
        // Observing frames catches ancestor transforms without requesting new frames.
        m_frameConnection =
            connect(window(), &QQuickWindow::afterAnimating, this, &BackdropSurface::refresh);
    }
    refresh();
}
void BackdropSurface::geometryChange(const QRectF& now, const QRectF& old) {
    QQuickItem::geometryChange(now, old);
    if (m_activity) refresh();
}
void BackdropSurface::refresh() {
    bool valid = m_active && m_activity->active() && m_source && window() &&
                 m_source->window() == window() && width() > 0 && height() > 0 &&
                 m_source->width() > 0 && m_source->height() > 0;
    for (QQuickItem* item = this; valid && item; item = item->parentItem())
        if (item == m_source) valid = false;
    QRectF rect;
    QSize  size;
    if (valid) {
        bool       ok        = false;
        const auto transform = itemTransform(m_source, &ok);
        valid = ok && transform.isAffine() && qFuzzyIsNull(transform.m12()) &&
                qFuzzyIsNull(transform.m21()) && transform.m11() > 0 && transform.m22() > 0;
        if (valid) {
            const QRectF area(
                -m_padding, -m_padding, width() + 2 * m_padding, height() + 2 * m_padding);
            rect                    = transform.mapRect(area);
            const auto dpr          = window()->effectiveDevicePixelRatio();
            const auto physicalArea = mapRectToScene(area);
            QSizeF     pixels(physicalArea.width() * dpr * m_scale,
                              physicalArea.height() * dpr * m_scale);
            const auto limit = qMin(1.0, m_maximum / qMax(pixels.width(), pixels.height()));
            size             = QSize(qMax(1, qCeil(pixels.width() * limit)),
                                     qMax(1, qCeil(pixels.height() * limit)));
        }
    }
    if (m_enabled == valid && m_rect == rect && m_size == size) return;
    m_enabled = valid;
    m_rect    = rect;
    m_size    = size;
    Q_EMIT captureChanged();
}
} // namespace qml_material
