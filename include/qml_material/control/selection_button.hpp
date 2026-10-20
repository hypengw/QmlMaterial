#pragma once

#include "qml_material/control/button_control.hpp"
#include "qml_material/util/qt.hpp"

namespace qml_material
{
class QML_MATERIAL_API SelectionButton : public ButtonControl {
    Q_OBJECT
    QML_NAMED_ELEMENT(SelectionButtonBase)
    Q_PROPERTY(bool selected READ isSelected WRITE setSelected NOTIFY selectedChanged FINAL)
public:
    explicit SelectionButton(QQuickItem* parent = nullptr): ButtonControl(parent) {
        m_selectedNotifier = m_selected.addNotifier([this] {
            Q_EMIT selectedChanged();
        });
    }
    bool             isSelected() const { return m_selected; }
    void             setSelected(bool value) { utils::updateBoundValue(m_selected, value); }
    QBindable<bool>  bindableSelected() { return &m_selected; }
    Q_INVOKABLE void requestSelection() {
        if (canActivate()) Q_EMIT selectionRequested();
    }
    Q_SIGNAL void selectedChanged();
    Q_SIGNAL void selectionRequested();

protected:
    void activate() override { requestSelection(); }

private:
    Q_OBJECT_BINDABLE_PROPERTY(SelectionButton, bool, m_selected)
    QPropertyNotifier m_selectedNotifier;
};
} // namespace qml_material
