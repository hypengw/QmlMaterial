#pragma once

#include "qml_material/input/interactive_item.hpp"
#include "qml_material/util/qt.hpp"

namespace qml_material
{
class QML_MATERIAL_API Selectable : public InteractiveItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool selected READ isSelected WRITE setSelected NOTIFY selectedChanged FINAL)
public:
    explicit Selectable(QQuickItem* parent = nullptr): InteractiveItem(parent) {
        m_selectedNotifier = m_selected.addNotifier([this] {
            Q_EMIT selectedChanged();
        });
    }
    bool            isSelected() const { return m_selected; }
    void            setSelected(bool value) { utils::updateBoundValue(m_selected, value); }
    QBindable<bool> bindableSelected() { return &m_selected; }
    Q_SIGNAL void   selectedChanged();
    Q_SIGNAL void   selectionRequested();

protected:
    void activate() override { Q_EMIT selectionRequested(); }

private:
    Q_OBJECT_BINDABLE_PROPERTY(Selectable, bool, m_selected)
    QPropertyNotifier m_selectedNotifier;
};
} // namespace qml_material
