#pragma once

#include "qml_material/anim/shimmer.hpp"
#include <QQuickItem>
#include <QQuickWindow>
#include <QTransform>
#include <QVector3D>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace qml_material::shimmer
{
inline bool validSize(QSizeF size) {
    const auto limit = std::numeric_limits<float>::max() / 16;
    return std::isfinite(size.width()) && std::isfinite(size.height()) && size.width() > 0 &&
           size.height() > 0 && size.width() < limit && size.height() < limit;
}
struct Area {
    QVector3D projection;
    float     travel                        = 1;
    bool      valid                         = false;
    bool      operator==(const Area&) const = default;
    float     at(QPointF p) const {
        return projection.x() * p.x() + projection.y() * p.y() + projection.z();
    }
};
struct Frame {
    float phase = 0, minimum = 1, maximum = 1;
    bool  operator==(const Frame&) const = default;
};
inline Area project(QSizeF size, const QTransform& transform, QRectF region, qreal angle,
                    qreal bandWidth) {
    if (! validSize(size) || region.isEmpty() || ! transform.isAffine() ||
        ! transform.isInvertible() || ! std::isfinite(bandWidth) || bandWidth <= 0 ||
        ! std::isfinite(angle))
        return {};
    const auto    theta = angle * std::numbers::pi / 180;
    const QPointF direction(std::cos(theta), std::sin(theta));
    const auto    scalar = [&](QPointF p) {
        return QPointF::dotProduct(p, direction) / bandWidth;
    };
    const qreal values[] = { scalar({ transform.m11(), transform.m12() }),
                             scalar({ transform.m21(), transform.m22() }),
                             scalar(QPointF(transform.dx(), transform.dy()) - region.center()),
                             (std::abs(region.width() * direction.x()) +
                              std::abs(region.height() * direction.y())) /
                                     bandWidth +
                                 1 };
    for (const auto value : values)
        if (! std::isfinite(value) || std::abs(value) > std::numeric_limits<float>::max() / 16)
            return {};
    if (std::abs(values[0]) * size.width() + std::abs(values[1]) * size.height() +
            std::abs(values[2]) >
        std::numeric_limits<float>::max() / 16)
        return {};
    return { { float(values[0]), float(values[1]), float(values[2]) }, float(values[3]), true };
}
inline Area areaFor(QQuickItem* item, const Shimmer* group) {
    if (! group) return {};
    QRectF     region(QPointF(), item->size());
    QTransform transform;
    if (group->boundsMode() != Shimmer::View) {
        auto* window    = item->window();
        auto* reference = group->boundsMode() == Shimmer::Window
                              ? (window ? window->contentItem() : nullptr)
                              : group->boundsItem();
        if (! window || ! reference || reference->window() != window) return {};
        bool ok   = false;
        transform = item->itemTransform(reference, &ok);
        if (! ok) return {};
        region = group->boundsMode() == Shimmer::Window
                     ? QRectF(0, 0, window->width(), window->height())
                     : reference->boundingRect();
    }
    return project(item->size(), transform, region, group->angle(), group->bandWidth());
}
inline float mask(float projection, float travel, const Frame& frame) {
    const auto t = std::clamp(projection - (frame.phase - .5f) * travel + .5f, 0.f, 1.f);
    return frame.minimum + (frame.maximum - frame.minimum) * (1 - std::abs(2 * t - 1));
}
} // namespace qml_material::shimmer
