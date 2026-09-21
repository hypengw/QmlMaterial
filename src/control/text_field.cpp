#include "qml_material/control/text_field.hpp"
#include "qml_material/util/qt.hpp"
#include <QGuiApplication>
#include <QMetaMethod>
#include <QtQuick/private/qquicktextinput_p_p.h>
#include <QtQuick/private/qquickclipnode_p.h>

namespace qml_material
{

class TextFieldPrivate : public QQuickTextInputPrivate {
public:
    qreal getImplicitWidth() const override { return QQuickItemPrivate::getImplicitWidth(); }
    qreal getImplicitHeight() const override { return QQuickItemPrivate::getImplicitHeight(); }
    void  implicitWidthChanged() override {
        QQuickItemPrivate::implicitWidthChanged();
        Q_EMIT static_cast<TextField*>(q_func())->fieldImplicitWidthChanged();
    }
    void implicitHeightChanged() override {
        QQuickItemPrivate::implicitHeightChanged();
        Q_EMIT static_cast<TextField*>(q_func())->fieldImplicitHeightChanged();
    }
};

TextField::TextField(QQuickItem* parent)
    : QQuickTextInput(*new TextFieldPrivate, parent), m_background(this) {
    QQuickTextInputPrivate::get(this)->setImplicitResizeEnabled(false);
    setAcceptedMouseButtons(Qt::AllButtons);
    setActiveFocusOnTab(true);
    setCursor(Qt::IBeamCursor);
    m_hold_timer.setSingleShot(true);
    connect(&m_hold_timer, &QTimer::timeout, this, &TextField::holdTimeout);
    const auto updateCursor = [this]() {
        setCursor(isReadOnly() && ! selectByMouse() ? Qt::ArrowCursor : Qt::IBeamCursor);
    };
    connect(this, &QQuickTextInput::readOnlyChanged, this, updateCursor);
    connect(this, &QQuickTextInput::selectByMouseChanged, this, updateCursor);
    connect(QGuiApplication::styleHints(),
            &QStyleHints::useHoverEffectsChanged,
            this,
            &TextField::refreshInheritedEnvironment);
    connect(
        &m_background, &TextControlBackground::itemChanged, this, &TextField::backgroundChanged);
    connect(&m_background,
            &TextControlBackground::implicitWidthChanged,
            this,
            &TextField::implicitBackgroundWidthChanged);
    connect(&m_background,
            &TextControlBackground::implicitHeightChanged,
            this,
            &TextField::implicitBackgroundHeightChanged);
    connect(&m_background, &TextControlBackground::insetChanged, this, [this](Qt::Edge edge) {
        switch (edge) {
        case Qt::TopEdge: Q_EMIT topInsetChanged(); break;
        case Qt::LeftEdge: Q_EMIT leftInsetChanged(); break;
        case Qt::RightEdge: Q_EMIT rightInsetChanged(); break;
        case Qt::BottomEdge: Q_EMIT bottomInsetChanged(); break;
        }
    });
    updateFont();
    connect(qGuiApp, &QGuiApplication::fontChanged, this, &TextField::refreshInheritedEnvironment);
}

TextField::~TextField() = default;

QFont TextField::font() const {
    auto result = m_effective_font;
    result.setResolveMask(m_requested_font.resolveMask());
    return result;
}

void TextField::setFont(const QFont& font) {
    if (m_requested_font == font && m_requested_font.resolveMask() == font.resolveMask()) return;
    m_requested_font = font;
    refreshInheritedEnvironment();
}

void TextField::resetFont() { setFont(QFont()); }

void TextField::updateFont() {
    const auto resolved = utils::resolveFont(m_requested_font, utils::inheritedFont(this));
    const bool changed  = m_effective_font != resolved;
    m_effective_font    = resolved;
    QQuickTextInput::setFont(resolved);
    if (changed) Q_EMIT fontChanged();
}

void TextField::refreshInheritedEnvironment() {
    updateFont();
    updateHover();
    utils::propagateControlEnvironment(this);
}

qreal TextField::implicitBackgroundWidth() const { return m_background.implicitWidth(); }
qreal TextField::implicitBackgroundHeight() const { return m_background.implicitHeight(); }
void  TextField::setBackground(QQuickItem* item) { m_background.setItem(item); }

void TextField::componentComplete() {
    QQuickTextInput::componentComplete();
    refreshInheritedEnvironment();
    m_background.complete();
}
void TextField::itemChange(ItemChange change, const ItemChangeData& data) {
    QQuickTextInput::itemChange(change, data);
    if (change == ItemParentHasChanged || change == ItemSceneChange) {
        cancelPress();
        refreshInheritedEnvironment();
    }
    if ((change == ItemEnabledHasChanged && ! isEnabled()) ||
        (change == ItemVisibleHasChanged && ! isVisible())) {
        cancelPress();
        setHovered(false);
    }
}
void TextField::geometryChange(const QRectF& geometry, const QRectF& oldGeometry) {
    QQuickTextInput::geometryChange(geometry, oldGeometry);
    m_background.layout();
}

void TextField::setTopInset(qreal value) { m_background.setInset(Qt::TopEdge, value); }
void TextField::resetTopInset() { m_background.resetInset(Qt::TopEdge); }

void TextField::setLeftInset(qreal value) { m_background.setInset(Qt::LeftEdge, value); }
void TextField::resetLeftInset() { m_background.resetInset(Qt::LeftEdge); }

void TextField::setRightInset(qreal value) { m_background.setInset(Qt::RightEdge, value); }
void TextField::resetRightInset() { m_background.resetInset(Qt::RightEdge); }

void TextField::setBottomInset(qreal value) { m_background.setInset(Qt::BottomEdge, value); }
void TextField::resetBottomInset() { m_background.resetInset(Qt::BottomEdge); }
void TextField::setPlaceholderText(const QString& value) {
    if (m_placeholder == value) return;
    m_placeholder = value;
    Q_EMIT placeholderTextChanged();
}
void TextField::setPlaceholderTextColor(const QColor& value) {
    if (m_placeholder_color == value) return;
    m_placeholder_color = value;
    Q_EMIT placeholderTextColorChanged();
}
void TextField::setFocusReason(Qt::FocusReason value) {
    if (m_focus_reason == value) return;
    m_focus_reason = value;
    Q_EMIT focusReasonChanged();
}
void TextField::focusInEvent(QFocusEvent* event) {
    QQuickTextInput::focusInEvent(event);
    setFocusReason(event->reason());
}
void TextField::focusOutEvent(QFocusEvent* event) {
    QQuickTextInput::focusOutEvent(event);
    setFocusReason(event->reason());
}
void TextField::setHovered(bool value) {
    if (m_hovered == value) return;
    m_hovered = value;
    Q_EMIT hoveredChanged();
}
void TextField::updateHover() {
    const bool value = m_requested_hover.value_or(utils::inheritedHoverEnabled(this));
    if (value == hoverEnabled()) return;
    setAcceptHoverEvents(value);
    if (! value) setHovered(false);
    Q_EMIT hoverEnabledChanged();
}
void TextField::setHoverEnabled(bool value) {
    m_requested_hover = value;
    refreshInheritedEnvironment();
}
void TextField::resetHoverEnabled() {
    m_requested_hover.reset();
    refreshInheritedEnvironment();
}
void TextField::hoverEnterEvent(QHoverEvent* event) {
    QQuickTextInput::hoverEnterEvent(event);
    setHovered(hoverEnabled());
    event->ignore();
}
void TextField::hoverLeaveEvent(QHoverEvent* event) {
    QQuickTextInput::hoverLeaveEvent(event);
    setHovered(false);
    event->ignore();
}
QSGNode* TextField::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data) {
    auto clip = static_cast<QQuickDefaultClipNode*>(oldNode);
    if (! clip) clip = new QQuickDefaultClipNode(QRectF());
    auto rect = clipRect().adjusted(leftPadding(), topPadding(), -rightPadding(), -bottomPadding());
    rect.setWidth(qMax<qreal>(0, rect.width()));
    rect.setHeight(qMax<qreal>(0, rect.height()));
    clip->setRect(rect);
    clip->update();
    auto text = QQuickTextInput::updatePaintNode(clip->firstChild(), data);
    if (text && ! text->parent()) clip->appendChildNode(text);
    return clip;
}

void TextField::cancelPress() {
    m_hold_timer.stop();
    m_delayed_press.reset();
    m_pressed = false;
    m_held    = false;
    QQuickTextInput::mouseUngrabEvent();
}
void TextField::deliverPress() {
    auto event = std::move(m_delayed_press);
    if (event) QQuickTextInput::mousePressEvent(event.get());
}
void TextField::mousePressEvent(QMouseEvent* event) {
    cancelPress();
    m_pressed   = true;
    m_press_pos = event->position();
    if (event->button() == Qt::LeftButton) {
        m_delayed_press.reset(event->clone());
        m_hold_timer.start(QGuiApplication::styleHints()->mousePressAndHoldInterval());
    }
    QQuickMouseEvent mouse;
    mouse.reset(m_press_pos.x(),
                m_press_pos.y(),
                event->button(),
                event->buttons(),
                event->modifiers(),
                false,
                false);
    mouse.setAccepted(true);
    QPointer<TextField> guard(this);
    Q_EMIT pressed(&mouse);
    if (! guard) return;
    event->setAccepted(mouse.isAccepted());
    if (! mouse.isAccepted()) {
        cancelPress();
        return;
    }
    if (m_pressed && event->button() != Qt::LeftButton && event->button() != Qt::RightButton)
        QQuickTextInput::mousePressEvent(event);
}
void TextField::mouseMoveEvent(QMouseEvent* event) {
    if (! m_pressed || m_held) return;
    if (qAbs(event->position().x() - m_press_pos.x()) >
        QGuiApplication::styleHints()->startDragDistance())
        m_hold_timer.stop();
    if (m_hold_timer.isActive()) return;
    QPointer<TextField> guard(this);
    deliverPress();
    if (! guard || ! m_pressed) return;
    if (event->buttons().testFlag(Qt::LeftButton)) QQuickTextInput::mouseMoveEvent(event);
}
void TextField::mouseReleaseEvent(QMouseEvent* event) {
    if (! m_pressed) return;
    m_hold_timer.stop();
    QQuickMouseEvent mouse;
    mouse.reset(m_press_pos.x(),
                m_press_pos.y(),
                event->button(),
                event->buttons(),
                event->modifiers(),
                false,
                false);
    mouse.setAccepted(true);
    QPointer<TextField> guard(this);
    Q_EMIT released(&mouse);
    if (! guard) return;
    event->setAccepted(mouse.isAccepted());
    if (m_pressed && ! m_held && mouse.isAccepted() && event->button() != Qt::RightButton) {
        deliverPress();
        if (! guard) return;
        if (m_pressed) QQuickTextInput::mouseReleaseEvent(event);
        if (! guard) return;
    }
    cancelPress();
}
void TextField::mouseDoubleClickEvent(QMouseEvent* event) {
    m_hold_timer.stop();
    QPointer<TextField> guard(this);
    deliverPress();
    if (! guard) return;
    if (event->button() == Qt::LeftButton) QQuickTextInput::mouseDoubleClickEvent(event);
}
void TextField::mouseUngrabEvent() { cancelPress(); }
void TextField::holdTimeout() {
    if (! m_pressed) return;
    if (! isSignalConnected(QMetaMethod::fromSignal(&TextField::pressAndHold))) {
        deliverPress();
        return;
    }
    QQuickMouseEvent mouse;
    mouse.reset(m_press_pos.x(),
                m_press_pos.y(),
                Qt::LeftButton,
                Qt::LeftButton,
                m_delayed_press ? m_delayed_press->modifiers() : Qt::NoModifier,
                false,
                true);
    mouse.setAccepted(true);
    m_held = true;
    QPointer<TextField> guard(this);
    Q_EMIT pressAndHold(&mouse);
    if (! guard || ! m_pressed) return;
    if (mouse.isAccepted())
        m_delayed_press.reset();
    else {
        m_held = false;
        deliverPress();
    }
}

} // namespace qml_material
