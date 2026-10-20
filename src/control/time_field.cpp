#include "qml_material/control/time_field.hpp"
#include <QMouseEvent>
#include <QTouchEvent>
#include <QKeyEvent>

namespace qml_material
{
TimeField::TimeField(QQuickItem* parent): TextField(parent) {
    auto& click          = inputHost().click();
    click.pointerStarted = [this](const PointerInput&) {
        selectField();
    };
    click.clicked = [this] {
        selectField();
    };
    click.combined().doubleClickEnabled = true;
    click.combined().doubleClicked      = [this] {
        Q_EMIT inputRequested();
    };
    connect(this, &QQuickTextInput::readOnlyChanged, this, [this] {
        inputHost().cancelActivation();
    });
    connect(this, &QQuickItem::activeFocusChanged, this, [this] {
        if (hasActiveFocus()) selectField();
    });
}
TimeField::~TimeField() { disconnect(m_destroyed); }
void TimeField::selectField() {
    if (m_time) m_time->setSelection(m_selection);
}
void TimeField::setTime(TimeState* time) {
    if (m_time == time && (time || ! m_destroyed)) return;
    const auto                revision = ++m_revision;
    const QPointer<TimeField> guard(this);
    const QPointer<TimeState> next(time);
    inputHost().cancelActivation();
    if (! guard || revision != m_revision) return;
    disconnect(m_destroyed);
    m_destroyed = {};
    m_time      = next;
    if (next)
        m_destroyed = connect(next, &QObject::destroyed, this, [this] {
            setTime(nullptr);
        });
    Q_EMIT configurationChanged();
}
void TimeField::setSelection(TimeState::Selection value) {
    if ((value != TimeState::Hours && value != TimeState::Minutes) || value == m_selection) return;
    m_selection = value;
    const QPointer<TimeField> guard(this);
    inputHost().cancelActivation();
    if (guard) Q_EMIT configurationChanged();
}
#define POINTER(Name, Event)             \
    void TimeField::Name(Event* event) { \
        if (isReadOnly())                \
            inputHost().event(event);    \
        else                             \
            TextField::Name(event);      \
    }
POINTER(mousePressEvent, QMouseEvent)
POINTER(mouseMoveEvent, QMouseEvent)
POINTER(mouseReleaseEvent, QMouseEvent)
POINTER(mouseDoubleClickEvent, QMouseEvent)
POINTER(touchEvent, QTouchEvent)
POINTER(keyPressEvent, QKeyEvent)
POINTER(keyReleaseEvent, QKeyEvent)
#undef POINTER
void TimeField::touchUngrabEvent() {
    if (isReadOnly())
        inputHost().touchUngrabbed();
    else
        TextField::touchUngrabEvent();
}
} // namespace qml_material
