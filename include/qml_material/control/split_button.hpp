#pragma once

#include "qml_material/control/command_button.hpp"

namespace qml_material
{
class QML_MATERIAL_API SplitButton : public Control {
    Q_OBJECT
    QML_NAMED_ELEMENT(SplitButtonBase)
    Q_PROPERTY(CommandButton* leadingControl READ leadingControl WRITE setLeadingControl NOTIFY
                   leadingControlChanged FINAL)
public:
    explicit SplitButton(QQuickItem* parent = nullptr): Control(parent) {}
    ~SplitButton() override;
    CommandButton* leadingControl() const { return m_leading; }
    void           setLeadingControl(CommandButton*);
    Q_SIGNAL void  leadingControlChanged();
    Q_SIGNAL void  clicked();

private:
    QPointer<CommandButton>        m_leading;
    QList<QMetaObject::Connection> m_connections;
};
} // namespace qml_material
