#include "qml_material/control/page.hpp"
#include "qml_material/control/tab_bar.hpp"
#include "qml_material/control/tool_bar.hpp"
#include "qml_material/util/qt.hpp"

#include <QScopedValueRollback>
#include <QtQml/qqmlinfo.h>
#include <algorithm>

namespace qml_material
{
namespace
{
qreal occupiedHeight(QQuickItem* item, qreal spacing) {
    return item && item->isVisible() && item->height() > 0 ? item->height() + spacing : 0;
}
} // namespace

Page::Page(QQuickItem* parent): Panel(parent) {
    m_spacing_connection = connect(this, &Control::spacingChanged, this, &Page::relayout);
}

Page::~Page() {
    disconnect(m_spacing_connection);
    utils::disconnectAll(m_header.connections);
    utils::disconnectAll(m_footer.connections);
}

QString Page::title() const { return m_title; }
void    Page::setTitle(const QString& value) {
    if (m_title == value) return;
    m_title = value;
    Q_EMIT titleChanged();
}
void Page::resetTitle() { setTitle({}); }
bool Page::canBack() const { return m_can_back; }
void Page::setCanBack(bool value) {
    if (m_can_back == value) return;
    m_can_back = value;
    Q_EMIT canBackChanged();
}
bool Page::scrolling() const { return m_scrolling; }
void Page::setScrolling(bool value) {
    if (m_scrolling == value) return;
    m_scrolling = value;
    Q_EMIT scrollingChanged();
}
QQuickItem* Page::header() const { return m_header.item; }
QQuickItem* Page::footer() const { return m_footer.item; }
void        Page::setHeader(QQuickItem* item) { setSlot(m_header, item); }
void        Page::setFooter(QQuickItem* item) { setSlot(m_footer, item); }
qreal       Page::implicitHeaderWidth() const { return m_header.implicitSize.width(); }
qreal       Page::implicitHeaderHeight() const { return m_header.implicitSize.height(); }
qreal       Page::implicitFooterWidth() const { return m_footer.implicitSize.width(); }
qreal       Page::implicitFooterHeight() const { return m_footer.implicitSize.height(); }

void Page::setSlot(Slot& slot, QQuickItem* item) {
    if (slot.item == item) return;
    const auto& other = &slot == &m_header ? m_footer : m_header;
    if (item && (item == other.item || item == this || item->isAncestorOf(this) ||
                 item == contentItem() || item == background())) {
        qmlWarning(this) << "Page header/footer requires a distinct item";
        return;
    }
    utils::disconnectAll(slot.connections);
    if (slot.item && slot.item->parentItem() == this) slot.item->setParentItem(nullptr);
    slot.item = item;
    if (item) {
        item->setParentItem(this);
        if (qFuzzyIsNull(item->z())) item->setZ(1);
        const auto update = [this, &slot]() {
            updateSlot(slot);
        };
        slot.connections.append(connect(item, &QQuickItem::implicitWidthChanged, this, update));
        slot.connections.append(connect(item, &QQuickItem::implicitHeightChanged, this, update));
        slot.connections.append(connect(item, &QQuickItem::visibleChanged, this, update));
        slot.connections.append(connect(item, &QQuickItem::heightChanged, this, &Page::relayout));
        slot.connections.append(connect(item, &QQuickItem::widthChanged, this, &Page::relayout));
        slot.connections.append(connect(item, &QObject::destroyed, this, [this, &slot]() {
            utils::disconnectAll(slot.connections);
            slot.item = nullptr;
            updateSlot(slot);
            if (&slot == &m_header)
                Q_EMIT headerChanged();
            else
                Q_EMIT footerChanged();
        }));
    }
    QPointer<Page> guard(this);
    if (auto* bar = qobject_cast<ToolBar*>(item))
        bar->setPosition(&slot == &m_header ? ToolBar::Header : ToolBar::Footer);
    else if (auto* bar = qobject_cast<TabBar*>(item))
        bar->setPosition(&slot == &m_header ? TabBar::Header : TabBar::Footer);
    if (! guard) return;
    updateSlot(slot);
    if (&slot == &m_header)
        Q_EMIT headerChanged();
    else
        Q_EMIT footerChanged();
}

void Page::updateSlot(Slot& slot) {
    const auto previous = slot.implicitSize;
    slot.implicitSize   = slot.item && slot.item->isVisible()
                              ? QSizeF(slot.item->implicitWidth(), slot.item->implicitHeight())
                              : QSizeF(0, 0);
    if (previous.width() != slot.implicitSize.width()) {
        if (&slot == &m_header)
            Q_EMIT implicitHeaderWidthChanged();
        else
            Q_EMIT implicitFooterWidthChanged();
    }
    if (previous.height() != slot.implicitSize.height()) {
        if (&slot == &m_header)
            Q_EMIT implicitHeaderHeightChanged();
        else
            Q_EMIT implicitFooterHeightChanged();
    }
    relayout();
}

QRectF Page::contentRect() const {
    auto       rect   = Control::contentRect();
    const auto top    = occupiedHeight(header(), spacing());
    const auto bottom = occupiedHeight(footer(), spacing());
    rect.setY(rect.y() + top);
    rect.setHeight(std::max<qreal>(0, availableHeight() - top - bottom));
    return rect;
}

void Page::relayout() {
    if (! isComponentComplete() || m_layout_active) return;
    QScopedValueRollback guard(m_layout_active, true);
    if (auto* item = header()) {
        item->setX(0);
        item->setY(0);
        item->setWidth(width());
    }
    if (auto* item = footer()) {
        item->setX(0);
        item->setWidth(width());
        item->setY(height() - item->height());
    }
    layoutContentItem();
}

void Page::componentComplete() {
    Panel::componentComplete();
    relayout();
}
void Page::geometryChange(const QRectF& geometry, const QRectF& oldGeometry) {
    Panel::geometryChange(geometry, oldGeometry);
    relayout();
}

} // namespace qml_material
