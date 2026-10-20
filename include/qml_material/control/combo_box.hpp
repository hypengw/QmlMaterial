#pragma once

#include "qml_material/control/control.hpp"
#include "qml_material/control/popup.hpp"
#include "qml_material/input/input_host.hpp"
#include "qml_material/model/combo_box_model.hpp"
#include <QQmlComponent>
#include <QValidator>
#include <QtQuick/private/qquicktextinput_p.h>

namespace qml_material
{

class QML_MATERIAL_API ComboBox : public Control {
    Q_OBJECT
    QML_NAMED_ELEMENT(ComboBoxBase)
    Q_PROPERTY(Popup* popup READ popup WRITE setPopup NOTIFY popupChanged FINAL)
    Q_PROPERTY(
        QQuickItem* indicator READ indicator WRITE setIndicator NOTIFY indicatorChanged FINAL)
    Q_PROPERTY(bool popupVisible READ popupVisible NOTIFY popupVisibleChanged FINAL)
    Q_PROPERTY(bool popupActiveFocus READ popupActiveFocus NOTIFY popupActiveFocusChanged FINAL)
    Q_PROPERTY(InteractionSource* interactionSource READ interactionSource WRITE
                   setInteractionSource NOTIFY interactionSourceChanged FINAL)
    Q_PROPERTY(bool focusVisible READ focusVisible NOTIFY focusVisibleChanged FINAL)
    Q_PROPERTY(int highlightedIndex READ highlightedIndex NOTIFY highlightedIndexChanged FINAL)
    Q_PROPERTY(
        bool wheelEnabled READ wheelEnabled WRITE setWheelEnabled NOTIFY wheelEnabledChanged FINAL)
    Q_PROPERTY(bool editable READ editable WRITE setEditable NOTIFY editableChanged FINAL)
    Q_PROPERTY(QString editText READ editText WRITE setEditText RESET resetEditText NOTIFY
                   editTextChanged FINAL)
    Q_PROPERTY(bool acceptableInput READ acceptableInput NOTIFY acceptableInputChanged BINDABLE
                   bindableAcceptableInput FINAL)
    Q_PROPERTY(bool inputMethodComposing READ inputMethodComposing NOTIFY
                   inputMethodComposingChanged FINAL)
    Q_PROPERTY(
        QValidator* validator READ validator WRITE setValidator NOTIFY validatorChanged FINAL)
    Q_PROPERTY(Qt::InputMethodHints inputMethodHints READ inputMethodHints WRITE setInputMethodHints
                   NOTIFY inputMethodHintsChanged FINAL)
    Q_PROPERTY(bool selectTextByMouse READ selectTextByMouse WRITE setSelectTextByMouse NOTIFY
                   selectTextByMouseChanged FINAL)
    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged FINAL)
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    Q_PROPERTY(QObject* delegateModel READ delegateModel NOTIFY delegateModelChanged FINAL)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_PROPERTY(QString textRole READ textRole WRITE setTextRole NOTIFY textRoleChanged FINAL)
    Q_PROPERTY(QString valueRole READ valueRole WRITE setValueRole NOTIFY valueRoleChanged FINAL)
    Q_PROPERTY(
        int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged FINAL)
    Q_PROPERTY(QVariant currentValue READ currentValue WRITE setCurrentValue NOTIFY
                   currentValueChanged FINAL)
    Q_PROPERTY(QString currentText READ currentText NOTIFY currentTextChanged FINAL)
    Q_PROPERTY(QString displayText READ displayText WRITE setDisplayText RESET resetDisplayText
                   NOTIFY displayTextChanged FINAL)
public:
    explicit ComboBox(QQuickItem* parent = nullptr);
    ~ComboBox() override;
    bool               popupVisible() const { return m_popup_visible; }
    bool               popupActiveFocus() const { return m_popup_focus; }
    int                highlightedIndex() const { return m_highlight; }
    Popup*             popup() const { return m_popup; }
    void               setPopup(Popup*);
    QQuickItem*        indicator() const { return m_indicator; }
    void               setIndicator(QQuickItem*);
    InteractionSource* interactionSource() { return m_input.source(); }
    void               setInteractionSource(InteractionSource* value) { m_input.setSource(value); }
    bool               focusVisible() const { return m_input.focusVisible(); }
    Q_SIGNAL void      popupChanged();
    Q_SIGNAL void      indicatorChanged();
    Q_SIGNAL void      interactionSourceChanged();
    Q_SIGNAL void      focusVisibleChanged();
    Q_INVOKABLE void   openPopup();
    Q_INVOKABLE void   closePopup(bool accept = false);
    Q_INVOKABLE void   highlightIndex(int);
    Q_SIGNAL void      popupVisibleChanged();
    Q_SIGNAL void      popupActiveFocusChanged();
    Q_SIGNAL void      highlightedIndexChanged();
    Q_SIGNAL void      highlighted(int index);
    bool               wheelEnabled() const { return m_wheel_enabled; }
    void               setWheelEnabled(bool);
    Q_SIGNAL void      wheelEnabledChanged();
    Q_INVOKABLE int    find(const QString&, Qt::MatchFlags flags = Qt::MatchExactly) const;
    Q_INVOKABLE void   incrementCurrentIndex();
    Q_INVOKABLE void   decrementCurrentIndex();
    bool               editable() const { return m_editable; }
    void               setEditable(bool);
    QString            editText() const { return m_edit_text; }
    void               setEditText(const QString&);
    void               resetEditText() { setEditText({}); }
    bool               acceptableInput() const;
    QBindable<bool>    bindableAcceptableInput() const { return QBindable<bool>(&m_acceptable); }
    bool               inputMethodComposing() const;
    QValidator*        validator() const { return m_validator; }
    void               setValidator(QValidator*);
    Qt::InputMethodHints inputMethodHints() const { return m_input_hints; }
    void                 setInputMethodHints(Qt::InputMethodHints);
    bool                 selectTextByMouse() const { return m_select_by_mouse; }
    void                 setSelectTextByMouse(bool);
    Q_INVOKABLE void     selectAll();
    Q_INVOKABLE void     acceptInput();
    Q_SIGNAL void        editableChanged();
    Q_SIGNAL void        editTextChanged();
    Q_SIGNAL void        acceptableInputChanged();
    Q_SIGNAL void        inputMethodComposingChanged();
    Q_SIGNAL void        validatorChanged();
    Q_SIGNAL void        inputMethodHintsChanged();
    Q_SIGNAL void        selectTextByMouseChanged();
    Q_SIGNAL void        accepted();
    QVariant             model() const;
    void                 setModel(const QVariant&);
    QQmlComponent*       delegate() const;
    void                 setDelegate(QQmlComponent*);
    QObject*             delegateModel() const;
    int                  count() const;
    QString              textRole() const;
    void                 setTextRole(const QString&);
    QString              valueRole() const;
    void                 setValueRole(const QString&);
    int                  currentIndex() const { return m_index; }
    void                 setCurrentIndex(int);
    QVariant             currentValue() const { return m_value; }
    void                 setCurrentValue(const QVariant&);
    QString              currentText() const { return m_text; }
    QString              displayText() const { return m_display; }
    void                 setDisplayText(const QString&);
    void                 resetDisplayText();
    Q_INVOKABLE QString  textAt(int) const;
    Q_INVOKABLE QVariant valueAt(int) const;
    Q_INVOKABLE int      indexOfValue(const QVariant&) const;
    Q_INVOKABLE void     activateIndex(int);

    Q_SIGNAL void modelChanged();
    Q_SIGNAL void delegateChanged();
    Q_SIGNAL void delegateModelChanged();
    Q_SIGNAL void countChanged();
    Q_SIGNAL void textRoleChanged();
    Q_SIGNAL void valueRoleChanged();
    Q_SIGNAL void currentIndexChanged();
    Q_SIGNAL void currentValueChanged();
    Q_SIGNAL void currentTextChanged();
    Q_SIGNAL void displayTextChanged();
    Q_SIGNAL void selectionChangeRequested(int index, const QVariant& value);

protected:
    bool event(QEvent*) override;
    void hoverEnterEvent(QHoverEvent*) override;
    void hoverMoveEvent(QHoverEvent*) override;
    void hoverLeaveEvent(QHoverEvent*) override;
    void classBegin() override;
    void componentComplete() override;
    void focusInEvent(QFocusEvent*) override;
    void focusOutEvent(QFocusEvent*) override;
    bool eventFilter(QObject*, QEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseUngrabEvent() override;
    void touchEvent(QTouchEvent*) override;
    void touchUngrabEvent() override;
    void wheelEvent(QWheelEvent*) override;
    void inputMethodEvent(QInputMethodEvent*) override;

private:
    void                           delegateClicked();
    void                           setHighlight(int, bool notify = false);
    void                           navigate(int);
    void                           checkFocus();
    void                           syncPopupState();
    bool                           m_popup_visible = false;
    bool                           m_popup_focus   = false;
    int                            m_highlight     = -1;
    InputHost                      m_input;
    QPointer<Popup>                m_popup;
    QPointer<QQuickItem>           m_indicator;
    QList<QMetaObject::Connection> m_popupConnections;
    QMetaObject::Connection        m_indicatorDestroyed;
    quint64                        m_revision = 0;
    enum class Selection
    {
        None,
        Index,
        Value
    };
    void                           ensureModel();
    void                           synchronize();
    void                           publish(int, const QVariant&);
    std::unique_ptr<ComboBoxModel> m_data;
    Selection                      m_selection = Selection::None;
    int                            m_index     = -1;
    QVariant                       m_value;
    QString                        m_text, m_display;
    std::optional<QString>         m_explicit_display;
    bool                           m_ready = false;
    void                           attachEditor();
    void                           updateEditText();
    int                            match(int start, const QString&, Qt::MatchFlags) const;
    void                           keySearch(const QString&);
    bool                           m_wheel_enabled  = false;
    bool                           m_user_edit      = false;
    bool                           m_allow_complete = true;
    void                           updateEditorState();
    int                            findEditText(const QString&) const;
    QPointer<QQuickTextInput>      m_editor;
    QList<QMetaObject::Connection> m_editor_connections;
    QPointer<QValidator>           m_validator;
    QMetaObject::Connection        m_validator_connection;
    QString                        m_edit_text;
    Qt::InputMethodHints           m_input_hints     = Qt::ImhNoPredictiveText;
    bool                           m_editable        = false;
    bool                           m_select_by_mouse = false;
    bool                           m_accepting       = false;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(ComboBox, bool, m_acceptable, true,
                                         &ComboBox::acceptableInputChanged)
    bool m_composing = false;
};

} // namespace qml_material
