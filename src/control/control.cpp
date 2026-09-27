#include "qml_material/control/control.hpp"
#include "qml_material/util/qt.hpp"

#include <QEvent>
#include <QFocusEvent>
#include <QGuiApplication>
#include <QHoverEvent>
#include <QStyleHints>

#include <algorithm>

namespace qml_material
{

Control::Control(QQuickItem* parent): QQuickItem(parent) {
    setFocusPolicy(Qt::NoFocus);
    resolveEnvironment();
    m_control_connections.append(
        connect(qGuiApp, &QGuiApplication::fontChanged, this, &Control::resolveFont));

    m_control_connections.append(
        connect(this, &QQuickItem::activeFocusChanged, this, &Control::updateVisualFocus));
    m_control_connections.append(connect(this, &QQuickItem::baselineOffsetChanged, this, [this]() {
        Q_EMIT baselineOffsetChanged();
    }));
    m_control_connections.append(
        connect(qGuiApp, &QGuiApplication::layoutDirectionChanged, this, [this]() {
            if (! m_layout_direction_explicit) resolveLayoutDirection();
        }));
}

Control::~Control() {
    utils::disconnectAll(m_control_connections);
    disconnectBackground();
    disconnectContentItem();
}

QFont Control::font() const {
    auto font = m_font;
    font.setResolveMask(m_font_explicit ? m_requested_font.resolveMask() : 0);
    return font;
}

void Control::setFont(const QFont& value) {
    if (m_font_explicit && m_requested_font == value &&
        m_requested_font.resolveMask() == value.resolveMask())
        return;
    m_requested_font = value;
    m_font_explicit  = true;
    resolveFont();
}

void Control::resetFont() {
    if (! m_font_explicit) return;
    m_font_explicit = false;
    resolveFont();
}

QLocale Control::locale() const { return m_locale; }

void Control::setLocale(const QLocale& value) {
    if (m_locale_explicit && m_requested_locale == value) return;
    m_requested_locale = value;
    m_locale_explicit  = true;
    updateLocale(value);
}

void Control::resetLocale() {
    if (! m_locale_explicit) return;
    m_locale_explicit = false;
    resolveLocale();
}

Qt::LayoutDirection Control::layoutDirection() const { return m_layout_direction; }

void Control::setLayoutDirection(Qt::LayoutDirection value) {
    if (m_layout_direction_explicit && m_requested_layout_direction == value) return;
    m_requested_layout_direction = value;
    m_layout_direction_explicit  = true;
    updateLayoutDirection(value);
}

void Control::resetLayoutDirection() {
    if (! m_layout_direction_explicit) return;
    m_layout_direction_explicit = false;
    resolveLayoutDirection();
}

bool Control::mirrored() const { return m_layout_direction == Qt::RightToLeft; }

qreal Control::availableWidth() const {
    return std::max<qreal>(0, width() - leftPadding() - rightPadding());
}

qreal Control::availableHeight() const {
    return std::max<qreal>(0, height() - topPadding() - bottomPadding());
}

qreal Control::padding() const { return m_padding; }

void Control::setPadding(qreal value) {
    if (qFuzzyCompare(m_padding, value)) return;
    const auto oldState = paddingState();
    m_padding           = value;
    Q_EMIT paddingChanged();
    finishPaddingChange(oldState);
}

void Control::resetPadding() { setPadding(0); }

qreal Control::horizontalPadding() const { return m_horizontal_padding.value_or(m_padding); }

void Control::setHorizontalPadding(qreal value) {
    if (m_horizontal_padding && qFuzzyCompare(*m_horizontal_padding, value)) return;
    const auto oldState  = paddingState();
    m_horizontal_padding = value;
    finishPaddingChange(oldState);
}

void Control::resetHorizontalPadding() {
    if (! m_horizontal_padding) return;
    const auto oldState = paddingState();
    m_horizontal_padding.reset();
    finishPaddingChange(oldState);
}

qreal Control::verticalPadding() const { return m_vertical_padding.value_or(m_padding); }

void Control::setVerticalPadding(qreal value) {
    if (m_vertical_padding && qFuzzyCompare(*m_vertical_padding, value)) return;
    const auto oldState = paddingState();
    m_vertical_padding  = value;
    finishPaddingChange(oldState);
}

void Control::resetVerticalPadding() {
    if (! m_vertical_padding) return;
    const auto oldState = paddingState();
    m_vertical_padding.reset();
    finishPaddingChange(oldState);
}

qreal Control::topPadding() const { return m_top_padding.value_or(verticalPadding()); }

void Control::setTopPadding(qreal value) {
    if (m_top_padding && qFuzzyCompare(*m_top_padding, value)) return;
    const auto oldState = paddingState();
    m_top_padding       = value;
    finishPaddingChange(oldState);
}

void Control::resetTopPadding() {
    if (! m_top_padding) return;
    const auto oldState = paddingState();
    m_top_padding.reset();
    finishPaddingChange(oldState);
}

qreal Control::leftPadding() const { return m_left_padding.value_or(horizontalPadding()); }

void Control::setLeftPadding(qreal value) {
    if (m_left_padding && qFuzzyCompare(*m_left_padding, value)) return;
    const auto oldState = paddingState();
    m_left_padding      = value;
    finishPaddingChange(oldState);
}

void Control::resetLeftPadding() {
    if (! m_left_padding) return;
    const auto oldState = paddingState();
    m_left_padding.reset();
    finishPaddingChange(oldState);
}

qreal Control::rightPadding() const { return m_right_padding.value_or(horizontalPadding()); }

void Control::setRightPadding(qreal value) {
    if (m_right_padding && qFuzzyCompare(*m_right_padding, value)) return;
    const auto oldState = paddingState();
    m_right_padding     = value;
    finishPaddingChange(oldState);
}

void Control::resetRightPadding() {
    if (! m_right_padding) return;
    const auto oldState = paddingState();
    m_right_padding.reset();
    finishPaddingChange(oldState);
}

qreal Control::bottomPadding() const { return m_bottom_padding.value_or(verticalPadding()); }

void Control::setBottomPadding(qreal value) {
    if (m_bottom_padding && qFuzzyCompare(*m_bottom_padding, value)) return;
    const auto oldState = paddingState();
    m_bottom_padding    = value;
    finishPaddingChange(oldState);
}

void Control::resetBottomPadding() {
    if (! m_bottom_padding) return;
    const auto oldState = paddingState();
    m_bottom_padding.reset();
    finishPaddingChange(oldState);
}

qreal Control::spacing() const { return m_spacing; }

void Control::setSpacing(qreal value) {
    if (qFuzzyCompare(m_spacing, value)) return;
    m_spacing = value;
    Q_EMIT spacingChanged();
}

void Control::resetSpacing() { setSpacing(0); }

qreal Control::topInset() const { return m_top_inset; }

void Control::setTopInset(qreal value) {
    if (qFuzzyCompare(m_top_inset, value)) return;
    const auto oldWidth  = implicitLayoutWidth();
    const auto oldHeight = implicitLayoutHeight();
    m_top_inset          = value;
    Q_EMIT topInsetChanged();
    finishInsetChange(oldWidth, oldHeight);
}

void Control::resetTopInset() { setTopInset(0); }

qreal Control::leftInset() const { return m_left_inset; }

void Control::setLeftInset(qreal value) {
    if (qFuzzyCompare(m_left_inset, value)) return;
    const auto oldWidth  = implicitLayoutWidth();
    const auto oldHeight = implicitLayoutHeight();
    m_left_inset         = value;
    Q_EMIT leftInsetChanged();
    finishInsetChange(oldWidth, oldHeight);
}

void Control::resetLeftInset() { setLeftInset(0); }

qreal Control::rightInset() const { return m_right_inset; }

void Control::setRightInset(qreal value) {
    if (qFuzzyCompare(m_right_inset, value)) return;
    const auto oldWidth  = implicitLayoutWidth();
    const auto oldHeight = implicitLayoutHeight();
    m_right_inset        = value;
    Q_EMIT rightInsetChanged();
    finishInsetChange(oldWidth, oldHeight);
}

void Control::resetRightInset() { setRightInset(0); }

qreal Control::bottomInset() const { return m_bottom_inset; }

void Control::setBottomInset(qreal value) {
    if (qFuzzyCompare(m_bottom_inset, value)) return;
    const auto oldWidth  = implicitLayoutWidth();
    const auto oldHeight = implicitLayoutHeight();
    m_bottom_inset       = value;
    Q_EMIT bottomInsetChanged();
    finishInsetChange(oldWidth, oldHeight);
}

void Control::resetBottomInset() { setBottomInset(0); }

QQuickItem* Control::background() const { return m_background; }

void Control::setBackground(QQuickItem* item) {
    const QScopedPropertyUpdateGroup group;
    m_background = item;
}

void Control::backgroundChange() {
    const QScopedPropertyUpdateGroup group;
    const QPointer<Control>          guard(this);
    const QPointer<QQuickItem>       item(background());
    const QPointer<QQuickItem>       oldItem(m_managed_background);
    disconnectBackground();
    m_managed_background = item;
    if (item) {
        m_background_connections.append(connect(
            item, &QQuickItem::implicitWidthChanged, this, &Control::updateImplicitMetrics));
        m_background_connections.append(connect(
            item, &QQuickItem::implicitHeightChanged, this, &Control::updateImplicitMetrics));
        m_background_connections.append(connect(item, &QObject::destroyed, this, [this]() {
            const QScopedPropertyUpdateGroup group;
            m_background.setValueBypassingBindings(nullptr);
            m_background.notify();
        }));
    }
    if (oldItem && oldItem != item && oldItem->parentItem() == this)
        oldItem->setParentItem(nullptr);
    if (! guard || background() != item) return;
    if (item && ! item->parentItem()) item->setParentItem(this);
    if (! guard || background() != item) return;
    orderManagedItems();
    if (! guard || background() != item) return;
    updateImplicitMetrics();
    if (! guard || background() != item) return;
    if (isComponentComplete()) layoutBackground();
    if (guard && background() == item) Q_EMIT backgroundChanged();
}

QQuickItem* Control::contentItem() const { return m_content_item; }

void Control::setContentItem(QQuickItem* item) {
    if (m_content_item == item) return;

    auto* oldItem = m_content_item.data();
    disconnectContentItem();
    m_content_item = nullptr;
    if (oldItem && oldItem->parentItem() == this) oldItem->setParentItem(nullptr);

    m_content_item = item;
    if (item) {
        if (! item->parentItem()) item->setParentItem(this);
        item->installEventFilter(this);
        m_content_connections.append(connect(
            item, &QQuickItem::implicitWidthChanged, this, &Control::updateImplicitMetrics));
        m_content_connections.append(connect(
            item, &QQuickItem::implicitHeightChanged, this, &Control::updateImplicitMetrics));
        m_content_connections.append(connect(
            item, &QQuickItem::baselineOffsetChanged, this, &Control::updateBaselineOffset));
        m_content_connections.append(
            connect(item, &QQuickItem::activeFocusChanged, this, &Control::updateVisualFocus));
        m_content_connections.append(connect(item, &QObject::destroyed, this, [this]() {
            disconnectContentItem();
            m_content_item = nullptr;
            updateImplicitMetrics();
            updateBaselineOffset();
            updateVisualFocus();
            Q_EMIT contentItemChanged();
        }));
    }

    orderManagedItems();
    updateImplicitMetrics();
    updateBaselineOffset();
    updateVisualFocus();
    if (isComponentComplete()) layoutContentItem();
    Q_EMIT contentItemChanged();
}

Control::LayoutFlags Control::backgroundLayout() const { return m_background_layout; }

void Control::setBackgroundLayout(LayoutFlags value) {
    if (m_background_layout == value) return;
    m_background_layout = value;
    if (isComponentComplete()) layoutBackground();
    Q_EMIT backgroundLayoutChanged();
}

void Control::resetBackgroundLayout() { setBackgroundLayout(LayoutAll); }

Control::LayoutFlags Control::contentItemLayout() const { return m_content_item_layout; }

void Control::setContentItemLayout(LayoutFlags value) {
    if (m_content_item_layout == value) return;
    m_content_item_layout = value;
    if (isComponentComplete()) layoutContentItem();
    Q_EMIT contentItemLayoutChanged();
}

void Control::resetContentItemLayout() { setContentItemLayout(LayoutAll); }

qreal Control::implicitBackgroundWidth() const { return m_implicit_background_width; }
qreal Control::implicitBackgroundHeight() const { return m_implicit_background_height; }
qreal Control::implicitContentWidth() const { return m_implicit_content_width; }
qreal Control::implicitContentHeight() const { return m_implicit_content_height; }

qreal Control::implicitLayoutWidth() const {
    return std::max({ qreal(0),
                      m_implicit_background_width + leftInset() + rightInset(),
                      m_implicit_content_width + leftPadding() + rightPadding(),
                      m_extra_implicit_width });
}

qreal Control::implicitLayoutHeight() const {
    return std::max({ qreal(0),
                      m_implicit_background_height + topInset() + bottomInset(),
                      m_implicit_content_height + topPadding() + bottomPadding(),
                      m_extra_implicit_height });
}

qreal Control::extraImplicitWidth() const { return m_extra_implicit_width; }

void Control::setExtraImplicitWidth(qreal value) {
    if (qFuzzyCompare(m_extra_implicit_width, value)) return;
    const auto oldWidth    = implicitLayoutWidth();
    m_extra_implicit_width = value;
    Q_EMIT extraImplicitWidthChanged();
    if (! qFuzzyCompare(oldWidth, implicitLayoutWidth())) Q_EMIT implicitLayoutWidthChanged();
}

void Control::resetExtraImplicitWidth() { setExtraImplicitWidth(0); }

qreal Control::extraImplicitHeight() const { return m_extra_implicit_height; }

void Control::setExtraImplicitHeight(qreal value) {
    if (qFuzzyCompare(m_extra_implicit_height, value)) return;
    const auto oldHeight    = implicitLayoutHeight();
    m_extra_implicit_height = value;
    Q_EMIT extraImplicitHeightChanged();
    if (! qFuzzyCompare(oldHeight, implicitLayoutHeight())) Q_EMIT implicitLayoutHeightChanged();
}

void Control::resetExtraImplicitHeight() { setExtraImplicitHeight(0); }

qreal Control::baselineOffset() const { return QQuickItem::baselineOffset(); }

void Control::setBaselineOffset(qreal value) {
    m_baseline_explicit = true;
    QQuickItem::setBaselineOffset(value);
}

void Control::resetBaselineOffset() {
    if (! m_baseline_explicit) return;
    m_baseline_explicit = false;
    updateBaselineOffset();
}

bool Control::hoverEnabled() const { return m_hover_enabled; }

void Control::setHoverEnabled(bool value) {
    if (m_requested_hover_enabled && *m_requested_hover_enabled == value) return;
    m_requested_hover_enabled = value;
    updateHoverEnabled(value);
}

void Control::resetHoverEnabled() {
    if (! m_requested_hover_enabled) return;
    m_requested_hover_enabled.reset();
    resolveHoverEnabled();
}

bool Control::hovered() const { return m_hovered; }

Qt::FocusReason Control::focusReason() const { return m_focus_reason; }

void Control::setFocusReason(Qt::FocusReason value) {
    if (m_focus_reason == value) return;
    m_focus_reason = value;
    Q_EMIT focusReasonChanged();
    updateVisualFocus();
}

bool Control::visualFocus() const { return m_visual_focus; }

void Control::componentComplete() {
    QQuickItem::componentComplete();
    resolveEnvironment();
    updateImplicitMetrics();
    layoutItems();
    updateBaselineOffset();
}

void Control::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) {
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    layoutItems();
    if (! qFuzzyCompare(newGeometry.width(), oldGeometry.width())) Q_EMIT availableWidthChanged();
    if (! qFuzzyCompare(newGeometry.height(), oldGeometry.height()))
        Q_EMIT availableHeightChanged();
}

void Control::itemChange(ItemChange change, const ItemChangeData& value) {
    QQuickItem::itemChange(change, value);
    switch (change) {
    case ItemParentHasChanged:
    case ItemSceneChange:
        if ((change == ItemParentHasChanged && value.item) ||
            (change == ItemSceneChange && value.window))
            resolveEnvironment();
        break;
    case ItemVisibleHasChanged:
        if (! value.boolValue) setHovered(false);
        break;
    case ItemEnabledHasChanged:
        if (! value.boolValue) setHovered(false);
        break;
    case ItemActiveFocusHasChanged: updateVisualFocus(); break;
    default: break;
    }
}

void Control::focusInEvent(QFocusEvent* event) {
    setFocusReason(event->reason());
    QQuickItem::focusInEvent(event);
    updateVisualFocus();
}

void Control::focusOutEvent(QFocusEvent* event) {
    setFocusReason(event->reason());
    QQuickItem::focusOutEvent(event);
    updateVisualFocus();
}

void Control::hoverEnterEvent(QHoverEvent* event) {
    setHovered(m_hover_enabled);
    event->ignore();
}

void Control::hoverMoveEvent(QHoverEvent* event) {
    setHovered(m_hover_enabled && contains(event->position()));
    event->ignore();
}

void Control::hoverLeaveEvent(QHoverEvent* event) {
    setHovered(false);
    event->ignore();
}

bool Control::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_content_item) {
        if (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut) {
            setFocusReason(static_cast<QFocusEvent*>(event)->reason());
            updateVisualFocus();
        }
    }
    return QQuickItem::eventFilter(watched, event);
}

Control::PaddingState Control::paddingState() const {
    return { topPadding(),          leftPadding(),         rightPadding(),   bottomPadding(),
             horizontalPadding(),   verticalPadding(),     availableWidth(), availableHeight(),
             implicitLayoutWidth(), implicitLayoutHeight() };
}

void Control::finishPaddingChange(const PaddingState& oldState) {
    const auto newState = paddingState();

    if (! qFuzzyCompare(oldState.horizontal, newState.horizontal))
        Q_EMIT horizontalPaddingChanged();
    if (! qFuzzyCompare(oldState.vertical, newState.vertical)) Q_EMIT verticalPaddingChanged();
    if (! qFuzzyCompare(oldState.top, newState.top)) Q_EMIT topPaddingChanged();
    if (! qFuzzyCompare(oldState.left, newState.left)) Q_EMIT leftPaddingChanged();
    if (! qFuzzyCompare(oldState.right, newState.right)) Q_EMIT rightPaddingChanged();
    if (! qFuzzyCompare(oldState.bottom, newState.bottom)) Q_EMIT bottomPaddingChanged();
    if (! qFuzzyCompare(oldState.availableWidth, newState.availableWidth))
        Q_EMIT availableWidthChanged();
    if (! qFuzzyCompare(oldState.availableHeight, newState.availableHeight))
        Q_EMIT availableHeightChanged();
    if (! qFuzzyCompare(oldState.implicitWidth, newState.implicitWidth))
        Q_EMIT implicitLayoutWidthChanged();
    if (! qFuzzyCompare(oldState.implicitHeight, newState.implicitHeight))
        Q_EMIT implicitLayoutHeightChanged();

    if (isComponentComplete()) layoutContentItem();
    updateBaselineOffset();
}

void Control::finishInsetChange(qreal oldImplicitWidth, qreal oldImplicitHeight) {
    if (! qFuzzyCompare(oldImplicitWidth, implicitLayoutWidth()))
        Q_EMIT implicitLayoutWidthChanged();
    if (! qFuzzyCompare(oldImplicitHeight, implicitLayoutHeight()))
        Q_EMIT implicitLayoutHeightChanged();
    if (isComponentComplete()) layoutBackground();
}

QSizeF Control::measureImplicitContent() const {
    return m_content_item
               ? QSizeF(m_content_item->implicitWidth(), m_content_item->implicitHeight())
               : QSizeF(0, 0);
}

void Control::updateImplicitMetrics() {
    const auto oldLayoutWidth   = implicitLayoutWidth();
    const auto oldLayoutHeight  = implicitLayoutHeight();
    const auto backgroundWidth  = m_background ? m_background->implicitWidth() : 0;
    const auto backgroundHeight = m_background ? m_background->implicitHeight() : 0;
    const auto contentSize      = measureImplicitContent();
    const auto contentWidth     = contentSize.width();
    const auto contentHeight    = contentSize.height();

    if (! qFuzzyCompare(m_implicit_background_width, backgroundWidth)) {
        m_implicit_background_width = backgroundWidth;
        Q_EMIT implicitBackgroundWidthChanged();
    }
    if (! qFuzzyCompare(m_implicit_background_height, backgroundHeight)) {
        m_implicit_background_height = backgroundHeight;
        Q_EMIT implicitBackgroundHeightChanged();
    }
    if (! qFuzzyCompare(m_implicit_content_width, contentWidth)) {
        m_implicit_content_width = contentWidth;
        Q_EMIT implicitContentWidthChanged();
    }
    if (! qFuzzyCompare(m_implicit_content_height, contentHeight)) {
        m_implicit_content_height = contentHeight;
        Q_EMIT implicitContentHeightChanged();
    }

    if (! qFuzzyCompare(oldLayoutWidth, implicitLayoutWidth())) Q_EMIT implicitLayoutWidthChanged();
    if (! qFuzzyCompare(oldLayoutHeight, implicitLayoutHeight()))
        Q_EMIT implicitLayoutHeightChanged();
}

void Control::updateBaselineOffset() {
    if (m_baseline_explicit) return;
    const auto value = m_content_item ? topPadding() + m_content_item->baselineOffset() : 0;
    QQuickItem::setBaselineOffset(value);
}

void Control::resolveEnvironment() {
    QPointer<Control> guard(this);
    resolveFont();
    if (! guard) return;
    resolveLocale();
    if (! guard) return;
    resolveLayoutDirection();
    if (! guard) return;
    resolveHoverEnabled();
}

void Control::resolveFont() {
    if (! utils::canUpdateControlEnvironment()) return;
    inheritFont(utils::inheritedFont(this));
}

void Control::inheritFont(const QFont& inherited) {
    if (! utils::canUpdateControlEnvironment()) return;
    const auto resolved =
        m_font_explicit ? utils::resolveFont(m_requested_font, inherited) : inherited;
    const bool changed = m_font != resolved;
    if (! changed && m_font.resolveMask() == resolved.resolveMask()) return;
    m_font = resolved;
    QPointer<Control> guard(this);
    utils::propagateFont(this, resolved);
    if (guard && changed) Q_EMIT fontChanged();
}

void Control::resolveLocale() {
    if (! utils::canUpdateControlEnvironment() || m_locale_explicit) return;
    inheritLocale(utils::inheritedLocale(this));
}

void Control::inheritLocale(const QLocale& value) {
    if (! m_locale_explicit) updateLocale(value);
}

void Control::updateLocale(const QLocale& value) {
    if (! utils::canUpdateControlEnvironment() || m_locale == value) return;
    const auto resolved = value;
    m_locale            = resolved;
    QPointer<Control> guard(this);
    utils::propagateLocale(this, resolved);
    if (guard) Q_EMIT localeChanged();
}

void Control::resolveLayoutDirection() {
    if (! utils::canUpdateControlEnvironment() || m_layout_direction_explicit) return;
    inheritLayoutDirection(utils::inheritedLayoutDirection(this));
}

void Control::inheritLayoutDirection(Qt::LayoutDirection value) {
    if (! m_layout_direction_explicit) updateLayoutDirection(value);
}

void Control::updateLayoutDirection(Qt::LayoutDirection value) {
    if (! utils::canUpdateControlEnvironment() || m_layout_direction == value) return;
    const auto wasMirrored = mirrored();
    m_layout_direction     = value;
    QPointer<Control> guard(this);
    utils::propagateLayoutDirection(this, value);
    if (! guard) return;
    Q_EMIT layoutDirectionChanged();
    if (guard && wasMirrored != mirrored()) Q_EMIT mirroredChanged();
}

void Control::resolveHoverEnabled() {
    if (! utils::canUpdateControlEnvironment() || m_requested_hover_enabled) return;
    inheritHoverEnabled(utils::inheritedHoverEnabled(this));
}

void Control::inheritHoverEnabled(bool value) {
    if (! m_requested_hover_enabled) updateHoverEnabled(value);
}

void Control::updateHoverEnabled(bool value) {
    if (! utils::canUpdateControlEnvironment() || m_hover_enabled == value) return;
    m_hover_enabled = value;
    setAcceptHoverEvents(value);
    QPointer<Control> guard(this);
    if (! value) setHovered(false);
    if (! guard) return;
    utils::propagateHoverEnabled(this, value);
    if (guard) Q_EMIT hoverEnabledChanged();
}

void Control::updateVisualFocus() {
    const bool hasFocus = hasActiveFocus() || (m_content_item && m_content_item->hasActiveFocus());
    const bool value    = hasFocus && utils::isKeyboardFocusReason(m_focus_reason);
    if (m_visual_focus == value) return;
    m_visual_focus = value;
}

void Control::setHovered(bool value) {
    if (m_hovered == value) return;
    m_hovered = value;
}

void Control::layoutItems() {
    if (! isComponentComplete()) return;
    layoutBackground();
    layoutContentItem();
}

void Control::layoutBackground() {
    if (! m_background) return;
    if (m_background_layout.testFlag(LayoutX)) m_background->setX(leftInset());
    if (m_background_layout.testFlag(LayoutY)) m_background->setY(topInset());
    if (m_background_layout.testFlag(LayoutWidth))
        m_background->setWidth(std::max<qreal>(0, width() - leftInset() - rightInset()));
    if (m_background_layout.testFlag(LayoutHeight))
        m_background->setHeight(std::max<qreal>(0, height() - topInset() - bottomInset()));
}

QRectF Control::contentRect() const {
    return QRectF(leftPadding(), topPadding(), availableWidth(), availableHeight());
}

void Control::layoutContentItem() {
    if (! m_content_item) return;
    const auto rect = contentRect();
    if (m_content_item_layout.testFlag(LayoutX)) m_content_item->setX(rect.x());
    if (m_content_item_layout.testFlag(LayoutY)) m_content_item->setY(rect.y());
    if (m_content_item_layout.testFlag(LayoutWidth)) m_content_item->setWidth(rect.width());
    if (m_content_item_layout.testFlag(LayoutHeight)) m_content_item->setHeight(rect.height());
}

void Control::orderManagedItems() {
    if (! m_background || m_background->parentItem() != this) return;
    if (qFuzzyIsNull(m_background->z())) m_background->setZ(-1);
    if (m_content_item && m_content_item->parentItem() == this)
        m_background->stackBefore(m_content_item);
}

void Control::disconnectBackground() { utils::disconnectAll(m_background_connections); }

void Control::disconnectContentItem() {
    if (m_content_item) m_content_item->removeEventFilter(this);
    utils::disconnectAll(m_content_connections);
}

} // namespace qml_material
