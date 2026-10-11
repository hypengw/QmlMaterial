#pragma once

#include "qml_material/control/popup.hpp"
#include <algorithm>
#include <cmath>
#include <optional>

namespace qml_material
{
struct PopupPlacement {
    QRectF                      rect;
    Popup::Placement            placement;
    QQuickItem::TransformOrigin origin;
};

inline std::optional<PopupPlacement> placePopup(const QRectF& anchor, const QSizeF& request,
                                                const QRectF& bounds, Popup::Placement preferred,
                                                Popup::AnchorAlignment alignment, qreal gap,
                                                bool mirrored) {
    for (const auto value : { anchor.x(),
                              anchor.y(),
                              anchor.width(),
                              anchor.height(),
                              request.width(),
                              request.height(),
                              bounds.x(),
                              bounds.y(),
                              bounds.width(),
                              bounds.height(),
                              gap })
        if (! std::isfinite(value)) return {};
    if (bounds.isEmpty() || request.isEmpty() || anchor.width() < 0 || anchor.height() < 0)
        return {};
    gap                 = std::max<qreal>(0, gap);
    const bool vertical = preferred == Popup::Below || preferred == Popup::Above;
    auto       opposite = [](Popup::Placement p) {
        switch (p) {
        case Popup::Below: return Popup::Above;
        case Popup::Above: return Popup::Below;
        case Popup::Leading: return Popup::Trailing;
        case Popup::Trailing: return Popup::Leading;
        }
        return Popup::Below;
    };
    auto leftSide = [mirrored](Popup::Placement p) {
        return (p == Popup::Leading) != mirrored;
    };
    auto space = [&](Popup::Placement p) {
        qreal available;
        if (p == Popup::Below)
            available = bounds.bottom() - anchor.bottom() - gap;
        else if (p == Popup::Above)
            available = anchor.top() - gap - bounds.top();
        else if (leftSide(p))
            available = anchor.left() - gap - bounds.left();
        else
            available = bounds.right() - anchor.right() - gap;
        return std::clamp(available, qreal(0), vertical ? bounds.height() : bounds.width());
    };
    auto       resolved = preferred;
    const auto other    = opposite(preferred);
    const auto extent   = vertical ? request.height() : request.width();
    if (extent > space(preferred) && space(other) > space(preferred)) resolved = other;
    if (space(resolved) <= 0) return {};
    QSizeF size(std::min(request.width(), bounds.width()),
                std::min(request.height(), bounds.height()));
    if (vertical)
        size.setHeight(std::min(size.height(), space(resolved)));
    else
        size.setWidth(std::min(size.width(), space(resolved)));
    const qreal low   = vertical ? bounds.left() : bounds.top();
    const qreal high  = vertical ? bounds.right() : bounds.bottom();
    const qreal cross = vertical ? size.width() : size.height();
    const qreal start = vertical ? anchor.left() : anchor.top();
    const qreal end   = vertical ? anchor.right() : anchor.bottom();
    auto        align = [&](Popup::AnchorAlignment a) {
        if (a == Popup::Center) return (start + end - cross) / 2;
        const bool atEnd = (a == Popup::End) != (vertical && mirrored);
        return atEnd ? end - cross : start;
    };
    qreal offset = align(alignment);
    if (alignment != Popup::Center && (offset < low || offset + cross > high)) {
        const auto alternate = align(alignment == Popup::Start ? Popup::End : Popup::Start);
        if (alternate >= low && alternate + cross <= high) offset = alternate;
    }
    offset = std::clamp(offset, low, high - cross);
    QPointF point;
    if (vertical)
        point = { offset,
                  resolved == Popup::Below ? anchor.bottom() + gap
                                           : anchor.top() - gap - size.height() };
    else
        point = { leftSide(resolved) ? anchor.left() - gap - size.width() : anchor.right() + gap,
                  offset };
    point.setX(std::clamp(point.x(), bounds.left(), bounds.right() - size.width()));
    point.setY(std::clamp(point.y(), bounds.top(), bounds.bottom() - size.height()));
    const qreal center      = vertical ? anchor.center().x() : anchor.center().y();
    const int   crossOrigin = std::clamp(qRound(2 * (center - offset) / cross), 0, 2);
    QQuickItem::TransformOrigin origin;
    if (vertical) {
        const QQuickItem::TransformOrigin top[]    = { QQuickItem::TopLeft,
                                                       QQuickItem::Top,
                                                       QQuickItem::TopRight };
        const QQuickItem::TransformOrigin bottom[] = { QQuickItem::BottomLeft,
                                                       QQuickItem::Bottom,
                                                       QQuickItem::BottomRight };
        origin = resolved == Popup::Below ? top[crossOrigin] : bottom[crossOrigin];
    } else {
        const QQuickItem::TransformOrigin left[]  = { QQuickItem::TopLeft,
                                                      QQuickItem::Left,
                                                      QQuickItem::BottomLeft };
        const QQuickItem::TransformOrigin right[] = { QQuickItem::TopRight,
                                                      QQuickItem::Right,
                                                      QQuickItem::BottomRight };
        origin = leftSide(resolved) ? right[crossOrigin] : left[crossOrigin];
    }
    return PopupPlacement { QRectF(point, size), resolved, origin };
}
} // namespace qml_material
