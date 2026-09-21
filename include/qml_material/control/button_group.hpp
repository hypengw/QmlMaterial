#pragma once

#include <QHash>
#include <QQmlListProperty>
#include <qqml.h>
#include "qml_material/control/abstract_button.hpp"
#include "qml_material/util/selection_set.hpp"

namespace qml_material
{
class ButtonGroupAttached;
class QML_MATERIAL_API ButtonGroup : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(ButtonGroup)
    QML_ATTACHED(ButtonGroupAttached)

    Q_PROPERTY(QQmlListProperty<AbstractButton> buttons READ buttons NOTIFY buttonsChanged FINAL)
    Q_PROPERTY(AbstractButton* checkedButton READ checkedButton WRITE setCheckedButton NOTIFY
                   checkedButtonChanged FINAL)
    Q_PROPERTY(bool exclusive READ isExclusive WRITE setExclusive NOTIFY exclusiveChanged FINAL)
    Q_PROPERTY(Qt::CheckState checkState READ checkState WRITE setCheckState NOTIFY
                   checkStateChanged FINAL)
public:
    explicit ButtonGroup(QObject* parent = nullptr): QObject(parent) {}
    ~ButtonGroup() override;
    static ButtonGroupAttached*      qmlAttachedProperties(QObject*);
    QQmlListProperty<AbstractButton> buttons();
    AbstractButton*                  checkedButton() const { return m_selection.selected; }
    void                             setCheckedButton(AbstractButton*);
    bool                             isExclusive() const { return m_selection.exclusive; }
    void                             setExclusive(bool);
    Qt::CheckState                   checkState() const { return m_check_state; }
    void                             setCheckState(Qt::CheckState);
    Q_INVOKABLE void                 addButton(AbstractButton*);
    Q_INVOKABLE void                 removeButton(AbstractButton*);
    Q_SIGNAL void                    buttonsChanged();
    Q_SIGNAL void                    checkedButtonChanged();
    Q_SIGNAL void                    exclusiveChanged();
    Q_SIGNAL void                    checkStateChanged();
    Q_SIGNAL void                    clicked(AbstractButton* button);

private:
    void                                update(AbstractButton* preferred = nullptr);
    void                                clear();
    utils::SelectionSet<AbstractButton> m_selection;
    QHash<AbstractButton*, QList<QMetaObject::Connection>> m_connections;
    AbstractButton*                                        m_notified_selected = nullptr;
    Qt::CheckState                                         m_check_state       = Qt::Unchecked;
    bool                                                   m_destroying        = false;
};

class QML_MATERIAL_API ButtonGroupAttached : public QObject {
    Q_OBJECT
    Q_PROPERTY(ButtonGroup* group READ group WRITE setGroup NOTIFY groupChanged FINAL)
public:
    explicit ButtonGroupAttached(QObject*);
    ButtonGroup*  group() const;
    void          setGroup(ButtonGroup*);
    Q_SIGNAL void groupChanged();

private:
    QPointer<AbstractButton> m_member;
};
} // namespace qml_material
QML_DECLARE_TYPEINFO(qml_material::ButtonGroup, QML_HAS_ATTACHED_PROPERTIES)
