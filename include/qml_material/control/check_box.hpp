#pragma once

#include "qml_material/control/button_control.hpp"

namespace qml_material
{
class QML_MATERIAL_API CheckBox : public ButtonControl {
    Q_OBJECT
    QML_NAMED_ELEMENT(CheckBoxBase)
    Q_PROPERTY(bool tristate READ isTristate WRITE setTristate NOTIFY tristateChanged BINDABLE
                   bindableTristate FINAL)
    Q_PROPERTY(Qt::CheckState checkState READ checkState WRITE setCheckState NOTIFY
                   checkStateChanged BINDABLE bindableCheckState FINAL)
public:
    explicit CheckBox(QQuickItem* parent = nullptr): ButtonControl(parent) {}
    bool                      isTristate() const { return m_tristate; }
    void                      setTristate(bool value) { m_tristate = value; }
    QBindable<bool>           bindableTristate() { return QBindable<bool>(&m_tristate); }
    Qt::CheckState            checkState() const { return m_state; }
    void                      setCheckState(Qt::CheckState);
    QBindable<Qt::CheckState> bindableCheckState() { return &m_state; }
    Q_INVOKABLE void          requestCheckState(Qt::CheckState);
    Q_SIGNAL void             tristateChanged();
    Q_SIGNAL void             checkStateChanged();
    Q_SIGNAL void             checkStateChangeRequested(Qt::CheckState checkState);

protected:
    void activate() override;

private:
    Q_OBJECT_BINDABLE_PROPERTY(CheckBox, bool, m_tristate, &CheckBox::tristateChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(CheckBox, Qt::CheckState, m_state, Qt::Unchecked,
                                         &CheckBox::checkStateChanged)
};
} // namespace qml_material
