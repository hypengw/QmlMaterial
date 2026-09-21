#pragma once

#include "qml_material/control/abstract_button.hpp"

namespace qml_material
{
class QML_MATERIAL_API Button : public AbstractButton {
    Q_OBJECT
    QML_NAMED_ELEMENT(ButtonBase)
    Q_PROPERTY(bool flat READ isFlat WRITE setFlat NOTIFY flatChanged FINAL)
    Q_PROPERTY(
        bool highlighted READ isHighlighted WRITE setHighlighted NOTIFY highlightedChanged FINAL)
public:
    explicit Button(QQuickItem* parent = nullptr): AbstractButton(parent) {}
    bool isFlat() const { return m_flat; }
    bool isHighlighted() const { return m_highlighted; }
    void setFlat(bool value) {
        if (m_flat == value) return;
        m_flat = value;
        Q_EMIT flatChanged();
    }
    void setHighlighted(bool value) {
        if (m_highlighted == value) return;
        m_highlighted = value;
        Q_EMIT highlightedChanged();
    }
    Q_SIGNAL void flatChanged();
    Q_SIGNAL void highlightedChanged();

private:
    bool m_flat        = false;
    bool m_highlighted = false;
};
} // namespace qml_material
