#pragma once

#include <QQmlComponent>
#include <qqml.h>
#include "qml_material/control/panel.hpp"
#include "qml_material/control/abstract_button.hpp"

namespace qml_material
{
class DialogButtonBoxAttached;
class QML_MATERIAL_API DialogButtonBox : public Panel {
    Q_OBJECT
    QML_NAMED_ELEMENT(DialogButtonBoxBase)
    QML_ATTACHED(DialogButtonBoxAttached)
    Q_PROPERTY(int standardButtons READ standardButtons WRITE setStandardButtons NOTIFY
                   standardButtonsChanged FINAL)
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_PROPERTY(
        Qt::Alignment alignment READ alignment WRITE setAlignment NOTIFY alignmentChanged FINAL)
public:
    enum StandardButton
    {
        NoButton        = 0,
        Ok              = 0x400,
        Save            = 0x800,
        SaveAll         = 0x1000,
        Open            = 0x2000,
        Yes             = 0x4000,
        YesToAll        = 0x8000,
        No              = 0x10000,
        NoToAll         = 0x20000,
        Abort           = 0x40000,
        Retry           = 0x80000,
        Ignore          = 0x100000,
        Close           = 0x200000,
        Cancel          = 0x400000,
        Discard         = 0x800000,
        Help            = 0x1000000,
        Apply           = 0x2000000,
        Reset           = 0x4000000,
        RestoreDefaults = 0x8000000
    };
    Q_ENUM(StandardButton)
    enum ButtonRole
    {
        InvalidRole = -1,
        AcceptRole,
        RejectRole,
        DestructiveRole,
        ActionRole,
        HelpRole,
        YesRole,
        NoRole,
        ResetRole,
        ApplyRole
    };
    Q_ENUM(ButtonRole)
    explicit DialogButtonBox(QQuickItem* parent = nullptr);
    ~DialogButtonBox() override;
    static DialogButtonBoxAttached* qmlAttachedProperties(QObject*);
    int                             standardButtons() const { return m_standard_buttons; }
    void                            setStandardButtons(int);
    QQmlComponent*                  delegate() const { return m_delegate; }
    void                            setDelegate(QQmlComponent*);
    int                             count() const { return m_buttons.size(); }
    Qt::Alignment                   alignment() const { return m_alignment; }
    void                            setAlignment(Qt::Alignment);
    Q_INVOKABLE AbstractButton*     standardButton(int) const;
    Q_INVOKABLE AbstractButton*     itemAt(int) const;
    Q_INVOKABLE void                addItem(AbstractButton*);
    Q_INVOKABLE void                removeItem(AbstractButton*);
    Q_SIGNAL void                   standardButtonsChanged();
    Q_SIGNAL void                   delegateChanged();
    Q_SIGNAL void                   countChanged();
    Q_SIGNAL void                   alignmentChanged();
    Q_SIGNAL void                   clicked(AbstractButton*);
    Q_SIGNAL void                   accepted();
    Q_SIGNAL void                   rejected();
    Q_SIGNAL void                   applied();
    Q_SIGNAL void                   reset();
    Q_SIGNAL void                   discarded();
    Q_SIGNAL void                   helpRequested();

protected:
    void componentComplete() override;
    void updatePolish() override;

private:
    void                                 rebuild();
    void                                 observe();
    void                                 activate(AbstractButton*);
    QPointer<QQmlComponent>              m_delegate;
    QHash<int, QPointer<AbstractButton>> m_standard_items;
    QList<QPointer<AbstractButton>>      m_buttons;
    QList<QMetaObject::Connection>       m_connections;
    int                                  m_standard_buttons = NoButton;
    Qt::Alignment                        m_alignment        = Qt::AlignRight;
    bool                                 m_complete         = false;
    bool                                 m_rebuilding       = false;
    quint64                              m_observation      = 0;
};
class QML_MATERIAL_API DialogButtonBoxAttached : public QObject {
    Q_OBJECT
    Q_PROPERTY(DialogButtonBox* buttonBox READ buttonBox NOTIFY buttonBoxChanged FINAL)
    Q_PROPERTY(DialogButtonBox::ButtonRole buttonRole READ buttonRole WRITE setButtonRole NOTIFY
                   buttonRoleChanged FINAL)
public:
    explicit DialogButtonBoxAttached(QObject* parent): QObject(parent) {}
    DialogButtonBox*            buttonBox() const { return m_box; }
    DialogButtonBox::ButtonRole buttonRole() const { return m_role; }
    void                        setButtonRole(DialogButtonBox::ButtonRole role) {
        if (m_role == role) return;
        m_role = role;
        Q_EMIT buttonRoleChanged();
    }
    Q_SIGNAL void buttonBoxChanged();
    Q_SIGNAL void buttonRoleChanged();

private:
    friend class DialogButtonBox;
    QPointer<DialogButtonBox>   m_box;
    DialogButtonBox::ButtonRole m_role = DialogButtonBox::InvalidRole;
};
} // namespace qml_material
QML_DECLARE_TYPEINFO(qml_material::DialogButtonBox, QML_HAS_ATTACHED_PROPERTIES)
