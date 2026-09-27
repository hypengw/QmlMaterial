#pragma once
#include "qml_material/control/abstract_button.hpp"

namespace qml_material
{
class QML_MATERIAL_API ItemDelegate : public AbstractButton {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool highlighted READ isHighlighted WRITE setHighlighted NOTIFY highlightedChanged
                   BINDABLE bindableHighlighted FINAL)
    Q_PROPERTY(qreal implicitDelegateHeight READ implicitDelegateHeight NOTIFY
                   implicitDelegateHeightChanged FINAL)
    Q_PROPERTY(QColor defaultIconColor READ defaultIconColor NOTIFY defaultIconColorChanged BINDABLE
                   bindableDefaultIconColor FINAL)
public:
    explicit ItemDelegate(QQuickItem* parent = nullptr);
    bool            isHighlighted() const { return m_highlighted; }
    void            setHighlighted(bool value) { m_highlighted = value; }
    QBindable<bool> bindableHighlighted() { return QBindable<bool>(&m_highlighted); }
    Q_SIGNAL void   highlightedChanged();

    qreal             implicitDelegateHeight() const;
    QColor            defaultIconColor() const { return m_defaultIconColor.value(); }
    QBindable<QColor> bindableDefaultIconColor() const {
        return QBindable<QColor>(&m_defaultIconColor);
    }
    Q_SIGNAL void implicitDelegateHeightChanged();
    Q_SIGNAL void defaultIconColorChanged();

protected:
    void classBegin() override;

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(ItemDelegate, bool, m_highlighted, false,
                                         &ItemDelegate::highlightedChanged)
    Q_OBJECT_BINDABLE_PROPERTY(ItemDelegate, QColor, m_defaultIconColor,
                               &ItemDelegate::defaultIconColorChanged)
};
} // namespace qml_material
