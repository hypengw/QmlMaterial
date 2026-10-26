#pragma once

#include "qml_material/view/lazy_view.hpp"

namespace qml_material
{
class QML_MATERIAL_API LazyList : public LazyView {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing NOTIFY spacingChanged FINAL)
    Q_PROPERTY(qreal estimatedItemExtent READ estimatedItemExtent WRITE setEstimatedItemExtent
                   NOTIFY estimatedItemExtentChanged FINAL)
public:
    explicit LazyList(QQuickItem* parent = nullptr);
    qreal         spacing() const;
    void          setSpacing(qreal);
    qreal         estimatedItemExtent() const;
    void          setEstimatedItemExtent(qreal);
    Q_SIGNAL void spacingChanged();
    Q_SIGNAL void estimatedItemExtentChanged();
};
} // namespace qml_material
