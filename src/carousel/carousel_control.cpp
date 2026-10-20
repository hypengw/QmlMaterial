#include "qml_material/carousel/carousel_control.hpp"
#include "qml_material/util/qt.hpp"

namespace qml_material
{
CarouselControl::CarouselControl(QQuickItem* parent): Control(parent) {
    connect(this, &QQuickItem::enabledChanged, this, &CarouselControl::invalidateCommands);
    connect(this, &QQuickItem::visibleChanged, this, &CarouselControl::invalidateCommands);
}
CarouselControl::~CarouselControl() {
    utils::disconnectAll(m_viewConnections);
    utils::disconnectAll(m_previousConnections);
    utils::disconnectAll(m_nextConnections);
}
void CarouselControl::invalidateCommands() {
    ++m_revision;
    const QPointer<CarouselControl> guard(this);
    if (m_previous) m_previous->cancelActivation();
    if (guard && m_next) m_next->cancelActivation();
}
void CarouselControl::setView(CarouselView* value) {
    if (m_view == value && (value || m_viewConnections.isEmpty())) return;
    utils::disconnectAll(m_viewConnections);
    m_view = value;
    if (value) {
        const QPointer<CarouselView> target(value);
        m_viewConnections.append(
            connect(value, &CarouselView::clicked, this, [this, target](int index) {
                if (target && m_view == target && isEnabled() && isVisible()) Q_EMIT clicked(index);
            }));
        m_viewConnections.append(
            connect(value, &CarouselView::currentIndexChanged, this, [this, target] {
                const QPointer<CarouselControl> guard(this);
                invalidateCommands();
                if (! guard || ! target || m_view != target) return;
                const auto revision = m_revision;
                Q_EMIT currentIndexChanged();
                if (guard && target && m_view == target && revision == m_revision)
                    Q_EMIT indexChanged(target->currentIndex());
            }));
        m_viewConnections.append(connect(
            value, &CarouselView::countChanged, this, &CarouselControl::invalidateCommands));
        m_viewConnections.append(connect(
            value, &CarouselView::modelChanged, this, &CarouselControl::invalidateCommands));
        m_viewConnections.append(connect(
            value, &CarouselView::delegateChanged, this, &CarouselControl::invalidateCommands));
        m_viewConnections.append(connect(
            value, &CarouselView::layoutChanged, this, &CarouselControl::invalidateCommands));
        m_viewConnections.append(connect(
            value, &CarouselView::orientationChanged, this, &CarouselControl::invalidateCommands));
        m_viewConnections.append(connect(
            value, &CarouselView::interactiveChanged, this, &CarouselControl::invalidateCommands));
        m_viewConnections.append(connect(
            value, &QQuickItem::enabledChanged, this, &CarouselControl::invalidateCommands));
        m_viewConnections.append(connect(
            value, &QQuickItem::visibleChanged, this, &CarouselControl::invalidateCommands));
        m_viewConnections.append(connect(value, &QObject::destroyed, this, [this] {
            setView(nullptr);
        }));
    }
    const QPointer<CarouselControl> guard(this);
    invalidateCommands();
    if (! guard) return;
    if (value && m_pendingIndex) {
        const int index = *std::exchange(m_pendingIndex, {});
        value->setCurrentIndex(index);
        if (! guard || m_view != value) return;
    }
    const auto revision = m_revision;
    Q_EMIT viewChanged();
    if (guard && revision == m_revision) Q_EMIT currentIndexChanged();
}
void CarouselControl::setPreviousControl(CommandButton* value) {
    if (m_previous == value && (value || m_previousConnections.isEmpty())) return;
    const auto old = m_previous;
    utils::disconnectAll(m_previousConnections);
    m_previous = value;
    if (value) {
        m_previousConnections.append(connect(value, &CommandButton::clicked, this, [this] {
            navigate(-1);
        }));
        m_previousConnections.append(connect(value, &QObject::destroyed, this, [this] {
            setPreviousControl(nullptr);
        }));
    }
    const QPointer<CarouselControl> guard(this);
    if (old) old->cancelActivation();
    if (! guard) return;
    invalidateCommands();
    if (guard) Q_EMIT previousControlChanged();
}
void CarouselControl::setNextControl(CommandButton* value) {
    if (m_next == value && (value || m_nextConnections.isEmpty())) return;
    const auto old = m_next;
    utils::disconnectAll(m_nextConnections);
    m_next = value;
    if (value) {
        m_nextConnections.append(connect(value, &CommandButton::clicked, this, [this] {
            navigate(1);
        }));
        m_nextConnections.append(connect(value, &QObject::destroyed, this, [this] {
            setNextControl(nullptr);
        }));
    }
    const QPointer<CarouselControl> guard(this);
    if (old) old->cancelActivation();
    if (! guard) return;
    invalidateCommands();
    if (guard) Q_EMIT nextControlChanged();
}
void CarouselControl::navigate(int direction) {
    const auto view   = m_view;
    const auto button = direction > 0 ? m_next : m_previous;
    if (! isEnabled() || ! isVisible() || ! view || ! view->isEnabled() || ! view->isVisible() ||
        ! view->interactive() || ! button || ! button->canActivate())
        return;
    const QPointer<CarouselControl> guard(this);
    const auto                      revision = m_revision;
    view->claimInteractionFocus();
    if (! guard || revision != m_revision || ! view || ! button || ! isEnabled() || ! isVisible() ||
        ! view->isEnabled() || ! view->isVisible() || ! view->interactive() ||
        ! button->canActivate())
        return;
    if (direction > 0)
        view->incrementCurrentIndex();
    else
        view->decrementCurrentIndex();
}
void CarouselControl::incrementCurrentIndex() {
    if (m_view) m_view->incrementCurrentIndex();
}
void CarouselControl::decrementCurrentIndex() {
    if (m_view) m_view->decrementCurrentIndex();
}
void CarouselControl::setCurrentIndex(int index, bool animated) {
    if (! m_view) {
        setCurrentIndex(index);
        return;
    }
    if (animated)
        m_view->setCurrentIndexAnimated(index);
    else
        m_view->setCurrentIndex(index);
}
int CarouselControl::currentIndex() const {
    return m_view ? m_view->currentIndex() : m_pendingIndex.value_or(0);
}
void CarouselControl::setCurrentIndex(int index) {
    if (m_view) {
        m_view->setCurrentIndex(index);
        return;
    }
    const int old  = currentIndex();
    m_pendingIndex = qMax(0, index);
    if (old != currentIndex()) Q_EMIT currentIndexChanged();
}
} // namespace qml_material
