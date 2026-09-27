#include "qml_material/control/button_group_container.hpp"
#include "qml_material/control/abstract_button.hpp"
#include "qml_material/util/qt.hpp"
#include "qml_material/token/duration.hpp"
#include <QtQuick/private/qquickitem_p.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace qml_material
{
namespace
{
ButtonGroupContainerAttached* attached(QQuickItem* item) {
    return qobject_cast<ButtonGroupContainerAttached*>(
        qmlAttachedPropertiesObject<ButtonGroupContainer>(item, true));
}
bool participates(QQuickItem* item) {
    return item && QQuickItemPrivate::get(item)->explicitVisible;
}
} // namespace

ButtonGroupContainer::ButtonGroupContainer(QQuickItem* parent): MaterialContainer(parent) {
    connect(this, &Control::spacingChanged, this, &QQuickItem::polish);
    connect(this, &Control::availableWidthChanged, this, &QQuickItem::polish);
    connect(this, &Control::mirroredChanged, this, &QQuickItem::polish);
}
ButtonGroupContainer::~ButtonGroupContainer() { beginTeardown(); }
ButtonGroupContainerAttached* ButtonGroupContainer::qmlAttachedProperties(QObject* object) {
    return new ButtonGroupContainerAttached(object);
}
qreal ButtonGroupContainer::defaultSpacing() const {
    return m_variant == Connected ? token::ButtonGroup::connectedSpacing
                                  : token::ButtonGroup::standardSpacing;
}
void ButtonGroupContainer::setVariant(Variant value) {
    if ((value != Standard && value != Connected) || m_variant == value) return;
    const auto oldSpacing   = spacing();
    const auto oldAnimation = animateWidth();
    m_variant               = value;
    polish();
    QPointer<ButtonGroupContainer> guard(this);
    Q_EMIT variantChanged();
    if (! guard) return;
    if (oldSpacing != spacing()) Q_EMIT spacingChanged();
    if (! guard) return;
    if (oldAnimation != animateWidth()) Q_EMIT animateWidthChanged();
}
void ButtonGroupContainer::setAnimateWidth(bool value) {
    const auto old = animateWidth();
    m_animateWidth = value;
    if (old != animateWidth()) Q_EMIT animateWidthChanged();
}
void ButtonGroupContainer::resetAnimateWidth() {
    const auto old = animateWidth();
    m_animateWidth.reset();
    if (old != animateWidth()) Q_EMIT animateWidthChanged();
}
void ButtonGroupContainer::setExpandedRatio(qreal value) {
    if (! std::isfinite(value) || value < 0 || value == m_expandedRatio) return;
    m_expandedRatio = value;
    polish();
    Q_EMIT expandedRatioChanged();
}
void ButtonGroupContainer::setButtonSize(int value) {
    if (value < int(Enum::ButtonSize::XS) || value > int(Enum::ButtonSize::XL) || m_size == value)
        return;
    m_size = value;
    Q_EMIT sizeChanged();
}
bool ButtonGroupContainer::isContent(QQuickItem* item) const {
    return qobject_cast<AbstractButton*>(item);
}
void ButtonGroupContainer::itemAdded(QQuickItem* item) {
    connect(attached(item),
            &ButtonGroupContainerAttached::layoutChanged,
            this,
            &QQuickItem::polish,
            Qt::UniqueConnection);
}
void ButtonGroupContainer::itemRemoved(QQuickItem* item) {
    auto* info = attached(item);
    disconnect(info, &ButtonGroupContainerAttached::layoutChanged, this, &QQuickItem::polish);
    QPointer<ButtonGroupContainerAttached> guard(info);
    info->stopAnimation();
    if (guard) info->restoreWidth();
    if (guard) info->setPosition(int(Enum::ItemPosition::PosSingle));
}
void ButtonGroupContainer::updatePolish() {
    QPointer<ButtonGroupContainer> guard(this);
    Container::updatePolish();
    if (! guard) return;
    const auto                  version  = revision();
    const auto                  snapshot = items();
    QList<QPointer<QQuickItem>> visible;
    for (auto item : snapshot) {
        if (participates(item))
            visible.append(item);
        else if (item)
            attached(item)->setPosition(int(Enum::ItemPosition::PosSingle));
        if (! guard) return;
        if (revision() != version) {
            polish();
            return;
        }
    }
    const int    n = visible.size();
    QList<qreal> base(n), minimum(n), weights(n), budgets(n), progress(n);
    QList<bool>  boundWidth(n);
    qreal        natural = std::max(0, n - 1) * spacing(), height = 0, weightSum = 0, fixed = 0;
    for (int i = 0; i < n; ++i) {
        auto item = visible[i];
        if (! item) {
            polish();
            return;
        }
        auto* info = attached(item);
        info->setPosition(int(n == 1       ? Enum::ItemPosition::PosSingle
                              : i == 0     ? Enum::ItemPosition::PosFirst
                              : i == n - 1 ? Enum::ItemPosition::PosLast
                                           : Enum::ItemPosition::PosMiddle));
        if (! guard) return;
        if (! item || revision() != version) {
            polish();
            return;
        }
        minimum[i] = info->minimumWidth();
        // setWidth removes bindings; application-bound widths opt out of redistribution.
        boundWidth[i] = QQuickItemPrivate::get(item)->width.hasBinding();
        base[i] =
            std::max(minimum[i],
                     info->preferredWidth() >= 0 ? info->preferredWidth() : item->implicitWidth());
        if (boundWidth[i]) base[i] = item->width();
        weights[i] = boundWidth[i] ? 0 : info->weight();
        weightSum += weights[i];
        if (weights[i] == 0) fixed += base[i];
        natural += base[i];
        height      = std::max(height, item->height());
        budgets[i]  = boundWidth[i] ? 0 : info->compressionLimit();
        progress[i] = animateWidth() && ! boundWidth[i] ? info->pressProgress() : 0;
    }
    setImplicitContentSize(QSizeF(std::max(qreal(0), natural), height));
    if (! guard) return;
    if (revision() != version) {
        polish();
        return;
    }
    // Weighted items share remaining space, preserving each minimum before distributing again.
    if (weightSum > 0 && QQuickItemPrivate::get(this)->widthValid()) {
        qreal remaining =
            std::max(qreal(0), availableWidth() - fixed - std::max(0, n - 1) * spacing());
        QList<bool> active(n, true);
        for (int pass = 0; pass < n && weightSum > 0; ++pass) {
            bool clamped = false;
            for (int i = 0; i < n; ++i) {
                if (weights[i] <= 0 || ! active[i]) continue;
                if (remaining * weights[i] / weightSum < minimum[i]) {
                    base[i]   = minimum[i];
                    remaining = std::max(qreal(0), remaining - minimum[i]);
                    weightSum -= weights[i];
                    active[i] = false;
                    clamped   = true;
                }
            }
            if (! clamped) break;
        }
        for (int i = 0; i < n; ++i)
            if (weights[i] > 0 && active[i] && weightSum > 0)
                base[i] = remaining * weights[i] / weightSum;
    }
    for (int i = 0; i < n; ++i)
        budgets[i] = std::min(budgets[i], std::max(qreal(0), base[i] - minimum[i]));
    // Resolve all requests against the same baseline; simultaneous presses share donor budgets.
    QList<qreal> request(n, 0), donated(n, 0), result = base;
    for (int i = 0; i < n && n > 1; ++i) {
        const int   neighbors = (i > 0) + (i + 1 < n);
        const qreal amount =
            progress[i] *
            std::min(expandedRatio() * base[i] / neighbors,
                     std::min(i > 0 ? budgets[i - 1] : std::numeric_limits<qreal>::max(),
                              i + 1 < n ? budgets[i + 1] : std::numeric_limits<qreal>::max()));
        request[i] = amount;
        if (i > 0) donated[i - 1] += amount;
        if (i + 1 < n) donated[i + 1] += amount;
    }
    for (int i = 0; i < n; ++i) {
        for (int j : { i - 1, i + 1 }) {
            if (j < 0 || j >= n) continue;
            const qreal amount =
                request[i] * (donated[j] > 0 ? std::min(qreal(1), budgets[j] / donated[j]) : 0);
            result[i] += amount;
            result[j] -= amount;
        }
    }
    const qreal total =
        std::accumulate(base.begin(), base.end(), qreal(0)) + std::max(0, n - 1) * spacing();
    qreal x = mirrored() ? total : 0;
    for (int i = 0; i < n; ++i) {
        auto item = visible[i];
        if (! item) {
            polish();
            return;
        }
        if (mirrored()) x -= result[i];
        if (! boundWidth[i]) {
            auto* data = attached(item);
            if (! data->m_managedWidth) {
                data->m_originalWidthValid = QQuickItemPrivate::get(item)->widthValid();
                data->m_originalWidth      = item->width();
                data->m_managedWidth       = true;
            }
            item->setWidth(result[i]);
        }
        if (! guard) return;
        if (! item || revision() != version) {
            polish();
            return;
        }
        item->setPosition(QPointF(x, (height - item->height()) / 2));
        if (! guard) return;
        if (revision() != version) {
            polish();
            return;
        }
        x += mirrored() ? -spacing() : result[i] + spacing();
    }
}

ButtonGroupContainerAttached::ButtonGroupContainerAttached(QObject* object)
    : QObject(object), m_animation(this) {
    m_animation.setDuration(int(token::Duration {}.short4));
    m_animation.setEasingCurve(QEasingCurve::OutCubic);
    connect(&m_animation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        m_progress = value.toReal();
        Q_EMIT layoutChanged();
    });
    connect(&m_animation, &QVariantAnimation::finished, this, [this] {
        if (m_releasing && m_progress > 0) animateTo(0);
    });
    auto* item = qobject_cast<AbstractButton*>(object);
    if (! item) return;
    auto* membership =
        qobject_cast<ContainerAttached*>(qmlAttachedPropertiesObject<Container>(item, true));
    connect(membership,
            &ContainerAttached::containerChanged,
            this,
            &ButtonGroupContainerAttached::membershipChanged);
    connect(item, &AbstractButton::downChanged, this, &ButtonGroupContainerAttached::updatePress);
    connect(item, &AbstractButton::canceled, this, &ButtonGroupContainerAttached::stopAnimation);
    connect(item, &QQuickItem::enabledChanged, this, &ButtonGroupContainerAttached::updatePress);
    connect(item, &QQuickItem::visibleChanged, this, &ButtonGroupContainerAttached::updatePress);
    membershipChanged();
}
ButtonGroupContainerAttached::~ButtonGroupContainerAttached() {
    utils::disconnectAll(m_ownerConnections);
}
void ButtonGroupContainerAttached::membershipChanged() {
    utils::disconnectAll(m_ownerConnections);
    if (auto* group = owner()) {
        m_ownerConnections.append(connect(group,
                                          &ButtonGroupContainer::variantChanged,
                                          this,
                                          &ButtonGroupContainerAttached::contextChanged));
        m_ownerConnections.append(connect(group,
                                          &ButtonGroupContainer::sizeChanged,
                                          this,
                                          &ButtonGroupContainerAttached::contextChanged));
        m_ownerConnections.append(connect(group,
                                          &ButtonGroupContainer::animateWidthChanged,
                                          this,
                                          &ButtonGroupContainerAttached::updatePress));
    }
    QPointer<ButtonGroupContainerAttached> guard(this);
    stopAnimation();
    if (! guard) return;
    restoreWidth();
    if (! guard) return;
    Q_EMIT contextChanged();
    if (guard) updatePress();
}
ButtonGroupContainer* ButtonGroupContainerAttached::owner() const {
    auto* membership =
        qobject_cast<ContainerAttached*>(qmlAttachedPropertiesObject<Container>(parent(), false));
    return membership ? qobject_cast<ButtonGroupContainer*>(membership->container()) : nullptr;
}
bool ButtonGroupContainerAttached::connected() const {
    return owner() && owner()->variant() == ButtonGroupContainer::Connected;
}
int ButtonGroupContainerAttached::buttonSize() const {
    return owner() ? owner()->buttonSize() : int(Enum::ButtonSize::S);
}
void ButtonGroupContainerAttached::setPosition(int value) {
    if (m_position == value) return;
    m_position = value;
    Q_EMIT contextChanged();
}
void ButtonGroupContainerAttached::animateTo(qreal target) {
    m_animation.stop();
    if (m_progress == target) return;
    m_animation.setStartValue(m_progress);
    m_animation.setEndValue(target);
    m_animation.start();
}
void ButtonGroupContainerAttached::stopAnimation() {
    m_animation.stop();
    m_releasing = false;
    if (m_progress == 0) return;
    m_progress = 0;
    Q_EMIT layoutChanged();
}
void ButtonGroupContainerAttached::restoreWidth() {
    if (! m_managedWidth) return;
    m_managedWidth = false;
    auto* item     = qobject_cast<QQuickItem*>(parent());
    if (! item || QQuickItemPrivate::get(item)->width.hasBinding()) return;
    if (m_originalWidthValid)
        item->setWidth(m_originalWidth);
    else
        item->resetWidth();
}
void ButtonGroupContainerAttached::updatePress() {
    const auto* item = qobject_cast<AbstractButton*>(parent());
    if (! item || ! owner() || ! owner()->animateWidth() || ! item->isEnabled() ||
        ! item->isVisible()) {
        stopAnimation();
        return;
    }
    m_releasing = ! item->isDown();
    if (! m_releasing)
        animateTo(1);
    else if (m_progress >= .75 || m_animation.state() != QAbstractAnimation::Running)
        animateTo(0);
}
#define VALUE(Upper, Member)                                                \
    void ButtonGroupContainerAttached::set##Upper(qreal value) {            \
        if (! std::isfinite(value) || value < 0 || Member == value) return; \
        Member = value;                                                     \
        Q_EMIT layoutChanged();                                             \
    }
VALUE(PreferredWidth, m_preferredWidth)
VALUE(MinimumWidth, m_minimumWidth)
VALUE(DefaultMinimumWidth, m_defaultMinimumWidth)
VALUE(Weight, m_weight)
VALUE(CompressionLimit, m_compressionLimit)
VALUE(DefaultCompressionLimit, m_defaultCompressionLimit)
#undef VALUE
void ButtonGroupContainerAttached::resetPreferredWidth() {
    m_preferredWidth.reset();
    Q_EMIT layoutChanged();
}
void ButtonGroupContainerAttached::resetMinimumWidth() {
    m_minimumWidth.reset();
    Q_EMIT layoutChanged();
}
void ButtonGroupContainerAttached::resetCompressionLimit() {
    m_compressionLimit.reset();
    Q_EMIT layoutChanged();
}
} // namespace qml_material
