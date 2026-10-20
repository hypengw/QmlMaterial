#include "qml_material/control/page_indicator.hpp"
#include <QMouseEvent>
#include <QTouchEvent>
#include <QLineF>
#include <QKeyEvent>
#include <QQmlEngine>
#include <QQmlInfo>
#include <limits>
#include <utility>

namespace qml_material
{

PageIndicator::PageIndicator(QQuickItem* parent)
    : Control(parent), m_input(this, [this] {
          Q_EMIT interactionSourceChanged();
      }) {
    m_input.acceptsInput = [this] {
        return m_interactive && m_count > 0;
    };
    m_input.click().pointerStarted = [this](const PointerInput& input) {
        setPressed(hitIndex(input.position));
    };
    m_input.click().pointerMoved = [this](QPointF point) {
        setPressed(hitIndex(point));
    };
    m_input.click().pointerClicked = [this](QPointF point) {
        releasePress(point);
    };
    m_input.click().pointerCancelled = [this] {
        setPressed(-1);
    };
    m_input.click().clicked = [this] {
        if (m_count > 0) Q_EMIT currentIndexChangeRequested((m_current_index + 1) % m_count);
    };
    m_input.focusVisibleChanged = [this] {
        Q_EMIT focusVisibleChanged();
    };
    setFocusPolicy(Qt::StrongFocus);
    m_input.setHoverEnabled(hoverEnabled());
    connect(this, &Control::hoverEnabledChanged, this, [this] {
        m_input.setHoverEnabled(hoverEnabled());
    });
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
        auto      object  = component->beginCreate(context);
        auto      item    = qobject_cast<QQuickItem*>(object);
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
        component->setInitialProperties(item, { { "index", index } });
        m_items.append({ item });
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
    Q_EMIT highlightedIndexChanged();
}

void PageIndicator::cancelPress() {
    const QPointer<PageIndicator> guard(this);
    m_input.cancelActivation();
    if (! guard) return;
    setPressed(-1);
}

void PageIndicator::releasePress(const QPointF& point) {
    const int                     index = hitIndex(point);
    const QPointer<PageIndicator> guard(this);
    setPressed(-1);
    if (guard && index >= 0 && index < m_count && m_interactive)
        Q_EMIT currentIndexChangeRequested(index);
}

bool PageIndicator::event(QEvent* event) {
    const QPointer<PageIndicator> guard(this);
    const bool                    handled = m_input.observeEvent(event);
    return ! guard || handled ? true : Control::event(event);
}
void PageIndicator::mousePressEvent(QMouseEvent* event) { m_input.event(event); }
void PageIndicator::mouseMoveEvent(QMouseEvent* event) { m_input.event(event); }
void PageIndicator::mouseReleaseEvent(QMouseEvent* event) { m_input.event(event); }
void PageIndicator::mouseUngrabEvent() { m_input.mouseUngrabbed(); }
void PageIndicator::touchUngrabEvent() { m_input.touchUngrabbed(); }
void PageIndicator::touchEvent(QTouchEvent* event) { m_input.event(event); }
void PageIndicator::keyPressEvent(QKeyEvent* event) {
    if (! m_input.available()) {
        event->ignore();
        return;
    }
    int next = m_current_index;
    switch (event->key()) {
    case Qt::Key_Left: next += mirrored() ? 1 : -1; break;
    case Qt::Key_Right: next += mirrored() ? -1 : 1; break;
    case Qt::Key_Home: next = 0; break;
    case Qt::Key_End: next = m_count - 1; break;
    default:
        if (! m_input.event(event)) Control::keyPressEvent(event);
        return;
    }
    event->accept();
    next = std::clamp(next, 0, m_count - 1);
    if (next != m_current_index) Q_EMIT currentIndexChangeRequested(next);
}
void PageIndicator::keyReleaseEvent(QKeyEvent* event) { m_input.event(event); }

} // namespace qml_material
