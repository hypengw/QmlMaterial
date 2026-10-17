#include "qml_material/core/item_activity.hpp"
#include <QPointer>
#include <QQuickItem>
#include <QQuickWindow>
#include <private/qquickitem_p.h>
#include <private/qqmldata_p.h>

namespace qml_material
{
namespace
{
class ActivityProbe;
class ProbePrivate : public QQuickItemPrivate {
public:
    explicit ProbePrivate(ItemActivity* observer): observer(observer) {}
    QPointer<ItemActivity> observer;
    bool                   transformChanged(QQuickItem* item) override;
};
class ActivityProbe : public QQuickItem {
public:
    ActivityProbe(QQuickItem* target, ItemActivity* observer)
        : QQuickItem(*new ProbePrivate(observer), target) {
        setFlag(ItemObservesViewport);
    }
};
bool ProbePrivate::transformChanged(QQuickItem* item) {
    const auto result = QQuickItemPrivate::transformChanged(item);
    if (observer) observer->refresh();
    return result;
}
} // namespace
struct ItemActivity::Private {
    QPointer<QQuickItem>           target;
    QPointer<ActivityProbe>        probe;
    QList<QMetaObject::Connection> connections;
    bool                           active = false;
};
ItemActivity::ItemActivity(QQuickItem* target): QObject(), d(std::make_unique<Private>()) {
    d->target = target;
    d->probe  = new ActivityProbe(target, this);
    connect(target, &QQuickItem::windowChanged, this, &ItemActivity::observe);
    observe();
}
ItemActivity::~ItemActivity() {
    for (const auto& c : d->connections) disconnect(c);
    if (d->probe) static_cast<ProbePrivate*>(QQuickItemPrivate::get(d->probe))->observer.clear();
    delete d->probe;
}
bool ItemActivity::active() const { return d->active; }
void ItemActivity::observe() {
    for (const auto& c : d->connections) disconnect(c);
    d->connections.clear();
    for (auto* item = d->target.data(); item; item = item->parentItem()) {
        const auto observe = [this, item](auto signal) {
            d->connections.append(connect(item, signal, this, &ItemActivity::refresh));
        };
        observe(&QQuickItem::widthChanged);
        observe(&QQuickItem::heightChanged);
        observe(&QQuickItem::xChanged);
        observe(&QQuickItem::yChanged);
        observe(&QQuickItem::visibleChanged);
        observe(&QQuickItem::opacityChanged);
        observe(&QQuickItem::clipChanged);
        d->connections.append(
            connect(item, &QQuickItem::parentChanged, this, &ItemActivity::observe));
    }
    if (d->target && d->target->window()) {
        d->connections.append(connect(
            d->target->window(), &QWindow::visibilityChanged, this, &ItemActivity::refresh));
    }
    refresh();
}
void ItemActivity::refresh() {
    auto* target = d->target.data();
    if (! target || QQmlData::wasDeleted(target)) return;
    auto*  window = target->window();
    QRectF area;
    if (target->isVisible() && window && window->isVisible() &&
        window->visibility() != QWindow::Minimized) {
        auto* root = window->contentItem();
        area       = target->mapRectToItem(root, target->boundingRect())
                         .intersected(QRectF(0, 0, root->width(), root->height()));
        for (auto* item = target; item && ! area.isEmpty(); item = item->parentItem()) {
            if (item->opacity() <= 0) area = {};
            if (item != target && item->clip())
                area = area.intersected(item->mapRectToItem(root, item->clipRect()));
        }
    }
    const bool active = ! area.isEmpty();
    if (d->active == active) return;
    d->active = active;
    Q_EMIT changed();
}
} // namespace qml_material
