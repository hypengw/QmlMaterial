#include "qml_material/control/number_input.hpp"
#include "qml_material/control/text_field.hpp"
#include "qml_material/util/qt.hpp"
#include "input/control_accessibility_p.hpp"
#include <QKeyEvent>
#include <algorithm>
#include <cmath>

namespace qml_material
{
NumberInput::NumberInput(QQuickItem* parent): Control(parent) {
    detail::ensureControlAccessibility();
    connect(this, &NumberInput::valueChanged, this, &NumberInput::formatChanged);
    connect(this, &NumberInput::valueChanged, this, [this] {
        detail::notifyAccessibleValue(this, value());
    });
    setFocusPolicy(Qt::StrongFocus);
}
NumberInput::~NumberInput() {
    utils::disconnectAll(m_buttons);
    utils::disconnectAll(m_editorConnections);
}
QString    NumberInput::formattedValue() const { return QString::number(m_value, 'f', m_decimals); }
TextField* NumberInput::editor() const { return m_editor; }
void       NumberInput::setValue(qreal value) {
    if (! std::isfinite(value) || m_value == value) return;
    m_value = value;
    Q_EMIT valueChanged();
}
void NumberInput::cancelButtons() {
    const QPointer<NumberInput> guard(this);
    const auto                  down = m_decrease;
    if (m_increase) m_increase->cancelActivation();
    if (guard && down) down->cancelActivation();
}
#define CONSTRAINT(Type, Name, Upper, Validation)      \
    void NumberInput::set##Upper(Type value) {         \
        if ((Validation) || m_##Name == value) return; \
        m_##Name = value;                              \
        const QPointer<NumberInput> guard(this);       \
        cancelButtons();                               \
        if (guard) Q_EMIT constraintsChanged();        \
    }
CONSTRAINT(qreal, from, From, std::isnan(value))
CONSTRAINT(qreal, to, To, std::isnan(value))
CONSTRAINT(qreal, step, Step, ! std::isfinite(value) || value < 0)
CONSTRAINT(bool, wrap, Wrap, false)
#undef CONSTRAINT
void NumberInput::setDecimals(int value) {
    value = std::clamp(value, 0, 15);
    if (m_decimals == value) return;
    m_decimals = value;
    Q_EMIT formatChanged();
}
void NumberInput::setIncreaseButton(CommandButton* button) {
    if (m_increase == button) return;
    const QPointer<NumberInput>   guard(this);
    const QPointer<CommandButton> next(button);
    cancelButtons();
    if (! guard) return;
    m_increase = next;
    bindButtons();
}
void NumberInput::setDecreaseButton(CommandButton* button) {
    if (m_decrease == button) return;
    const QPointer<NumberInput>   guard(this);
    const QPointer<CommandButton> next(button);
    cancelButtons();
    if (! guard) return;
    m_decrease = next;
    bindButtons();
}
void NumberInput::bindButtons() {
    utils::disconnectAll(m_buttons);
    if (m_increase) {
        m_buttons.append(
            connect(m_increase, &CommandButton::clicked, this, &NumberInput::increase));
        m_buttons.append(connect(m_increase, &QObject::destroyed, this, [this] {
            bindButtons();
        }));
    }
    if (m_decrease) {
        m_buttons.append(
            connect(m_decrease, &CommandButton::clicked, this, &NumberInput::decrease));
        m_buttons.append(connect(m_decrease, &QObject::destroyed, this, [this] {
            bindButtons();
        }));
    }
    Q_EMIT buttonsChanged();
}
void NumberInput::setEditor(TextField* editor) {
    if (m_editor == editor) return;
    utils::disconnectAll(m_editorConnections);
    m_editor = editor;
    if (editor) {
        m_editorConnections.append(connect(editor, &QQuickTextInput::editingFinished, this, [this] {
            if (! m_editor) return;
            bool                        ok    = false;
            const auto                  value = m_editor->text().toDouble(&ok);
            const QPointer<NumberInput> guard(this);
            if (ok) requestValue(value);
            if (guard && m_editor) m_editor->setText(formattedValue());
        }));
        m_editorConnections.append(
            connect(editor, &QObject::destroyed, this, &NumberInput::editorChanged));
    }
    Q_EMIT editorChanged();
}
void NumberInput::requestValue(qreal value) {
    if (! isEnabled() || ! isVisible() || ! std::isfinite(value)) return;
    const auto low = std::min(m_from, m_to), high = std::max(m_from, m_to);
    const auto span = high - low;
    if (m_wrap && std::isfinite(span) && span > 0 && (value < low || value > high)) {
        value = low + std::fmod(value - low, span);
        if (value < low) value += span;
    } else
        value = std::clamp(value, low, high);
    if (std::isfinite(value) && value != m_value) Q_EMIT valueChangeRequested(value);
}
void NumberInput::increase() { requestValue(m_value + m_step); }
void NumberInput::decrease() { requestValue(m_value - m_step); }
void NumberInput::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Up)
        increase();
    else if (event->key() == Qt::Key_Down)
        decrease();
    else {
        Control::keyPressEvent(event);
        return;
    }
    event->accept();
}
} // namespace qml_material
