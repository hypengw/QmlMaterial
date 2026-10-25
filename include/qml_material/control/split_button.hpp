#pragma once

#include "qml_material/control/command_button.hpp"

namespace qml_material
{
class QML_MATERIAL_API SplitButton : public Control {
    Q_OBJECT
    QML_NAMED_ELEMENT(SplitButtonBase)
    Q_PROPERTY(CommandButton* leadingControl READ leadingControl WRITE setLeadingControl NOTIFY
                   leadingControlChanged FINAL)
    Q_PROPERTY(CommandButton* trailingControl READ trailingControl WRITE setTrailingControl NOTIFY
                   trailingControlChanged FINAL)
public:
    explicit SplitButton(QQuickItem* parent = nullptr): Control(parent) {}
    ~SplitButton() override;
    CommandButton* leadingControl() const { return m_leading; }
    void           setLeadingControl(CommandButton*);
    CommandButton* trailingControl() const { return m_trailing; }
    void           setTrailingControl(CommandButton*);
    Q_SIGNAL void  leadingControlChanged();
    Q_SIGNAL void  trailingControlChanged();
    Q_SIGNAL void  clicked();
    Q_SIGNAL void  trailingClicked();

private:
    void                           setControl(CommandButton*, bool trailing);
    QPointer<CommandButton>        m_leading, m_trailing;
    QList<QMetaObject::Connection> m_leadingConnections, m_trailingConnections;
};
} // namespace qml_material
