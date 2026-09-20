#pragma once
#include "qml_material/control/radio_button.hpp"

namespace qml_material
{
class QML_MATERIAL_API TabButton : public RadioButton {
    Q_OBJECT
    QML_NAMED_ELEMENT(TabButtonBase)
    Q_PROPERTY(bool fillWidth READ fillWidth WRITE setFillWidth NOTIFY fillWidthChanged FINAL)
    Q_PROPERTY(bool fillHeight READ fillHeight WRITE setFillHeight NOTIFY fillHeightChanged FINAL)
public:
    explicit TabButton(QQuickItem* parent = nullptr): RadioButton(parent) {}
    bool fillWidth() const { return m_fill_width; }
    bool fillHeight() const { return m_fill_height; }
    void setFillWidth(bool value) {
        if (m_fill_width == value) return;
        m_fill_width = value;
        Q_EMIT fillWidthChanged();
    }
    void setFillHeight(bool value) {
        if (m_fill_height == value) return;
        m_fill_height = value;
        Q_EMIT fillHeightChanged();
    }
    Q_SIGNAL void fillWidthChanged();
    Q_SIGNAL void fillHeightChanged();

private:
    bool m_fill_width  = true;
    bool m_fill_height = true;
};
} // namespace qml_material
