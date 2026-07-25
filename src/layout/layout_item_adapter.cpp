#include "layout_item_adapter_p.hpp"

#include <algorithm>
#include <private/qquickitem_p.h>
#include <utility>

namespace qml_material
{

LayoutItemAdapter::LayoutItemAdapter(std::function<void(QQuickItem*)> siblingOrderChanged)
    : m_sibling_order_changed(std::move(siblingOrderChanged)) {}

LayoutItemAdapter::~LayoutItemAdapter() {
    for (const auto& item : std::as_const(m_watched_items)) {
        if (item) {
            QQuickItemPrivate::get(item)->removeItemChangeListener(this,
                                                                   QQuickItemPrivate::SiblingOrder);
        }
    }
}

void LayoutItemAdapter::watch(QQuickItem* item) {
    const auto existing = std::find(m_watched_items.cbegin(), m_watched_items.cend(), item);
    if (! item || existing != m_watched_items.cend()) {
        return;
    }
    m_watched_items.push_back(item);
    QQuickItemPrivate::get(item)->addItemChangeListener(this, QQuickItemPrivate::SiblingOrder);
}

void LayoutItemAdapter::unwatch(QQuickItem* item) {
    const auto existing = std::find(m_watched_items.begin(), m_watched_items.end(), item);
    if (existing == m_watched_items.end()) {
        return;
    }
    if (*existing) {
        QQuickItemPrivate::get(*existing)->removeItemChangeListener(
            this, QQuickItemPrivate::SiblingOrder);
    }
    m_watched_items.erase(existing);
}

bool LayoutItemAdapter::participates(const QQuickItem* item) const {
    if (! item) {
        return false;
    }
    const auto* item_private = QQuickItemPrivate::get(item);
    return isExplicitlyVisible(item) && ! item_private->isTransparentForPositioner();
}

bool LayoutItemAdapter::isExplicitlyVisible(const QQuickItem* item) const {
    return item && QQuickItemPrivate::get(item)->explicitVisible;
}

void LayoutItemAdapter::itemSiblingOrderChanged(QQuickItem* item) {
    if (m_sibling_order_changed) {
        m_sibling_order_changed(item);
    }
}

} // namespace qml_material
