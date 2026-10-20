#pragma once

#include "qml_material/input/interactive_item.hpp"
#include "qml_material/util/qt.hpp"

namespace qml_material
{
class QML_MATERIAL_API Toggleable : public InteractiveItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY checkedChanged FINAL)
public:
    explicit Toggleable(QQuickItem* parent = nullptr): InteractiveItem(parent) {
        m_checkedNotifier = m_checked.addNotifier([this] {
            Q_EMIT checkedChanged();
        });
    }
    bool            isChecked() const { return m_checked; }
    void            setChecked(bool value) { utils::updateBoundValue(m_checked, value); }
    QBindable<bool> bindableChecked() { return &m_checked; }
    Q_SIGNAL void   checkedChanged();
    Q_SIGNAL void   checkedChangeRequested(bool checked);

protected:
    void activate() override { Q_EMIT checkedChangeRequested(! isChecked()); }

private:
    Q_OBJECT_BINDABLE_PROPERTY(Toggleable, bool, m_checked)
    QPropertyNotifier m_checkedNotifier;
};
} // namespace qml_material
