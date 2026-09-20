#include "qml_material/control/segmented_button_group.hpp"
#include <algorithm>

namespace qml_material
{
SegmentedButtonGroup::SegmentedButtonGroup(QQuickItem* parent)
    : Container(parent), m_group(new ButtonGroup(this)) {
    connect(m_group, &ButtonGroup::exclusiveChanged, this, &SegmentedButtonGroup::exclusiveChanged);
    connect(this, &Control::spacingChanged, this, &QQuickItem::polish);
    connect(this, &Control::mirroredChanged, this, &QQuickItem::polish);
}
bool SegmentedButtonGroup::isContent(QQuickItem* item) const {
    return qobject_cast<AbstractButton*>(item);
}
void SegmentedButtonGroup::itemAdded(QQuickItem* item) {
    m_group->addButton(qobject_cast<AbstractButton*>(item));
}
void SegmentedButtonGroup::itemRemoved(QQuickItem* item) {
    m_group->removeButton(qobject_cast<AbstractButton*>(item));
}
void SegmentedButtonGroup::updatePolish() {
    QPointer<SegmentedButtonGroup> guard(this);
    Container::updatePolish();
    if (! guard) return;
    const auto layoutRevision = revision();
    const auto snapshot       = items();
    qreal      width = 0, height = 0;
    int        itemCount = 0;
    for (auto item : snapshot) {
        if (! item) continue;
        width += item->width();
        height = std::max(height, item->height());
        ++itemCount;
    }
    width   = std::max<qreal>(0, width + std::max(0, itemCount - 1) * spacing());
    qreal x = mirrored() ? width : 0;
    for (auto item : snapshot) {
        if (! item) continue;
        if (mirrored()) x -= item->width();
        item->setPosition(QPointF(x, 0));
        if (! guard) return;
        if (layoutRevision != revision()) {
            polish();
            return;
        }
        if (item) x += mirrored() ? -spacing() : item->width() + spacing();
    }
    setImplicitContentSize(QSizeF(width, height));
}
} // namespace qml_material
