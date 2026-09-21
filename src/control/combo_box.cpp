#include "qml_material/control/combo_box.hpp"
#include "qml_material/control/abstract_button.hpp"
#include <QtQmlModels/private/qqmlobjectmodel_p.h>
#include <QJSValue>
#include <QRegularExpression>
#include <utility>
#include <QQuickWindow>
#include <QGuiApplication>
#include <QInputMethod>
#include <QTimer>
#include "qml_material/util/qt.hpp"

namespace qml_material
{

ComboBox::ComboBox(QQuickItem* parent): Control(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setFlag(QQuickItem::ItemIsFocusScope);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptTouchEvents(true);
    connect(this, &Control::contentItemChanged, this, &ComboBox::attachEditor);
    connect(this, &QQuickItem::enabledChanged, this, [this] {
        if (! isEnabled()) {
            QPointer<ComboBox> guard(this);
            mouseUngrabEvent();
            if (guard) closePopup();
        }
    });
    connect(this, &QQuickItem::visibleChanged, this, [this] {
        if (! isVisible()) {
            QPointer<ComboBox> guard(this);
            mouseUngrabEvent();
            if (guard) closePopup();
        }
    });
}
ComboBox::~ComboBox() { utils::disconnectAll(m_editor_connections); }

void ComboBox::ensureModel() {
    if (m_data) return;
    m_data = std::make_unique<ComboBoxModel>(qmlContext(this));
    connect(
        m_data.get(), &ComboBoxModel::delegateModelChanged, this, &ComboBox::delegateModelChanged);
    connect(m_data.get(), &ComboBoxModel::countChanged, this, [this]() {
        QPointer<ComboBox> guard(this);
        if (m_ready && ! m_accepting && count() == 0 && m_selection != Selection::Value)
            publish(-1, {});
        if (guard) Q_EMIT countChanged();
    });
    connect(m_data.get(), &ComboBoxModel::modelChanged, this, [this]() {
        QPointer<ComboBox> guard(this);
        if (m_ready && ! m_accepting) {
            if (m_selection == Selection::Value)
                synchronize();
            else
                publish(count() > 0 ? 0 : -1, valueAt(0));
        }
        if (guard) Q_EMIT modelChanged();
    });
    connect(m_data.get(), &ComboBoxModel::changed, this, &ComboBox::synchronize);
    connect(m_data.get(), &ComboBoxModel::itemCreated, this, [this](QObject* object) {
        auto button = qobject_cast<AbstractButton*>(object);
        if (! button) return;
        button->setFocusPolicy(Qt::NoFocus);
        connect(button,
                &AbstractButton::clicked,
                this,
                &ComboBox::delegateClicked,
                Qt::UniqueConnection);
        connect(button,
                &Control::hoveredChanged,
                this,
                &ComboBox::delegateHovered,
                Qt::UniqueConnection);
    });
}
void ComboBox::delegateClicked() { activateIndex(m_data->indexOf(sender())); }
void ComboBox::delegateHovered() {
    auto      button = qobject_cast<AbstractButton*>(sender());
    const int index  = m_data->indexOf(button);
    if (index >= 0 && button && button->hovered() && button->isEnabled()) highlightIndex(index);
}

void ComboBox::classBegin() {
    Control::classBegin();
    ensureModel();
}
void ComboBox::componentComplete() {
    Control::componentComplete();
    ensureModel();
    QPointer<ComboBox> guard(this);
    m_data->complete();
    if (! guard) return;
    m_ready = true;
    if (m_selection == Selection::None && count() > 0)
        publish(0, valueAt(0));
    else
        synchronize();
}

QVariant ComboBox::model() const { return m_data ? m_data->model() : QVariant(); }
void     ComboBox::setModel(const QVariant& value) {
    ensureModel();
    m_data->setModel(value);
}
QQmlComponent* ComboBox::delegate() const { return m_data ? m_data->delegate() : nullptr; }
void           ComboBox::setDelegate(QQmlComponent* value) {
    ensureModel();
    if (delegate() == value) return;
    QPointer<ComboBox> guard(this);
    m_data->setDelegate(value);
    if (guard) Q_EMIT delegateChanged();
}
QObject* ComboBox::delegateModel() const { return m_data ? m_data->delegateModel() : nullptr; }
int      ComboBox::count() const { return m_data ? m_data->count() : 0; }
QString  ComboBox::textRole() const { return m_data ? m_data->textRole() : QString(); }
void     ComboBox::setTextRole(const QString& value) {
    ensureModel();
    if (textRole() == value) return;
    QPointer<ComboBox> guard(this);
    m_data->setTextRole(value);
    if (guard) Q_EMIT textRoleChanged();
}
QString ComboBox::valueRole() const { return m_data ? m_data->valueRole() : QString(); }
void    ComboBox::setValueRole(const QString& value) {
    ensureModel();
    if (valueRole() == value) return;
    QPointer<ComboBox> guard(this);
    m_data->setValueRole(value);
    if (guard) Q_EMIT valueRoleChanged();
}
QString  ComboBox::textAt(int index) const { return m_data ? m_data->textAt(index) : QString(); }
QVariant ComboBox::valueAt(int index) const { return m_data ? m_data->valueAt(index) : QVariant(); }
int      ComboBox::indexOfValue(const QVariant& value) const {
    return m_data ? m_data->indexOfValue(value) : -1;
}

void ComboBox::synchronize() {
    if (! m_ready || m_accepting) return;
    QPointer<ComboBox> guard(this);
    if (m_highlight >= count()) setHighlight(count() - 1);
    if (! guard) return;
    if (m_selection == Selection::Value)
        publish(indexOfValue(m_value), m_value);
    else
        publish(m_index, valueAt(m_index));
}
void ComboBox::publish(int index, const QVariant& value) {
    const QString text           = m_ready ? textAt(index) : QString();
    const QString display        = m_explicit_display.value_or(text);
    const bool    indexChanged   = m_index != index;
    const bool    valueChanged   = m_value != value;
    const bool    textChanged    = m_text != text;
    const bool    displayChanged = m_display != display;
    m_index                      = index;
    m_value                      = value;
    m_text                       = text;
    m_display                    = display;
    QPointer<ComboBox> guard(this);
    if (indexChanged) Q_EMIT currentIndexChanged();
    if (! guard) return;
    if (valueChanged) Q_EMIT currentValueChanged();
    if (! guard) return;
    if (textChanged) Q_EMIT currentTextChanged();
    if (! guard) return;
    if (displayChanged) Q_EMIT displayTextChanged();
    if (! guard) return;
    if (m_ready && ! m_accepting && (indexChanged || textChanged || valueChanged))
        setEditText(m_text);
}
void ComboBox::setCurrentIndex(int index) {
    m_selection = Selection::Index;
    publish(index, m_ready ? valueAt(index) : m_value);
}
void ComboBox::setCurrentValue(const QVariant& value) {
    const auto normalized = value.metaType() == QMetaType::fromType<QJSValue>()
                                ? value.value<QJSValue>().toVariant()
                                : value;
    m_selection           = Selection::Value;
    publish(m_ready ? indexOfValue(normalized) : m_index, normalized);
}
void ComboBox::setDisplayText(const QString& text) {
    m_explicit_display = text;
    publish(m_index, m_value);
}
void ComboBox::resetDisplayText() {
    m_explicit_display.reset();
    publish(m_index, m_value);
}
void ComboBox::activateIndex(int index) {
    if (! m_ready || ! isEnabled() || index < 0 || index >= count()) return;
    QPointer<ComboBox> guard(this);
    publish(index, valueAt(index));
    if (guard) Q_EMIT activated(index);
    if (guard && m_popup_visible) closePopup();
}

void ComboBox::setHighlight(int index, bool notify) {
    index = index >= 0 && index < count() ? index : -1;
    if (m_highlight == index) return;
    m_highlight = index;
    QPointer<ComboBox> guard(this);
    Q_EMIT highlightedIndexChanged();
    if (guard && notify) Q_EMIT highlighted(index);
}
void ComboBox::highlightIndex(int index) {
    if (isEnabled() && m_popup_visible) setHighlight(index, true);
}
void ComboBox::setPopupVisible(bool visible) {
    if (m_popup_visible == visible) return;
    const bool oldDown = isDown();
    m_popup_visible    = visible;
    QPointer<ComboBox> guard(this);
    if (visible) QGuiApplication::inputMethod()->reset();
    if (! guard) return;
    setHighlight(visible ? m_index : -1);
    if (! guard) return;
    Q_EMIT popupVisibleChanged();
    if (guard && oldDown != isDown()) Q_EMIT downChanged();
}
void ComboBox::setPopupActiveFocus(bool value) {
    if (m_popup_focus == value) return;
    m_popup_focus = value;
    Q_EMIT popupActiveFocusChanged();
}
void ComboBox::setPressed(bool value) {
    if (m_pressed == value) return;
    const bool oldDown = isDown();
    m_pressed          = value;
    QPointer<ComboBox> guard(this);
    Q_EMIT pressedChanged();
    if (guard && oldDown != isDown()) Q_EMIT downChanged();
}
void ComboBox::setDown(bool value) {
    const bool old = isDown();
    m_down         = value;
    if (old != isDown()) Q_EMIT downChanged();
}
void ComboBox::resetDown() {
    const bool old = isDown();
    m_down.reset();
    if (old != isDown()) Q_EMIT downChanged();
}
void ComboBox::openPopup() {
    if (isEnabled() && ! m_popup_visible) Q_EMIT popupOpenRequested();
}
void ComboBox::closePopup(bool accept) {
    QPointer<ComboBox> guard(this);
    if (accept && m_popup_visible && m_highlight >= 0) {
        activateIndex(m_highlight);
        return;
    }
    setPressed(false);
    if (guard && m_popup_visible) Q_EMIT popupCloseRequested();
}
void ComboBox::navigate(int index) {
    if (m_popup_visible)
        highlightIndex(index);
    else if (index != m_index)
        activateIndex(index);
}
void ComboBox::checkFocus() {
    // Focus delivery updates the window's active item after FocusOut.
    QTimer::singleShot(0, this, [this] {
        auto focus = window() ? window()->activeFocusItem() : nullptr;
        if ((focus && (focus == this || focus == m_editor)) || m_popup_focus) return;
        QPointer<ComboBox> guard(this);
        closePopup();
        if (! guard) return;
        if (m_editable && acceptableInput() && ! inputMethodComposing()) {
            const int index = findEditText(m_edit_text);
            if (index >= 0 && index != m_index) activateIndex(index);
        }
    });
}
void ComboBox::focusOutEvent(QFocusEvent* event) {
    Control::focusOutEvent(event);
    checkFocus();
}
void ComboBox::mousePressEvent(QMouseEvent* event) {
    if (! isEnabled() || event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }
    QPointer<ComboBox> guard(this);
    forceActiveFocus(Qt::MouseFocusReason);
    if (! guard) return;
    m_pointer = true;
    setPressed(true);
    event->accept();
}
void ComboBox::mouseMoveEvent(QMouseEvent* event) {
    if (m_pointer) setPressed(contains(event->position()));
}
void ComboBox::finishPointer(const QPointF& position) {
    const bool activate = m_pointer && contains(position) && isEnabled();
    m_pointer           = false;
    m_touch_id          = -1;
    QPointer<ComboBox> guard(this);
    setPressed(false);
    if (! guard || ! activate) return;
    if (m_popup_visible)
        closePopup();
    else
        openPopup();
}
void ComboBox::mouseReleaseEvent(QMouseEvent* event) { finishPointer(event->position()); }
void ComboBox::mouseUngrabEvent() {
    m_pointer  = false;
    m_touch_id = -1;
    setPressed(false);
}
void ComboBox::touchUngrabEvent() { mouseUngrabEvent(); }
void ComboBox::touchEvent(QTouchEvent* event) {
    if (event->type() == QEvent::TouchCancel) {
        mouseUngrabEvent();
        return;
    }
    if (! isEnabled()) {
        event->ignore();
        return;
    }
    QPointer<ComboBox> guard(this);
    for (const auto& point : event->points()) {
        if (point.state() == QEventPoint::Pressed && m_touch_id < 0) {
            m_touch_id = point.id();
            m_pointer  = true;
            forceActiveFocus(Qt::MouseFocusReason);
            if (! guard) return;
            setPressed(true);
        } else if (point.id() == m_touch_id) {
            if (point.state() == QEventPoint::Released)
                finishPointer(point.position());
            else
                setPressed(contains(point.position()));
        }
        if (! guard) return;
    }
    event->accept();
}

void ComboBox::setEditable(bool value) {
    if (m_editable == value) return;
    m_editable = value;
    Q_EMIT editableChanged();
}
void ComboBox::setEditText(const QString& value) {
    if (m_edit_text == value) return;
    m_edit_text = value;
    Q_EMIT editTextChanged();
}
bool ComboBox::acceptableInput() const { return m_acceptable; }
bool ComboBox::inputMethodComposing() const { return m_composing; }
void ComboBox::setValidator(QValidator* value) {
    if (m_validator == value) return;
    disconnect(m_validator_connection);
    m_validator = value;
    if (value)
        m_validator_connection = connect(value, &QObject::destroyed, this, [this]() {
            m_validator = nullptr;
            Q_EMIT validatorChanged();
        });
    Q_EMIT validatorChanged();
}
void ComboBox::setInputMethodHints(Qt::InputMethodHints value) {
    if (m_input_hints == value) return;
    m_input_hints = value;
    Q_EMIT inputMethodHintsChanged();
}
void ComboBox::setSelectTextByMouse(bool value) {
    if (m_select_by_mouse == value) return;
    m_select_by_mouse = value;
    Q_EMIT selectTextByMouseChanged();
}
void ComboBox::attachEditor() {
    utils::disconnectAll(m_editor_connections);
    m_user_edit = false;
    m_editor    = qobject_cast<QQuickTextInput*>(contentItem());
    if (m_editor) {
        m_editor_connections.append(connect(m_editor, &QQuickTextInput::textEdited, this, [this]() {
            m_user_edit = true;
        }));
        m_editor_connections.append(
            connect(m_editor, &QQuickTextInput::textChanged, this, &ComboBox::updateEditText));
        m_editor_connections.append(
            connect(m_editor, &QQuickTextInput::accepted, this, &ComboBox::acceptInput));
        m_editor_connections.append(connect(m_editor,
                                            &QQuickTextInput::acceptableInputChanged,
                                            this,
                                            &ComboBox::updateEditorState));
        m_editor_connections.append(connect(m_editor,
                                            &QQuickTextInput::inputMethodComposingChanged,
                                            this,
                                            &ComboBox::updateEditorState));
        m_editor_connections.append(connect(m_editor, &QObject::destroyed, this, [this]() {
            m_editor = nullptr;
            updateEditorState();
        }));
    }
    updateEditorState();
}
void ComboBox::updateEditorState() {
    const bool acceptable        = ! m_editor || m_editor->hasAcceptableInput();
    const bool composing         = m_editor && m_editor->isInputMethodComposing();
    const bool acceptableChanged = m_acceptable != acceptable;
    const bool composingChanged  = m_composing != composing;
    m_acceptable                 = acceptable;
    m_composing                  = composing;
    QPointer<ComboBox> guard(this);
    if (acceptableChanged) Q_EMIT acceptableInputChanged();
    if (guard && composingChanged) Q_EMIT inputMethodComposingChanged();
}
void ComboBox::selectAll() {
    if (m_editable && m_editor) m_editor->selectAll();
}
int  ComboBox::findEditText(const QString& text) const { return find(text, Qt::MatchFixedString); }
void ComboBox::acceptInput() {
    if (! m_ready || ! m_editable || ! isEnabled() || ! m_editor || ! acceptableInput() ||
        inputMethodComposing() || m_accepting)
        return;
    QPointer<ComboBox> guard(this);
    int                index = findEditText(m_edit_text);
    m_accepting              = true;
    if (index >= 0) {
        publish(index, valueAt(index));
        if (! guard) return;
        setEditText(m_text);
        if (! guard) return;
        if (m_editor) m_editor->deselect();
        if (! guard) return;
    }
    Q_EMIT accepted();
    if (! guard) return;
    // accepted handlers may insert, replace or reorder the model.
    index = findEditText(m_edit_text);
    publish(index, valueAt(index));
    if (! guard) return;
    m_accepting = false;
}
void ComboBox::focusInEvent(QFocusEvent* event) {
    Control::focusInEvent(event);
    if (m_editable && m_editor &&
        (event->reason() == Qt::TabFocusReason || event->reason() == Qt::BacktabFocusReason ||
         event->reason() == Qt::ShortcutFocusReason))
        m_editor->forceActiveFocus(event->reason());
}

void ComboBox::setWheelEnabled(bool value) {
    if (m_wheel_enabled == value) return;
    m_wheel_enabled = value;
    Q_EMIT wheelEnabledChanged();
}
int ComboBox::find(const QString& text, Qt::MatchFlags flags) const {
    return match(0, text, flags);
}
int ComboBox::match(int start, const QString& text, Qt::MatchFlags flags) const {
    const int size = count();
    if (size == 0) return -1;
    start           = qBound(0, start, size);
    const int  mode = flags.toInt() & 0x0f;
    const auto sensitivity =
        flags.testFlag(Qt::MatchCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive;
    QRegularExpression expression;
    if (mode == Qt::MatchRegularExpression || mode == Qt::MatchWildcard) {
        expression.setPattern(mode == Qt::MatchWildcard
                                  ? QRegularExpression::wildcardToRegularExpression(text)
                                  : QRegularExpression::anchoredPattern(text));
        if (sensitivity == Qt::CaseInsensitive)
            expression.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        if (! expression.isValid()) return -1;
    }
    const int length = flags.testFlag(Qt::MatchWrap) ? size : size - start;
    for (int offset = 0; offset < length; ++offset) {
        const int  index     = (start + offset) % size;
        const auto candidate = textAt(index);
        bool       matches   = false;
        switch (mode) {
        case Qt::MatchExactly: matches = candidate == text; break;
        case Qt::MatchFixedString: matches = candidate.compare(text, sensitivity) == 0; break;
        case Qt::MatchStartsWith: matches = candidate.startsWith(text, sensitivity); break;
        case Qt::MatchEndsWith: matches = candidate.endsWith(text, sensitivity); break;
        case Qt::MatchRegularExpression:
        case Qt::MatchWildcard: matches = expression.match(candidate).hasMatch(); break;
        default: matches = candidate.contains(text, sensitivity); break;
        }
        if (matches) return index;
    }
    return -1;
}
void ComboBox::incrementCurrentIndex() {
    const int index = m_popup_visible ? m_highlight : m_index;
    if (index < count() - 1) navigate(index + 1);
}
void ComboBox::decrementCurrentIndex() {
    const int index = m_popup_visible ? m_highlight : m_index;
    if (index > 0) navigate(index - 1);
}
void ComboBox::keySearch(const QString& text) {
    if (text.isEmpty()) return;
    const int index = match(
        (m_popup_visible ? m_highlight : m_index) + 1, text, Qt::MatchStartsWith | Qt::MatchWrap);
    if (index >= 0) navigate(index);
}
void ComboBox::keyPressEvent(QKeyEvent* event) {
    if (! isEnabled() || inputMethodComposing() ||
        event->modifiers().testFlag(Qt::ControlModifier) ||
        event->modifiers().testFlag(Qt::AltModifier) ||
        event->modifiers().testFlag(Qt::MetaModifier)) {
        event->ignore();
        return;
    }
    switch (event->key()) {
    case Qt::Key_Space:
        if (m_editable) {
            event->ignore();
            return;
        }
        if (! event->isAutoRepeat()) setPressed(true);
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (! m_popup_visible) {
            event->ignore();
            return;
        }
        if (! event->isAutoRepeat()) setPressed(true);
        break;
    case Qt::Key_Escape:
    case Qt::Key_Back:
        if (! m_popup_visible) {
            event->ignore();
            return;
        }
        closePopup();
        break;
    case Qt::Key_Up: decrementCurrentIndex(); break;
    case Qt::Key_Down: incrementCurrentIndex(); break;
    case Qt::Key_Home:
        if (m_editable && ! m_popup_visible) {
            event->ignore();
            return;
        }
        navigate(0);
        break;
    case Qt::Key_End:
        if (m_editable && ! m_popup_visible) {
            event->ignore();
            return;
        }
        navigate(count() - 1);
        break;
    default:
        if (m_editable || event->text().isEmpty() || ! event->text().at(0).isPrint()) {
            event->ignore();
            return;
        }
        keySearch(event->text());
    }
    event->accept();
}
void ComboBox::keyReleaseEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) {
        event->accept();
        return;
    }
    if (m_pressed && (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return ||
                      event->key() == Qt::Key_Enter)) {
        QPointer<ComboBox> guard(this);
        setPressed(false);
        if (! guard) return;
        if (m_popup_visible)
            closePopup(true);
        else if (! m_editable && event->key() == Qt::Key_Space)
            openPopup();
        event->accept();
    } else
        event->ignore();
}
void ComboBox::wheelEvent(QWheelEvent* event) {
    if (! m_wheel_enabled || ! isEnabled() || inputMethodComposing() ||
        event->angleDelta().y() == 0) {
        event->ignore();
        return;
    }
    if (event->angleDelta().y() > 0)
        decrementCurrentIndex();
    else
        incrementCurrentIndex();
    event->accept();
}
void ComboBox::inputMethodEvent(QInputMethodEvent* event) {
    if (! m_editable && isEnabled() && ! event->commitString().isEmpty()) {
        keySearch(event->commitString());
        event->accept();
    } else
        event->ignore();
}
bool ComboBox::eventFilter(QObject* watched, QEvent* event) {
    QPointer<ComboBox> guard(this);
    if (watched == m_editor && m_editable) {
        if (event->type() == QEvent::FocusOut) checkFocus();
        if (event->type() == QEvent::MouseButtonRelease && m_popup_visible) closePopup();
        if (! guard) return true;
        if (event->type() == QEvent::KeyRelease && m_pressed) {
            keyReleaseEvent(static_cast<QKeyEvent*>(event));
            if (event->isAccepted()) return true;
        }
        if (event->type() == QEvent::KeyPress) {
            auto key = static_cast<QKeyEvent*>(event);
            m_allow_complete =
                key->key() != Qt::Key_Backspace && key->key() != Qt::Key_Delete &&
                ! (key->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier));
            if (! inputMethodComposing() &&
                (key->key() == Qt::Key_Up || key->key() == Qt::Key_Down ||
                 (m_popup_visible && key->key() != Qt::Key_Space &&
                  (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter ||
                   key->key() == Qt::Key_Escape || key->key() == Qt::Key_Home ||
                   key->key() == Qt::Key_End)))) {
                keyPressEvent(key);
                return key->isAccepted();
            }
        } else if (event->type() == QEvent::InputMethod) {
            m_allow_complete = ! static_cast<QInputMethodEvent*>(event)->commitString().isEmpty();
        }
    }
    return Control::eventFilter(watched, event);
}
void ComboBox::updateEditText() {
    const bool userEdit = std::exchange(m_user_edit, false);
    if (! m_editable || ! m_editor) return;
    const auto text = m_editor->text();
    if (userEdit && m_allow_complete && ! m_editor->isInputMethodComposing() && ! text.isEmpty()) {
        QString completion;
        for (int index = 0; index < count(); ++index) {
            const auto candidate = textAt(index);
            if (candidate.startsWith(text, Qt::CaseInsensitive) &&
                (completion.isEmpty() || candidate.size() < completion.size()))
                completion = candidate;
        }
        if (completion.size() > text.size()) {
            QPointer<ComboBox> guard(this);
            const auto         editor = m_editor;
            editor->setText(text + completion.mid(text.size()));
            if (guard && editor && m_editor == editor)
                editor->select(completion.size(), text.size());
            return;
        }
    }
    setEditText(text);
}

} // namespace qml_material
