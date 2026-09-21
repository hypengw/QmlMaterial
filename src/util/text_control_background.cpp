#include "qml_material/util/text_control_background.hpp"
#include "qml_material/util/qt.hpp"
#include <QtQuick/private/qquickitem_p.h>

namespace qml_material
{

TextControlBackground::TextControlBackground(QQuickItem* owner): QObject(owner), m_owner(owner) {}
TextControlBackground::~TextControlBackground() { utils::disconnectAll(m_connections); }

const std::optional<qreal>& TextControlBackground::insetValue(Qt::Edge edge) const {
    switch (edge) {
    case Qt::TopEdge: return m_insets[0];
    case Qt::LeftEdge: return m_insets[1];
    case Qt::RightEdge: return m_insets[2];
    case Qt::BottomEdge: return m_insets[3];
    }
    Q_UNREACHABLE();
}
std::optional<qreal>& TextControlBackground::insetValue(Qt::Edge edge) {
    return const_cast<std::optional<qreal>&>(std::as_const(*this).insetValue(edge));
}
qreal TextControlBackground::inset(Qt::Edge edge) const { return insetValue(edge).value_or(0); }
void  TextControlBackground::setInset(Qt::Edge edge, qreal value) {
    auto& stored = insetValue(edge);
    if (stored && qFuzzyCompare(*stored, value)) return;
    const auto old = stored.value_or(0);
    stored         = value;
    layout();
    if (! qFuzzyCompare(old, value)) Q_EMIT insetChanged(edge);
}
void TextControlBackground::resetInset(Qt::Edge edge) {
    auto& stored = insetValue(edge);
    if (! stored) return;
    const auto old = *stored;
    stored.reset();
    layout();
    if (! qFuzzyIsNull(old)) Q_EMIT insetChanged(edge);
}
qreal TextControlBackground::implicitWidth() const { return m_item ? m_item->implicitWidth() : 0; }
qreal TextControlBackground::implicitHeight() const {
    return m_item ? m_item->implicitHeight() : 0;
}

void TextControlBackground::setItem(QQuickItem* item) {
    if (m_item == item) return;
    const auto oldWidth  = implicitWidth();
    const auto oldHeight = implicitHeight();
    utils::disconnectAll(m_connections);
    if (m_item) m_item->setParentItem(nullptr);
    m_item = item;
    if (item) {
        item->setParentItem(m_owner);
        if (qFuzzyIsNull(item->z())) item->setZ(-1);
        const auto priv   = QQuickItemPrivate::get(item);
        m_width_explicit  = priv->widthValid();
        m_height_explicit = priv->heightValid();
        m_connections.append(connect(item,
                                     &QQuickItem::implicitWidthChanged,
                                     this,
                                     &TextControlBackground::implicitWidthChanged));
        m_connections.append(connect(item,
                                     &QQuickItem::implicitHeightChanged,
                                     this,
                                     &TextControlBackground::implicitHeightChanged));
        m_connections.append(
            connect(item, &QQuickItem::widthChanged, this, &TextControlBackground::sizeChanged));
        m_connections.append(
            connect(item, &QQuickItem::heightChanged, this, &TextControlBackground::sizeChanged));
        m_connections.append(connect(item, &QObject::destroyed, this, [this]() {
            utils::disconnectAll(m_connections);
            m_item = nullptr;
            Q_EMIT itemChanged();
            Q_EMIT implicitWidthChanged();
            Q_EMIT implicitHeightChanged();
        }));
        layout();
    }
    if (! qFuzzyCompare(oldWidth, implicitWidth())) Q_EMIT implicitWidthChanged();
    if (! qFuzzyCompare(oldHeight, implicitHeight())) Q_EMIT implicitHeightChanged();
    Q_EMIT itemChanged();
}

void TextControlBackground::sizeChanged() {
    if (m_laying_out || ! m_item) return;
    const auto priv   = QQuickItemPrivate::get(m_item);
    m_width_explicit  = priv->widthValid();
    m_height_explicit = priv->heightValid();
    layout();
}

void TextControlBackground::complete() {
    m_complete = true;
    layout();
}

void TextControlBackground::layout() {
    if (! m_item || m_laying_out || ! m_complete) return;
    m_laying_out                    = true;
    const QPointer<QQuickItem> item = m_item;
    if ((! m_width_explicit && qFuzzyIsNull(m_item->x())) || insetValue(Qt::LeftEdge) ||
        insetValue(Qt::RightEdge)) {
        m_item->setX(inset(Qt::LeftEdge));
        if (m_item && m_item == item) {
            const bool wasExplicit = QQuickItemPrivate::get(item)->widthValid();
            m_item->setWidth(
                qMax<qreal>(0, m_owner->width() - inset(Qt::LeftEdge) - inset(Qt::RightEdge)));
            if (item && m_item == item && ! wasExplicit)
                QQuickItemPrivate::get(item)->widthValidFlag = false;
        }
    }
    if (m_item && m_item == item &&
        ((! m_height_explicit && qFuzzyIsNull(m_item->y())) || insetValue(Qt::TopEdge) ||
         insetValue(Qt::BottomEdge))) {
        m_item->setY(inset(Qt::TopEdge));
        if (m_item && m_item == item) {
            const bool wasExplicit = QQuickItemPrivate::get(item)->heightValid();
            m_item->setHeight(
                qMax<qreal>(0, m_owner->height() - inset(Qt::TopEdge) - inset(Qt::BottomEdge)));
            if (item && m_item == item && ! wasExplicit)
                QQuickItemPrivate::get(item)->heightValidFlag = false;
        }
    }
    m_laying_out = false;
    if (m_item && m_item != item) layout();
}

} // namespace qml_material
