#pragma once

#include "qml_material/control/control.hpp"
#include "qml_material/control/command_button.hpp"
#include <limits>

Q_MOC_INCLUDE("qml_material/control/text_field.hpp")

namespace qml_material
{
class TextField;
/** @ingroup control */
class QML_MATERIAL_API NumberInput : public Control {
    Q_OBJECT
    QML_NAMED_ELEMENT(NumberInputBase)
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged FINAL)
    Q_PROPERTY(qreal from READ from WRITE setFrom NOTIFY constraintsChanged FINAL)
    Q_PROPERTY(qreal to READ to WRITE setTo NOTIFY constraintsChanged FINAL)
    Q_PROPERTY(qreal step READ step WRITE setStep NOTIFY constraintsChanged FINAL)
    Q_PROPERTY(bool wrap READ wrap WRITE setWrap NOTIFY constraintsChanged FINAL)
    Q_PROPERTY(int decimals READ decimals WRITE setDecimals NOTIFY formatChanged FINAL)
    Q_PROPERTY(QString formattedValue READ formattedValue NOTIFY formatChanged FINAL)
    Q_PROPERTY(CommandButton* increaseButton READ increaseButton WRITE setIncreaseButton NOTIFY
                   buttonsChanged FINAL)
    Q_PROPERTY(CommandButton* decreaseButton READ decreaseButton WRITE setDecreaseButton NOTIFY
                   buttonsChanged FINAL)
    Q_PROPERTY(TextField* editor READ editor WRITE setEditor NOTIFY editorChanged FINAL)
public:
    explicit NumberInput(QQuickItem* parent = nullptr);
    ~NumberInput() override;
    qreal            value() const { return m_value; }
    qreal            from() const { return m_from; }
    qreal            to() const { return m_to; }
    qreal            step() const { return m_step; }
    bool             wrap() const { return m_wrap; }
    int              decimals() const { return m_decimals; }
    QString          formattedValue() const;
    CommandButton*   increaseButton() const { return m_increase; }
    CommandButton*   decreaseButton() const { return m_decrease; }
    TextField*       editor() const;
    void             setValue(qreal);
    void             setFrom(qreal);
    void             setTo(qreal);
    void             setStep(qreal);
    void             setWrap(bool);
    void             setDecimals(int);
    void             setIncreaseButton(CommandButton*);
    void             setDecreaseButton(CommandButton*);
    void             setEditor(TextField*);
    Q_INVOKABLE void increase();
    Q_INVOKABLE void decrease();
    Q_INVOKABLE void requestValue(qreal);
    Q_SIGNAL void    valueChanged();
    Q_SIGNAL void    constraintsChanged();
    Q_SIGNAL void    formatChanged();
    Q_SIGNAL void    buttonsChanged();
    Q_SIGNAL void    editorChanged();
    Q_SIGNAL void    valueChangeRequested(qreal value);

protected:
    void keyPressEvent(QKeyEvent*) override;

private:
    void                           bindButtons();
    void                           cancelButtons();
    qreal                          m_value = 0, m_from = -std::numeric_limits<qreal>::infinity(),
                                   m_to = std::numeric_limits<qreal>::infinity(), m_step = 1;
    bool                           m_wrap     = false;
    int                            m_decimals = 0;
    QPointer<CommandButton>        m_increase, m_decrease;
    QPointer<TextField>            m_editor;
    QList<QMetaObject::Connection> m_buttons, m_editorConnections;
};
} // namespace qml_material
