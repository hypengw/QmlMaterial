#pragma once

#include <QSet>
#include <QQmlListProperty>
#include <QQmlParserStatus>
#include "qml_material/control/toggle_button.hpp"

namespace qml_material
{
class ButtonGroupAttached;
class QML_MATERIAL_API ButtonGroup : public QObject, public QQmlParserStatus {
    Q_OBJECT
    QML_NAMED_ELEMENT(ButtonGroup)
    QML_ATTACHED(ButtonGroupAttached)
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QQmlListProperty<ToggleButton> buttons READ buttons NOTIFY buttonsChanged FINAL)
    Q_PROPERTY(ToggleButton* checkedButton READ checkedButton WRITE setCheckedButton NOTIFY
                   checkedButtonsChanged FINAL)
    Q_PROPERTY(QList<qml_material::ToggleButton*> checkedButtons READ checkedButtons WRITE
                   setCheckedButtons NOTIFY checkedButtonsChanged FINAL)
    Q_PROPERTY(bool exclusive READ isExclusive WRITE setExclusive NOTIFY exclusiveChanged FINAL)
    Q_PROPERTY(Qt::CheckState checkState READ checkState WRITE setCheckState NOTIFY
                   checkStateChanged FINAL)
public:
    explicit ButtonGroup(QObject* parent = nullptr): QObject(parent) {}
    ~ButtonGroup() override;
    static ButtonGroupAttached*    qmlAttachedProperties(QObject*);
    QQmlListProperty<ToggleButton> buttons();
    ToggleButton*                  checkedButton() const;
    void                           setCheckedButton(ToggleButton*);
    QList<ToggleButton*>           checkedButtons() const;
    void                           setCheckedButtons(const QList<ToggleButton*>&);
    bool                           isExclusive() const { return m_exclusive; }
    void                           setExclusive(bool);
    Qt::CheckState                 checkState() const;
    void                           setCheckState(Qt::CheckState);
    Q_INVOKABLE void               addButton(ToggleButton*);
    Q_INVOKABLE void               removeButton(ToggleButton*);
    Q_INVOKABLE void               requestChecked(ToggleButton*, bool);
    Q_SIGNAL void                  buttonsChanged();
    Q_SIGNAL void                  checkedButtonsChanged();
    Q_SIGNAL void                  exclusiveChanged();
    Q_SIGNAL void                  checkStateChanged();
    void                           classBegin() override { m_complete = false; }
    void                           componentComplete() override;

private:
    friend class ButtonGroupAttached;
    bool                                        isChecked(const ButtonGroupAttached*) const;
    void                                        commit(QSet<const ButtonGroupAttached*>);
    void                                        notifySelection();
    void                                        clear();
    QList<QPointer<ButtonGroupAttached>>        m_members;
    QProperty<QSet<const ButtonGroupAttached*>> m_checked;
    QList<QPointer<ToggleButton>>               m_initialButtons;
    bool                                        m_complete         = true;
    bool                                        m_initialSelection = false;
    bool                                        m_exclusive        = true;
    bool                                        m_destroying       = false;
    bool                                        m_notifying        = false;
    bool                                        m_dirty            = false;
    quint64                                     m_revision         = 0;
};

class QML_MATERIAL_API ButtonGroupAttached : public QObject {
    Q_OBJECT
    Q_PROPERTY(ButtonGroup* group READ group WRITE setGroup NOTIFY groupChanged FINAL)
    Q_PROPERTY(bool checked READ isChecked NOTIFY checkedChanged BINDABLE bindableChecked FINAL)
public:
    explicit ButtonGroupAttached(ToggleButton*);
    ~ButtonGroupAttached() override;
    ButtonGroup*    group() const { return m_group.value(); }
    void            setGroup(ButtonGroup*);
    bool            isChecked() const { return m_checked; }
    QBindable<bool> bindableChecked() const { return QBindable<bool>(&m_checked); }
    ToggleButton*   button() const { return m_button; }
    Q_SIGNAL void   groupChanged();
    Q_SIGNAL void   checkedChanged();

private:
    friend class ButtonGroup;
    QPointer<ToggleButton>           m_button;
    QProperty<QPointer<ButtonGroup>> m_group;
    QProperty<bool>                  m_checked;
    QPropertyNotifier                m_checkedNotifier;
    QMetaObject::Connection          m_requestConnection;
    quint64                          m_revision   = 0;
    bool                             m_destroying = false;
};
} // namespace qml_material
QML_DECLARE_TYPEINFO(qml_material::ButtonGroup, QML_HAS_ATTACHED_PROPERTIES)
