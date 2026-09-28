#include "qml_material/control/fab_menu.hpp"
#include "qml_material/scrollable/flickable.hpp"
#include "qml_material/token/fab_menu.hpp"
#include "qml_material/util/qt.hpp"
#include <QKeyEvent>
#include <QHoverEvent>
#include <QQuickWindow>
#include <QtQuick/private/qquickitem_p.h>
#include <algorithm>

namespace qml_material
{
namespace
{
class MenuViewport : public Flickable {
public:
    using Flickable::Flickable;
    QQuickItem* host = nullptr;
    bool        contains(const QPointF& point) const override {
        if (! QQuickItem::contains(point) || ! host) return false;
        for (auto* item : host->childItems())
            if (qobject_cast<FABMenuItem*>(item) && item->isVisible() && item->isEnabled() &&
                item->contains(item->mapFromItem(this, point)))
                return true;
        return false;
    }
};
} // namespace
bool FABMenuItem::contains(const QPointF& point) const {
    return m_inputEnabled && Button::contains(point);
}
void FABMenuItem::focusInEvent(QFocusEvent* event) {
    if (! m_inputEnabled) {
        setFocus(false);
        return;
    }
    Button::focusInEvent(event);
}
void FABMenuItem::setInputEnabled(bool value) {
    if (m_inputEnabled == value) return;
    m_inputEnabled = value;
    QPointer<FABMenuItem> guard(this);
    if (! value) {
        mouseUngrabEvent();
        if (! guard) return;
        touchUngrabEvent();
        if (! guard) return;
        QHoverEvent leave(QEvent::HoverLeave, QPointF(), QPointF(), QPointF());
        hoverLeaveEvent(&leave);
        if (! guard) return;
    }
    Q_EMIT inputEnabledChanged();
}
void FABMenuItem::setWidthProgress(qreal value) {
    value = std::clamp(value, 0., 1.);
    if (m_widthProgress == value) return;
    m_widthProgress = value;
    Q_EMIT progressChanged();
}
void FABMenuItem::setAlphaProgress(qreal value) {
    value = std::clamp(value, 0., 1.);
    if (m_alphaProgress == value) return;
    m_alphaProgress = value;
    Q_EMIT progressChanged();
}
void FABMenuItem::setAnimating(bool value) {
    if (m_animating == value) return;
    m_animating = value;
    Q_EMIT animatingChanged();
}
FABMenu::FABMenu(QQuickItem* parent): Container(parent), m_viewport(new MenuViewport(this)) {
    const token::FABMenu defaults;
    setHorizontalPadding(defaults.edgePadding);
    setBottomPadding(defaults.edgePadding);
    setSpacing(defaults.itemSpacing);
    m_buttonSpacing = defaults.buttonSpacing;
    setContentItem(m_viewport);
    setContentItemLayout(LayoutNone);
    contentHost()->setParentItem(m_viewport->contentItem());
    static_cast<MenuViewport*>(m_viewport)->host = contentHost();
    m_viewport->setClip(true);
    m_viewport->setFlickableDirection(Flickable::VerticalFlick);
    connect(this, &Control::availableWidthChanged, this, &QQuickItem::polish);
    connect(this, &Control::availableHeightChanged, this, &QQuickItem::polish);
    connect(this, &Control::spacingChanged, this, &QQuickItem::polish);
    connect(this, &Control::mirroredChanged, this, &QQuickItem::polish);
}
FABMenu::~FABMenu() {
    beginTeardown();
    utils::disconnectAll(m_buttonConnections);
    for (auto& connections : m_itemConnections) utils::disconnectAll(connections);
}
QQuickItem* FABMenu::viewport() const { return m_viewport; }
bool        FABMenu::isContent(QQuickItem* item) const { return qobject_cast<FABMenuItem*>(item); }
QList<FABMenuItem*> FABMenu::visibleItems() const {
    QList<FABMenuItem*> result;
    for (auto item : items())
        if (item && QQuickItemPrivate::get(item)->explicitVisible)
            result.append(static_cast<FABMenuItem*>(item.data()));
    return result;
}
void FABMenu::itemAdded(QQuickItem* item) {
    auto* entry = static_cast<FABMenuItem*>(item);
    QQuickItemPrivate::get(entry)->setCulled(true);
    item->installEventFilter(this);
    auto& connections = m_itemConnections[item];
    connections.append(connect(entry, &FABMenuItem::progressChanged, this, &QQuickItem::polish));
    connections.append(connect(entry, &FABMenuItem::animatingChanged, this, [this] {
        polish();
        updateTransitioning();
    }));
    connections.append(connect(entry, &QObject::destroyed, this, [this, item] {
        m_itemConnections.remove(item);
        polish();
    }));
    entry->setInputEnabled(false);
}
void FABMenu::itemRemoved(QQuickItem* item) {
    item->removeEventFilter(this);
    auto connections = m_itemConnections.take(item);
    utils::disconnectAll(connections);
    auto* entry            = static_cast<FABMenuItem*>(item);
    entry->m_inputEnabled  = true;
    entry->m_revealed      = true;
    entry->m_motionEnabled = false;
    QQuickItemPrivate::get(entry)->setCulled(false);
    QPointer<FABMenuItem> guard(entry);
    Q_EMIT entry->inputEnabledChanged();
    if (guard) Q_EMIT entry->presentationChanged();
}
void FABMenu::setButton(QQuickItem* value) {
    if (value == m_button || value == this || (value && value->isAncestorOf(this))) return;
    utils::disconnectAll(m_buttonConnections);
    QPointer<FABMenu>    guard(this);
    QPointer<QQuickItem> next(value);
    if (m_button) {
        m_button->removeEventFilter(this);
        m_button->setParentItem(nullptr);
        if (! guard) return;
    }
    m_button = next;
    if (next) {
        next->setParentItem(this);
        if (! guard || ! next) return;
        next->installEventFilter(this);
        for (auto signal :
             { &QQuickItem::implicitWidthChanged, &QQuickItem::implicitHeightChanged })
            m_buttonConnections.append(connect(value, signal, this, &QQuickItem::polish));
        m_buttonConnections.append(connect(value, &QObject::destroyed, this, [this] {
            m_button = nullptr;
            polish();
            Q_EMIT buttonChanged();
        }));
    }
    polish();
    Q_EMIT buttonChanged();
}
void FABMenu::setExpanded(bool value) {
    if (m_expanded == value) return;
    m_expanded = value;
    QPointer<FABMenu> guard(this);
    if (! value && window()) {
        auto* focus = window()->activeFocusItem();
        if (focus && contentHost()->isAncestorOf(focus) && m_button)
            m_button->forceActiveFocus(Qt::OtherFocusReason);
    }
    if (! guard) return;
    updateInput();
    if (! guard) return;
    polish();
    Q_EMIT expandedChanged();
}
void FABMenu::setHorizontalAlignment(Qt::Alignment value) {
    if (m_alignment == value) return;
    m_alignment = value;
    polish();
    Q_EMIT horizontalAlignmentChanged();
}
void FABMenu::setButtonSpacing(qreal value) {
    value = std::max(0., value);
    if (m_buttonSpacing == value) return;
    m_buttonSpacing = value;
    polish();
    Q_EMIT buttonSpacingChanged();
}
void FABMenu::setRevealCount(qreal value) {
    if (m_revealCount == value) return;
    m_revealCount = value;
    polish();
    Q_EMIT revealCountChanged();
}
void FABMenu::setSequencing(bool value) {
    if (m_sequencing == value) return;
    m_sequencing = value;
    QPointer<FABMenu> guard(this);
    Q_EMIT sequencingChanged();
    if (guard) updateTransitioning();
}
void FABMenu::setAnimationsEnabled(bool value) {
    if (m_animationsEnabled == value) return;
    m_animationsEnabled = value;
    QPointer<FABMenu> guard(this);
    const auto        snapshot = items();
    for (auto item : snapshot) {
        if (! item) continue;
        auto* entry            = static_cast<FABMenuItem*>(item.data());
        entry->m_motionEnabled = m_ready && value;
        Q_EMIT entry->presentationChanged();
        if (! guard) return;
    }
    Q_EMIT animationsEnabledChanged();
}
void FABMenu::updateTransitioning() {
    bool value = m_sequencing;
    for (auto item : items())
        if (item) value |= static_cast<FABMenuItem*>(item.data())->animating();
    if (value == m_transitioning) return;
    m_transitioning = value;
    Q_EMIT transitioningChanged();
}
void FABMenu::updateInput() {
    QPointer<FABMenu> guard(this);
    const auto        snapshot = items();
    for (auto item : snapshot) {
        if (! item) continue;
        auto*      entry   = static_cast<FABMenuItem*>(item.data());
        const bool enabled = m_expanded && entry->revealed() && entry->alphaProgress() > 0;
        entry->setInputEnabled(enabled);
        if (! guard) return;
    }
}
void FABMenu::updatePolish() {
    QPointer<FABMenu> guard(this);
    Container::updatePolish();
    if (! guard) return;
    const auto version = revision();
    const auto valid   = [&] {
        if (! guard) return false;
        if (revision() == version) return true;
        polish();
        return false;
    };
    const auto entries = visibleItems();
    if (m_visibleCount != entries.size()) {
        m_visibleCount = entries.size();
        Q_EMIT visibleCountChanged();
        if (! valid()) return;
    }
    const bool  right         = bool(m_alignment & Qt::AlignRight) != mirrored();
    const qreal buttonWidth   = m_button ? m_button->implicitWidth() : 0;
    const qreal buttonHeight  = m_button ? m_button->implicitHeight() : 0;
    qreal       naturalWidth  = buttonWidth;
    qreal       naturalHeight = 0;
    bool        presenting    = m_expanded || m_sequencing;
    for (auto* entry : entries) {
        naturalWidth = std::max(naturalWidth, entry->implicitWidth());
        naturalHeight += entry->implicitHeight();
        presenting |= entry->alphaProgress() > 0 || entry->animating();
    }
    if (! entries.isEmpty()) naturalHeight += spacing() * (entries.size() - 1);
    const qreal gap = entries.isEmpty() || ! m_button ? 0 : m_buttonSpacing;
    setImplicitContentSize({ naturalWidth, buttonHeight + (presenting ? naturalHeight + gap : 0) });
    if (! valid()) return;
    const qreal w = availableWidth();
    const qreal h = std::min(naturalHeight, std::max(0., availableHeight() - buttonHeight - gap));
    m_viewport->setPosition(
        { leftPadding(), topPadding() + std::max(0., availableHeight() - buttonHeight - gap - h) });
    if (! valid()) return;
    m_viewport->setSize({ w, h });
    if (! valid()) return;
    m_viewport->setContentWidth(w);
    if (! valid()) return;
    m_viewport->setContentHeight(naturalHeight);
    if (! valid()) return;
    contentHost()->setPosition({ 0, 0 });
    if (! valid()) return;
    contentHost()->setSize({ w, naturalHeight });
    if (! valid()) return;
    m_viewport->setInteractive(m_expanded && naturalHeight > h);
    if (! valid()) return;
    if (m_button) {
        m_button->setSize({ std::min(w, buttonWidth), buttonHeight });
        if (! valid()) return;
        if (m_button)
            m_button->setPosition({ leftPadding() + (right ? w - m_button->width() : 0),
                                    height() - bottomPadding() - buttonHeight });
        if (! valid()) return;
    }
    qreal      y        = 0;
    const auto snapshot = items();
    for (auto item : snapshot) {
        if (! item) continue;
        auto*     entry = static_cast<FABMenuItem*>(item.data());
        const int index = entries.indexOf(entry);
        if (index < 0) {
            continue;
        }
        const bool reveal = index >= entries.size() - qRound(m_revealCount);
        if (entry->m_revealed != reveal || entry->m_alignRight != right) {
            entry->m_revealed   = reveal;
            entry->m_alignRight = right;
            Q_EMIT entry->presentationChanged();
            if (! valid() || ! item) return;
        }
        const qreal fullWidth = std::min(w, entry->implicitWidth());
        if (entry->m_fullWidth != fullWidth) {
            entry->m_fullWidth = fullWidth;
            Q_EMIT entry->fullWidthChanged();
            if (! valid() || ! item) return;
        }
        const qreal width = fullWidth * entry->widthProgress();
        entry->setPosition({ right ? w - width : 0, y });
        if (! valid() || ! item) return;
        entry->setSize({ width, entry->implicitHeight() });
        if (! valid() || ! item) return;
        QQuickItemPrivate::get(entry)->setCulled(entry->alphaProgress() <= 0);
        y += entry->implicitHeight() + spacing();
        if (m_animationsEnabled && ! entry->m_motionEnabled) {
            entry->m_motionEnabled = true;
            Q_EMIT entry->presentationChanged();
            if (! valid() || ! item) return;
        }
    }
    updateInput();
    if (! valid()) return;
    updateTransitioning();
    if (! valid()) return;
    if (! m_ready) {
        m_ready = true;
        Q_EMIT motionEnabledChanged();
    }
}
bool FABMenu::eventFilter(QObject* object, QEvent* event) {
    if (event->type() != QEvent::KeyPress || ! m_expanded)
        return Container::eventFilter(object, event);
    auto* key     = static_cast<QKeyEvent*>(event);
    auto  entries = visibleItems();
    entries.removeIf([](FABMenuItem* item) {
        return ! item->isEnabled() || ! item->isVisible() || ! item->inputEnabled();
    });
    QQuickItem* target = nullptr;
    const bool  back = key->key() == Qt::Key_Up || key->key() == Qt::Key_Backtab ||
                       (key->key() == Qt::Key_Tab && key->modifiers().testFlag(Qt::ShiftModifier));
    const bool  forward = key->key() == Qt::Key_Down || (key->key() == Qt::Key_Tab && ! back);
    if (key->key() == Qt::Key_Escape) {
        close();
        return true;
    }
    const int index = entries.indexOf(qobject_cast<FABMenuItem*>(object));
    if (object == m_button && forward && ! entries.isEmpty())
        target = entries.first();
    else if (index >= 0 && back)
        target = index ? entries[index - 1] : m_button.data();
    else if (index >= 0 && forward && index + 1 < entries.size())
        target = entries[index + 1];
    if (! target) return Container::eventFilter(object, event);
    target->forceActiveFocus(back ? Qt::BacktabFocusReason : Qt::TabFocusReason);
    if (auto* entry = qobject_cast<FABMenuItem*>(target)) {
        const qreal y = presentationItem(entry)->y();
        m_viewport->setContentY(
            std::clamp(y, 0., std::max(0., m_viewport->contentHeight() - m_viewport->height())));
    }
    return true;
}
} // namespace qml_material
