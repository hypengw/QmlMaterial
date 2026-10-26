#pragma once

#include "qml_material/view/lazy_view.hpp"

namespace qml_material
{
class QML_MATERIAL_API LazyGrid : public LazyView {
    Q_OBJECT
    QML_ELEMENT
    // Zero fixedSlotCount selects adaptive sizing; fixed counts divide the available cross axis.
    Q_PROPERTY(int fixedSlotCount READ fixedSlotCount WRITE setFixedSlotCount NOTIFY
                   fixedSlotCountChanged FINAL)
    Q_PROPERTY(qreal minimumCellExtent READ minimumCellExtent WRITE setMinimumCellExtent NOTIFY
                   minimumCellExtentChanged FINAL)
    Q_PROPERTY(qreal itemExtent READ itemExtent WRITE setItemExtent NOTIFY itemExtentChanged FINAL)
    Q_PROPERTY(
        qreal mainSpacing READ mainSpacing WRITE setMainSpacing NOTIFY mainSpacingChanged FINAL)
    Q_PROPERTY(
        qreal crossSpacing READ crossSpacing WRITE setCrossSpacing NOTIFY crossSpacingChanged FINAL)
    Q_PROPERTY(int slotCount READ slotCount NOTIFY layoutChanged FINAL)
    Q_PROPERTY(qreal cellCrossExtent READ cellCrossExtent NOTIFY layoutChanged FINAL)
    Q_PROPERTY(Qt::LayoutDirection layoutDirection READ layoutDirection WRITE setLayoutDirection
                   NOTIFY layoutDirectionChanged FINAL)
public:
    explicit LazyGrid(QQuickItem* parent = nullptr);
    int                 fixedSlotCount() const;
    void                setFixedSlotCount(int);
    qreal               minimumCellExtent() const;
    void                setMinimumCellExtent(qreal);
    qreal               itemExtent() const;
    void                setItemExtent(qreal);
    qreal               mainSpacing() const;
    void                setMainSpacing(qreal);
    qreal               crossSpacing() const;
    void                setCrossSpacing(qreal);
    int                 slotCount() const;
    qreal               cellCrossExtent() const;
    Qt::LayoutDirection layoutDirection() const;
    void                setLayoutDirection(Qt::LayoutDirection);
    Q_SIGNAL void       fixedSlotCountChanged();
    Q_SIGNAL void       minimumCellExtentChanged();
    Q_SIGNAL void       itemExtentChanged();
    Q_SIGNAL void       mainSpacingChanged();
    Q_SIGNAL void       crossSpacingChanged();
    Q_SIGNAL void       layoutDirectionChanged();
};
} // namespace qml_material
