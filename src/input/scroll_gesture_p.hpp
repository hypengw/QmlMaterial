#pragma once

#include "scroll_coordinates_p.hpp"
#include <QList>

namespace qml_material
{
class ScrollGesture {
public:
    struct Candidate {
        QPointer<QQuickItem> item;
        Qt::Orientations     axes;
    };
    void clear() {
        m_basis = nullptr;
        m_axes  = {};
        m_samples.clear();
    }
    QQuickItem* basis() const { return m_basis; }
    void        sample(QPointF point, ulong timestamp) {
        if (! m_samples.isEmpty()) {
            const auto last     = m_samples.back();
            const auto delta    = m_basis ? project(point - last.second) : point - last.second;
            const auto previous = m_basis ? project(sampleVelocity()) : sampleVelocity();
            if (timestamp < last.first || timestamp - last.first >= 100 ||
                delta.x() * previous.x() > 0 || delta.y() * previous.y() > 0) {
                m_samples.clear();
                m_samples.append(last);
            }
        }
        m_samples.append({ timestamp, point });
        while (m_samples.size() > 10 ||
               (m_samples.size() > 2 && timestamp - m_samples.front().first > 140))
            m_samples.removeFirst();
    }
    QPointF sampleVelocity() const {
        if (m_samples.size() < 2) return {};
        const auto elapsed = m_samples.back().first - m_samples.front().first;
        return elapsed ? (m_samples.front().second - m_samples.back().second) * (1000.0 / elapsed)
                       : QPointF();
    }
    QPointF project(QPointF sceneDelta) const {
        if (! m_basis) return {};
        auto local = vectorFromScene(m_basis, sceneDelta);
        if (! m_axes.testFlag(Qt::Horizontal)) local.setX(0);
        if (! m_axes.testFlag(Qt::Vertical)) local.setY(0);
        return vectorToScene(m_basis, local);
    }
    bool select(const QList<Candidate>& candidates, QPointF sceneDelta, qreal threshold) {
        QPointer<QQuickItem> fallback;
        Qt::Orientations     fallbackAxes;
        for (const auto& candidate : candidates) {
            auto* item = candidate.item.data();
            if (! item) return false;
            auto local = vectorFromScene(item, sceneDelta);
            if (! finite(local)) return false;
            const auto axes      = candidate.axes;
            auto       projected = local;
            if (! axes.testFlag(Qt::Horizontal)) projected.setX(0);
            if (! axes.testFlag(Qt::Vertical)) projected.setY(0);
            const auto distance = vectorToScene(item, projected);
            if (std::hypot(distance.x(), distance.y()) < threshold || projected.isNull()) continue;
            if (! fallback) {
                fallback     = item;
                fallbackAxes = axes;
            }
            const bool horizontal = std::abs(local.x()) >= std::abs(local.y());
            if (axes == (Qt::Horizontal | Qt::Vertical) ||
                (axes.testFlag(Qt::Horizontal) && horizontal) ||
                (axes.testFlag(Qt::Vertical) && ! horizontal)) {
                m_basis = item;
                m_axes  = axes;
                return true;
            }
        }
        if (candidates.size() == 1) {
            m_basis = fallback;
            m_axes  = fallbackAxes;
        }
        return m_basis;
    }

private:
    QPointer<QQuickItem>         m_basis;
    Qt::Orientations             m_axes;
    QList<QPair<ulong, QPointF>> m_samples;
};
} // namespace qml_material
