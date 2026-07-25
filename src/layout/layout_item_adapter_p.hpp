#pragma once

#include <QPointer>
#include <QQuickItem>
#include <QVector>
#include <functional>
#include <private/qquickitemchangelistener_p.h>

namespace qml_material
{

class LayoutItemAdapter final : public QQuickItemChangeListener {
public:
    explicit LayoutItemAdapter(std::function<void(QQuickItem*)> siblingOrderChanged);
    ~LayoutItemAdapter() override;

    void watch(QQuickItem* item);
    void unwatch(QQuickItem* item);
    bool participates(const QQuickItem* item) const;
    bool isExplicitlyVisible(const QQuickItem* item) const;

private:
    void itemSiblingOrderChanged(QQuickItem* item) override;

    std::function<void(QQuickItem*)> m_sibling_order_changed;
    QVector<QPointer<QQuickItem>>    m_watched_items;
};

} // namespace qml_material
