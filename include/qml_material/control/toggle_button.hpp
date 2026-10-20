#pragma once

#include "qml_material/control/button_control.hpp"
#include "qml_material/util/qt.hpp"

namespace qml_material
{
class QML_MATERIAL_API ToggleButton : public ButtonControl {
    Q_OBJECT
    QML_NAMED_ELEMENT(ToggleButtonBase)
    Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY checkedChanged FINAL)
public:
    explicit ToggleButton(QQuickItem* parent = nullptr): ButtonControl(parent) {
        // Disconnect before a binding callback can notify a deleted sibling.
        m_checkedNotifier = m_checked.addNotifier([this] {
            checkedChange();
        });
    }
    bool             isChecked() const { return m_checked; }
    void             setChecked(bool value) { utils::updateBoundValue(m_checked, value); }
    QBindable<bool>  bindableChecked() { return &m_checked; }
    Q_INVOKABLE void requestChecked(bool value) {
        if (canActivate() && value != isChecked()) Q_EMIT checkedChangeRequested(value);
    }
    Q_SIGNAL void checkedChanged();
    Q_SIGNAL void checkedChangeRequested(bool checked);

protected:
    void         activate() override { requestChecked(! isChecked()); }
    virtual void checkedChange() { Q_EMIT checkedChanged(); }

private:
    friend class ButtonGroupAttached;
    Q_OBJECT_BINDABLE_PROPERTY(ToggleButton, bool, m_checked)
    QPropertyNotifier m_checkedNotifier;
};
} // namespace qml_material
