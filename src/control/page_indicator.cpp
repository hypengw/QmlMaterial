#include "qml_material/control/page_indicator.hpp"
#include <QMouseEvent>
#include <QTouchEvent>
#include <QLineF>
#include <QMetaProperty>
#include <QQmlEngine>
#include <QQmlInfo>
#include <limits>
#include <utility>

namespace qml_material
{

PageIndicator::PageIndicator(QQuickItem* parent): Control(parent) {
    connect(this, &Control::contentItemChanged, this, [this]() {
        cancelPress();
        for (const auto& entry : m_items)
            if (entry.item) entry.item->setParentItem(contentItem());
        scheduleDelegates();
    });
}

PageIndicator::~PageIndicator() {
    for (const auto& connection : m_delegate_connections) disconnect(connection);
    // Delegates may have QML handlers that refer back to the dying control.
    for (const auto& entry : m_items)
        if (entry.item) disconnect(entry.item, nullptr, this, nullptr);
}

void PageIndicator::setCount(int value) {
    value = qMax(0, value);
    if (m_count == value) return;
    m_count = value;
    cancelPress();
    scheduleDelegates();
    Q_EMIT countChanged();
}

void PageIndicator::setCurrentIndex(int value) {
    if (m_current_index == value) return;
    m_current_index = value;
    Q_EMIT currentIndexChanged();
}

void PageIndicator::setInteractive(bool value) {
    if (m_interactive == value) return;
    m_interactive = value;
    setAcceptedMouseButtons(value ? Qt::LeftButton : Qt::NoButton);
    setAcceptTouchEvents(value);
    if (! value) cancelPress();
    Q_EMIT interactiveChanged();
}

void PageIndicator::setDelegate(QQmlComponent* value) {
    if (m_delegate == value) return;
    for (const auto& connection : m_delegate_connections) disconnect(connection);
    m_delegate_connections.clear();
    m_delegate = value;
    m_rebuild  = true;
    cancelPress();
    if (value) {
        m_delegate_connections.append(
            connect(value, &QQmlComponent::statusChanged, this, &PageIndicator::scheduleDelegates));
        m_delegate_connections.append(connect(value, &QObject::destroyed, this, [this]() {
            m_rebuild = true;
            cancelPress();
            scheduleDelegates();
            Q_EMIT delegateChanged();
        }));
    }
    scheduleDelegates();
    Q_EMIT delegateChanged();
}

QQuickItem* PageIndicator::itemAt(int index) const {
    return index >= 0 && index < m_items.size() ? m_items[index].item.data() : nullptr;
}

void PageIndicator::scheduleDelegates() {
    if (isComponentComplete()) polish();
}

void PageIndicator::clearDelegates() {
    cancelPress();
    const auto entries = std::exchange(m_items, {});
    for (const auto& entry : entries) {
        if (! entry.item) continue;
        disconnect(entry.item, nullptr, this, nullptr);
        entry.item->setParentItem(nullptr);
        entry.item->deleteLater();
    }
}

void PageIndicator::updatePolish() {
    if (m_syncing) {
        polish();
        return;
    }
    m_syncing = true;
    if (std::exchange(m_rebuild, false)) clearDelegates();
    while (m_items.size() > m_count) {
        auto entry = m_items.takeLast();
        if (entry.item) {
            disconnect(entry.item, nullptr, this, nullptr);
            entry.item->setParentItem(nullptr);
            entry.item->deleteLater();
        }
    }
    while (! m_rebuild && m_items.size() < m_count && m_delegate && m_delegate->isReady() &&
           contentItem()) {
        const QPointer<QQmlComponent> component     = m_delegate;
        auto                          parentContext = component->creationContext();
        if (! parentContext) parentContext = qmlContext(this);
        if (! parentContext) break;
        auto      context = new QQmlContext(parentContext, this);
        const int index   = m_items.size();
        context->setContextProperty(QStringLiteral("index"), index);
        context->setContextProperty(QStringLiteral("pressed"), false);
        auto object = component->beginCreate(context);
        auto item   = qobject_cast<QQuickItem*>(object);
        if (! item) {
            component->completeCreate();
            delete object;
            delete context;
            qmlWarning(this) << "PageIndicator delegate must create an Item";
            break;
        }
        item->setParent(this);
        context->setParent(item);
        QQmlEngine::setObjectOwnership(item, QQmlEngine::CppOwnership);
        const auto  meta            = item->metaObject();
        const int   pressedProperty = meta->indexOfProperty("pressed");
        QVariantMap initial;
        const int   indexProperty = meta->indexOfProperty("index");
        if (indexProperty >= 0 && meta->property(indexProperty).isWritable())
            initial.insert("index", index);
        const bool writablePressed =
            pressedProperty >= 0 && meta->property(pressedProperty).isWritable();
        if (writablePressed) initial.insert("pressed", false);
        component->setInitialProperties(item, initial);
        m_items.append({ item, context, writablePressed });
        connect(item, &QObject::destroyed, this, [this]() {
            m_rebuild = true;
            cancelPress();
            scheduleDelegates();
        });
        item->setParentItem(contentItem());
        component->completeCreate();
        if (component && component->isError()) {
            qmlWarning(this) << component->errorString();
            m_rebuild = true;
            clearDelegates();
            break;
        }
    }
    m_syncing = false;
}

void PageIndicator::componentComplete() {
    Control::componentComplete();
    updatePolish();
}

int PageIndicator::hitIndex(const QPointF& point) const {
    if (! contains(point)) return -1;
    qreal distance = std::numeric_limits<qreal>::infinity();
    int   nearest  = -1;
    for (int i = 0; i < m_items.size() && i < m_count; ++i) {
        auto item = m_items[i].item;
        if (! item || ! item->isVisible() || ! item->isEnabled() ||
            item->parentItem() != contentItem())
            continue;
        const auto local = item->mapFromItem(this, point);
        if (item->contains(local)) return i;
        const qreal candidate =
            QLineF(local, QPointF(item->width() / 2, item->height() / 2)).length();
        if (candidate < distance) {
            distance = candidate;
            nearest  = i;
        }
    }
    return nearest;
}

void PageIndicator::setPressed(int index) {
    if (m_pressed_index == index) return;
    m_pressed_index = index;
    // Copy guards against QML property handlers changing the delegate collection.
    const auto entries = m_items;
    for (int i = 0; i < entries.size(); ++i) {
        const auto& entry   = entries[i];
        const bool  pressed = i == m_pressed_index;
        if (entry.context) entry.context->setContextProperty(QStringLiteral("pressed"), pressed);
        if (entry.item && entry.pressedProperty)
            entry.item->setProperty("pressed", i == m_pressed_index);
    }
}

void PageIndicator::cancelPress() {
    m_mouse_pressed = false;
    m_touch_id      = -1;
    setPressed(-1);
}

void PageIndicator::releasePress(const QPointF& point) {
    const int index = hitIndex(point);
    cancelPress();
    if (index >= 0 && index < m_count && m_interactive) setCurrentIndex(index);
}

void PageIndicator::mousePressEvent(QMouseEvent* event) {
    if (! m_interactive || event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }
    m_mouse_pressed = true;
    setPressed(hitIndex(event->position()));
    event->accept();
}
void PageIndicator::mouseMoveEvent(QMouseEvent* event) {
    if (! m_mouse_pressed) {
        event->ignore();
        return;
    }
    setPressed(hitIndex(event->position()));
    event->accept();
}
void PageIndicator::mouseReleaseEvent(QMouseEvent* event) {
    if (! m_mouse_pressed || event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }
    releasePress(event->position());
    event->accept();
}
void PageIndicator::mouseUngrabEvent() { cancelPress(); }
void PageIndicator::touchUngrabEvent() { cancelPress(); }
void PageIndicator::touchEvent(QTouchEvent* event) {
    if (! m_interactive) {
        event->ignore();
        return;
    }
    if (event->type() == QEvent::TouchCancel) {
        cancelPress();
        event->accept();
        return;
    }
    for (const auto& point : event->points()) {
        if (m_touch_id < 0 && point.state() == QEventPoint::Pressed) m_touch_id = point.id();
        if (point.id() != m_touch_id) continue;
        if (point.state() == QEventPoint::Released)
            releasePress(point.position());
        else
            setPressed(hitIndex(point.position()));
        event->accept();
        return;
    }
    event->ignore();
}
void PageIndicator::itemChange(ItemChange change, const ItemChangeData& data) {
    Control::itemChange(change, data);
    if ((change == ItemEnabledHasChanged && ! isEnabled()) ||
        (change == ItemVisibleHasChanged && ! isVisible()) || change == ItemSceneChange)
        cancelPress();
}

} // namespace qml_material
