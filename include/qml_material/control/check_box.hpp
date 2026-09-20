#pragma once

#include <QJSValue>
#include "qml_material/control/abstract_button.hpp"

namespace qml_material
{
class QML_MATERIAL_API CheckBox : public AbstractButton {
    Q_OBJECT
    QML_NAMED_ELEMENT(CheckBoxBase)
    Q_PROPERTY(bool tristate READ isTristate WRITE setTristate NOTIFY tristateChanged FINAL)
    Q_PROPERTY(Qt::CheckState checkState READ checkState WRITE setCheckState NOTIFY
                   checkStateChanged FINAL)
    Q_PROPERTY(QJSValue nextCheckState READ nextCheckStateCallback WRITE setNextCheckStateCallback
                   NOTIFY nextCheckStateChanged FINAL)
public:
    explicit CheckBox(QQuickItem* parent = nullptr);
    bool           isTristate() const { return m_tristate; }
    void           setTristate(bool);
    Qt::CheckState checkState() const { return m_state; }
    void           setCheckState(Qt::CheckState);
    QJSValue       nextCheckStateCallback() const { return m_callback; }
    void           setNextCheckStateCallback(const QJSValue&);
    Q_SIGNAL void  tristateChanged();
    Q_SIGNAL void  checkStateChanged();
    Q_SIGNAL void  nextCheckStateChanged();

protected:
    void nextCheckState() override;

private:
    bool           m_tristate = false;
    bool           m_syncing  = false;
    Qt::CheckState m_state    = Qt::Unchecked;
    QJSValue       m_callback;
};
} // namespace qml_material
