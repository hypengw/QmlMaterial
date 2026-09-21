#pragma once
#include "qml_material/control/abstract_button.hpp"

namespace qml_material
{
class QML_MATERIAL_API ItemDelegate : public AbstractButton {
    Q_OBJECT
    QML_NAMED_ELEMENT(ItemDelegateBase)
    Q_PROPERTY(
        bool highlighted READ isHighlighted WRITE setHighlighted NOTIFY highlightedChanged FINAL)
public:
    explicit ItemDelegate(QQuickItem* parent = nullptr): AbstractButton(parent) {
        setFocusPolicy(Qt::NoFocus);
    }
    bool isHighlighted() const { return m_highlighted; }
    void setHighlighted(bool value) {
        if (m_highlighted == value) return;
        m_highlighted = value;
        Q_EMIT highlightedChanged();
    }
    Q_SIGNAL void highlightedChanged();

private:
    bool m_highlighted = false;
};
} // namespace qml_material
