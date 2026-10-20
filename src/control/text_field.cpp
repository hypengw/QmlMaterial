#include "qml_material/control/text_field.hpp"
#include "qml_material/util/qt.hpp"
#include <QGuiApplication>
#include <QStyleHints>
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
    : QQuickTextInput(*new TextFieldPrivate, parent), m_background(this), m_input(this, [this] {
          Q_EMIT interactionSourceChanged();
      }) {
    QQuickTextInputPrivate::get(this)->setImplicitResizeEnabled(false);
    setAcceptedMouseButtons(Qt::AllButtons);
    setActiveFocusOnTab(true);
    setCursor(Qt::IBeamCursor);
    const auto updateCursor = [this]() {
        setCursor(isReadOnly() && ! selectByMouse() ? Qt::ArrowCursor : Qt::IBeamCursor);
    };
    connect(this, &QQuickTextInput::readOnlyChanged, this, updateCursor);
    connect(this, &QQuickTextInput::selectByMouseChanged, this, updateCursor);
    connect(QGuiApplication::styleHints(),
            &QStyleHints::useHoverEffectsChanged,
            this,
            &TextField::resolveHoverEnabled);
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
    resolveFont();
    resolveHoverEnabled();
    connect(qGuiApp, &QGuiApplication::fontChanged, this, &TextField::resolveFont);
}

TextField::~TextField() = default;

bool TextField::event(QEvent* event) {
    if (! usesEditorInput()) {
        const QPointer<TextField> guard(this);
        const bool                handled = m_input.observeEvent(event);
        return ! guard || handled ? true : QQuickTextInput::event(event);
    }
    return QtInputAdapter::deliverToEditor(m_input, event, [this, event] {
        return QQuickTextInput::event(event);
    });
}
void TextField::mouseUngrabEvent() {
    QQuickTextInput::mouseUngrabEvent();
    m_input.mouseUngrabbed();
}

QFont TextField::font() const {
    auto result = m_effective_font;
    result.setResolveMask(m_requested_font.resolveMask());
    return result;
}

void TextField::setFont(const QFont& font) {
    if (m_requested_font == font && m_requested_font.resolveMask() == font.resolveMask()) return;
    m_requested_font = font;
    resolveFont();
}

void TextField::resetFont() { setFont(QFont()); }

void TextField::inheritFont(const QFont& inherited) {
    if (! utils::canUpdateControlEnvironment()) return;
    const auto resolved = utils::resolveFont(m_requested_font, inherited);
    const bool changed  = m_effective_font != resolved;
    if (! changed && m_effective_font.resolveMask() == resolved.resolveMask()) return;
    m_effective_font = resolved;
    QPointer<TextField> guard(this);
    QQuickTextInput::setFont(resolved);
    if (! guard) return;
    utils::propagateFont(this, resolved);
    if (guard && changed) Q_EMIT fontChanged();
}

void TextField::resolveFont() {
    if (! utils::canUpdateControlEnvironment()) return;
    inheritFont(utils::inheritedFont(this));
}

qreal TextField::implicitBackgroundWidth() const { return m_background.implicitWidth(); }
qreal TextField::implicitBackgroundHeight() const { return m_background.implicitHeight(); }
void  TextField::setBackground(QQuickItem* item) { m_background.setItem(item); }

void TextField::componentComplete() {
    QQuickTextInput::componentComplete();
    resolveFont();
    resolveHoverEnabled();
    m_background.complete();
}
void TextField::itemChange(ItemChange change, const ItemChangeData& data) {
    QPointer<TextField> life(this);
    QQuickTextInput::itemChange(change, data);
    if (! life) return;
    if (change == ItemParentHasChanged || change == ItemSceneChange) {
        if ((change == ItemParentHasChanged && data.item) ||
            (change == ItemSceneChange && data.window)) {
            QPointer<TextField> guard(this);
            resolveFont();
            if (guard) resolveHoverEnabled();
        }
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
    QPointer<TextField> guard(this);
    QQuickTextInput::focusInEvent(event);
    if (guard) setFocusReason(event->reason());
}
void TextField::focusOutEvent(QFocusEvent* event) {
    QPointer<TextField> guard(this);
    QQuickTextInput::focusOutEvent(event);
    if (guard) setFocusReason(event->reason());
}
void TextField::resolveHoverEnabled() {
    if (! utils::canUpdateControlEnvironment() || m_requested_hover) return;
    inheritHoverEnabled(utils::inheritedHoverEnabled(this));
}
void TextField::inheritHoverEnabled(bool value) {
    if (! m_requested_hover) updateHoverEnabled(value);
}
void TextField::updateHoverEnabled(bool value) {
    if (! utils::canUpdateControlEnvironment() || value == hoverEnabled()) return;
    m_input.setHoverEnabled(value);
    QPointer<TextField> guard(this);
    utils::propagateHoverEnabled(this, value);
    if (guard) Q_EMIT hoverEnabledChanged();
}
void TextField::setHoverEnabled(bool value) {
    m_requested_hover = value;
    updateHoverEnabled(value);
}
void TextField::resetHoverEnabled() {
    m_requested_hover.reset();
    resolveHoverEnabled();
}
void TextField::hoverEnterEvent(QHoverEvent* event) {
    QQuickTextInput::hoverEnterEvent(event);
    event->ignore();
}
void TextField::hoverLeaveEvent(QHoverEvent* event) {
    QQuickTextInput::hoverLeaveEvent(event);
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

} // namespace qml_material
