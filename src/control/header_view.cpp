#include "qml_material/control/header_view.hpp"
#include <QtQuick/private/qquickdraghandler_p.h>

namespace qml_material
{
class HeaderDragHandler : public QQuickDragHandler {
public:
    HeaderDragHandler(HeaderView* view, Qt::Orientation orientation)
        : QQuickDragHandler(view->contentItem()), m_view(view), m_orientation(orientation) {
        setTarget(nullptr);
        setAcceptedButtons(Qt::LeftButton);
        setGrabPermissions(CanTakeOverFromItems | ApprovesCancellation);
        xAxis()->setEnabled(orientation == Qt::Horizontal);
        yAxis()->setEnabled(orientation == Qt::Vertical);
        setEnabled(false);
    }
    void cancel() {
        if (m_source >= 0) m_view->setFiltersChildMouseEvents(m_filterChildren);
        m_source = -1;
        m_point  = -1;
    }

protected:
    bool wantsEventPoint(const QPointerEvent* event, const QEventPoint& point) override {
        if (point.state() == QEventPoint::Pressed) {
            const auto local = m_view->mapFromScene(point.scenePosition());
            if (! m_view->contains(local)) return false;
            const auto pos  = m_view->contentItem()->mapFromScene(point.scenePosition());
            const auto cell = m_view->cellAtPosition(pos);
            auto       item = m_view->itemAtCell(cell);
            if (! item) return false;
            const auto  inside     = item->mapFromScene(point.scenePosition());
            const bool  horizontal = m_orientation == Qt::Horizontal;
            const qreal coordinate = horizontal ? inside.x() : inside.y();
            const qreal size       = horizontal ? item->width() : item->height();
            const bool  resize = horizontal ? m_view->resizableColumns() : m_view->resizableRows();
            if (resize && (coordinate <= 5 || coordinate >= size - 5)) return false;
        }
        return QQuickDragHandler::wantsEventPoint(event, point);
    }
    void handlePointerEventImpl(QPointerEvent* event) override {
        if (event->isBeginEvent() && ! event->points().isEmpty()) {
            const auto& point = event->points().first();
            const auto  pos   = m_view->contentItem()->mapFromScene(point.scenePosition());
            const auto  cell  = m_view->cellAtPosition(pos);
            m_source          = m_orientation == Qt::Horizontal ? cell.x() : cell.y();
            m_point           = point.id();
            m_filterChildren  = m_view->filtersChildMouseEvents();
            if (m_source >= 0) m_view->setFiltersChildMouseEvents(false);
        }
        QQuickDragHandler::handlePointerEventImpl(event);
    }
    void onGrabChanged(QQuickPointerHandler* grabber, QPointingDevice::GrabTransition transition,
                       QPointerEvent* event, QEventPoint& point) override {
        const int source      = m_source;
        int       destination = -1;
        if (grabber == this && point.id() == m_point && source >= 0 &&
            transition == QPointingDevice::UngrabExclusive &&
            point.state() == QEventPoint::Released &&
            m_view->contains(m_view->mapFromScene(point.scenePosition()))) {
            const auto cell = m_view->cellAtPosition(
                m_view->contentItem()->mapFromScene(point.scenePosition()), true);
            destination = m_orientation == Qt::Horizontal ? cell.x() : cell.y();
        }
        if (grabber == this && transition != QPointingDevice::GrabExclusive &&
            transition != QPointingDevice::GrabPassive)
            cancel();
        QQuickDragHandler::onGrabChanged(grabber, transition, event, point);
        if (destination >= 0 && destination != source) {
            if (m_orientation == Qt::Horizontal)
                m_view->moveColumn(source, destination);
            else
                m_view->moveRow(source, destination);
        }
    }

private:
    HeaderView*     m_view;
    Qt::Orientation m_orientation;
    int             m_source         = -1;
    int             m_point          = -1;
    bool            m_filterChildren = true;
};

HeaderView::HeaderView(Qt::Orientation orientation, QQuickItem* parent)
    : QQuickTableView(parent), m_model(this) {
    m_model.setOrientation(orientation);
    setSyncDirection(orientation);
    setFlickableDirection(orientation == Qt::Horizontal ? HorizontalFlick : VerticalFlick);
    setResizableColumns(orientation == Qt::Horizontal);
    setResizableRows(orientation == Qt::Vertical);
    m_drag = new HeaderDragHandler(this, orientation);
    connect(this, &QQuickItem::windowChanged, this, [this] {
        m_drag->cancel();
    });
    connect(this, &QQuickItem::visibleChanged, this, [this] {
        if (! isVisible()) m_drag->cancel();
    });
    connect(this, &QQuickItem::enabledChanged, this, [this] {
        if (! isEnabled()) m_drag->cancel();
    });
    connect(&m_model, &HeaderModel::contentChanged, this, [this] {
        m_drag->cancel();
    });
    connect(&m_model, &HeaderModel::modelChanged, this, &QQuickTableView::modelChanged);
    connect(&m_model, &HeaderModel::textRoleChanged, this, &HeaderView::textRoleChanged);
    connect(&m_model, &HeaderModel::sourceModeChanged, this, &HeaderView::sourceModeChanged);
    connect(&m_model, &HeaderModel::contentChanged, this, &HeaderView::headerContentChanged);
    connect(this, &HeaderView::headerContentChanged, this, &HeaderView::sectionOrderChanged);
    connect(this, &QQuickTableView::columnMoved, this, &HeaderView::sectionOrderChanged);
    connect(this, &QQuickTableView::rowMoved, this, &HeaderView::sectionOrderChanged);
    connect(this, &QQuickTableView::layoutChanged, this, &HeaderView::sectionOrderChanged);
    connect(&m_model, &HeaderModel::effectiveModelChanged, this, [this] {
        QQuickTableView::setModel(QVariant::fromValue(m_model.effectiveModel()));
    });
    QQuickTableView::setModel(QVariant::fromValue(m_model.effectiveModel()));
    connect(this, &QQuickTableView::syncDirectionChanged, this, [this, orientation] {
        if (syncDirection() != orientation) setSyncDirection(orientation);
    });
    connect(this, &QQuickTableView::syncViewChanged, this, [this] {
        disconnect(m_syncChanged);
        disconnect(m_syncDestroyed);
        if (auto view = syncView()) {
            m_syncChanged =
                connect(view, &QQuickTableView::modelChanged, this, &HeaderView::syncSource);
            m_syncDestroyed = connect(view, &QObject::destroyed, this, [this] {
                m_model.setSyncModel(nullptr);
            });
        }
        syncSource();
    });
}

HeaderView::~HeaderView() {
    m_drag->cancel();
    delete m_drag;
}

void HeaderView::setMovableSections(bool value) {
    if (m_movable == value) return;
    m_movable = value;
    if (! value) m_drag->cancel();
    m_drag->setEnabled(value);
    Q_EMIT movableSectionsChanged();
}

void HeaderView::syncSource() {
    m_model.setSyncSource(syncView() ? syncView()->model() : QVariant {});
}

QList<int> HeaderView::visualSections() const {
    QList<int> result;
    const bool horizontal = m_model.orientation() == Qt::Horizontal;
    for (int section = 0; section < sectionCount(); ++section) {
        const auto index  = horizontal ? m_model.effectiveModel()->index(0, section)
                                       : m_model.effectiveModel()->index(section, 0);
        const int  visual = horizontal ? columnAtIndex(index) : rowAtIndex(index);
        result.append(visual < 0 ? section : visual);
    }
    return result;
}

HorizontalHeaderView::HorizontalHeaderView(QQuickItem* parent)
    : HeaderView(Qt::Horizontal, parent) {}
VerticalHeaderView::VerticalHeaderView(QQuickItem* parent): HeaderView(Qt::Vertical, parent) {}
} // namespace qml_material
