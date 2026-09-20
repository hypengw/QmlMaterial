#include "qml_material/control/panel.hpp"
#include "qml_material/util/qt.hpp"

#include <QMouseEvent>
#include <QTouchEvent>
#include <QVariant>
#if QT_CONFIG(cursor)
#    include <QCursor>
#endif

namespace qml_material
{
Panel::Panel(QQuickItem* parent): Control(parent) {
    setFlag(ItemIsFocusScope);
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptTouchEvents(true);
#if QT_CONFIG(cursor)
    setCursor(Qt::ArrowCursor);
#endif
    m_slot_connection =
        connect(this, &Control::contentItemChanged, this, &Panel::observeContentItem);
    setContentItem(new QQuickItem(this));
}

Panel::~Panel() {
    disconnect(m_slot_connection);
    utils::disconnectAll(m_host_connections);
    utils::disconnectAll(m_child_connections);
}

QQmlListProperty<QObject> Panel::contentData() {
    return contentItem() ? contentItem()->property("data").value<QQmlListProperty<QObject>>()
                         : QQmlListProperty<QObject>();
}

QQmlListProperty<QQuickItem> Panel::contentChildren() {
    return contentItem() ? contentItem()->property("children").value<QQmlListProperty<QQuickItem>>()
                         : QQmlListProperty<QQuickItem>();
}

qreal Panel::contentWidth() const { return m_content_size.width(); }
qreal Panel::contentHeight() const { return m_content_size.height(); }

void Panel::setContentWidth(qreal value) {
    m_content_width = value;
    updateContentSize();
}

void Panel::setContentHeight(qreal value) {
    m_content_height = value;
    updateContentSize();
}

void Panel::resetContentWidth() {
    m_content_width.reset();
    updateContentSize();
}

void Panel::resetContentHeight() {
    m_content_height.reset();
    updateContentSize();
}

QSizeF Panel::measureImplicitContent() const { return m_content_size; }

QSizeF Panel::calculateContentSize() const {
    QSizeF size(0, 0);
    if (auto* host = contentItem()) {
        const auto children = host->childItems();
        if (children.size() == 1) {
            size = QSizeF(children.front()->implicitWidth(), children.front()->implicitHeight());
        } else if (children.size() > 1) {
            size = host->childrenRect().size();
        }
        if (! qFuzzyIsNull(host->implicitWidth())) size.setWidth(host->implicitWidth());
        if (! qFuzzyIsNull(host->implicitHeight())) size.setHeight(host->implicitHeight());
    }
    if (m_content_width) size.setWidth(*m_content_width);
    if (m_content_height) size.setHeight(*m_content_height);
    return size;
}

void Panel::updateContentSize() {
    const auto previous = m_content_size;
    m_content_size      = calculateContentSize();
    updateImplicitMetrics();
    if (previous.width() != m_content_size.width()) Q_EMIT contentWidthChanged();
    if (previous.height() != m_content_size.height()) Q_EMIT contentHeightChanged();
}

void Panel::observeContentItem() {
    utils::disconnectAll(m_host_connections);
    if (auto* host = contentItem()) {
        m_host_connections.append(
            connect(host, &QQuickItem::childrenChanged, this, &Panel::observeChildren));
        m_host_connections.append(
            connect(host, &QQuickItem::childrenRectChanged, this, &Panel::updateContentSize));
        m_host_connections.append(
            connect(host, &QQuickItem::implicitWidthChanged, this, &Panel::updateContentSize));
        m_host_connections.append(
            connect(host, &QQuickItem::implicitHeightChanged, this, &Panel::updateContentSize));
    }
    observeChildren();
}

void Panel::observeChildren() {
    utils::disconnectAll(m_child_connections);
    if (auto* host = contentItem()) {
        const auto children = host->childItems();
        if (children.size() == 1) {
            auto* child = children.front();
            m_child_connections.append(
                connect(child, &QQuickItem::implicitWidthChanged, this, &Panel::updateContentSize));
            m_child_connections.append(connect(
                child, &QQuickItem::implicitHeightChanged, this, &Panel::updateContentSize));
        }
    }
    updateContentSize();
    Q_EMIT contentChildrenChanged();
}

void Panel::componentComplete() {
    Control::componentComplete();
    updateContentSize();
}

void Panel::mousePressEvent(QMouseEvent* event) { event->accept(); }
void Panel::mouseReleaseEvent(QMouseEvent* event) { event->accept(); }
void Panel::mouseMoveEvent(QMouseEvent* event) { event->accept(); }
void Panel::touchEvent(QTouchEvent* event) { event->accept(); }

} // namespace qml_material
