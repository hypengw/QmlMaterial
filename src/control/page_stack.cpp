#include "qml_material/control/page_stack.hpp"
#include "page_stack_entries_p.hpp"
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquicktransitionmanager_p_p.h>
#include <QQuickWindow>
#include <QTimer>
#include <array>
#include <functional>

namespace qml_material
{
namespace
{
class StackMotion : public QQuickTransitionManager {
public:
    std::function<void()> done;

protected:
    void finished() override {
        if (done) done();
    }
};
} // namespace
struct PageStack::State {
    struct Page {
        QPointer<QQuickItem> item;
        QPointer<QQuickItem> parent;
        bool                 widthExplicit  = false;
        bool                 heightExplicit = false;
    };
    PageStackEntries                          entries;
    QMap<quint64, Page>                       pages;
    QPointer<QQuickItem>                      initial;
    quint64                                   notifiedCurrent = 0;
    int                                       notifiedDepth   = 0;
    bool                                      operating       = false;
    bool                                      closing         = false;
    bool                                      notifiedBusy    = false;
    std::array<QPointer<QQuickTransition>, 6> transitions;
    struct Motion {
        QPointer<QQuickItem>         item;
        QPointF                      position;
        qreal                        opacity = 1;
        qreal                        scale   = 1;
        std::shared_ptr<StackMotion> runner;
    };
    QList<Motion>  motions;
    quint64        motionId       = 0;
    int            pendingMotions = 0;
    TransitionMode mode           = Animated;
};

static PageStackAttached* attached(QQuickItem* item) {
    return qobject_cast<PageStackAttached*>(qmlAttachedPropertiesObject<PageStack>(item, true));
}

void PageStackAttached::update(PageStack* view, int index, Status status) {
    if (m_view == view && m_index == index && m_status == status) return;
    const bool statusChangedValue = m_status != status;
    m_view                        = view;
    m_index                       = index;
    m_status                      = status;
    QPointer<PageStackAttached> guard(this);
    Q_EMIT changed();
    if (guard && statusChangedValue && m_status == status) Q_EMIT statusChanged();
}

PageStack::PageStack(QQuickItem* parent): Control(parent), m_state(std::make_shared<State>()) {
    setFlag(ItemIsFocusScope);
    connect(&m_state->entries, &PageStackEntries::changed, this, &PageStack::synchronize);
    connect(&m_state->entries, &PageStackEntries::entryRemoved, this, [this](quint64 id) {
        removePage(id);
    });
    connect(&m_state->entries,
            &PageStackEntries::transitionRequested,
            this,
            &PageStack::startTransition);
}
PageStack::~PageStack() {
    const auto state = m_state;
    state->closing   = true;
    stopMotions();
    state->entries.shutdown();
    // Entries may already be erased while their removal notifications are still pending.
    const auto ids = state->pages.keys();
    for (auto id : ids) removePage(id);
}
PageStackAttached* PageStack::qmlAttachedProperties(QObject* object) {
    return new PageStackAttached(object);
}
int  PageStack::depth() const { return m_state->entries.depth(); }
bool PageStack::busy() const { return m_state->notifiedBusy; }
#define QM_STACK_TRANSITION(Name, Setter, Index)                                      \
    QQuickTransition* PageStack::Name() const { return m_state->transitions[Index]; } \
    void              PageStack::Setter(QQuickTransition* transition) {               \
        if (m_state->transitions[Index] == transition) return;                        \
        m_state->transitions[Index] = transition;                                     \
        Q_EMIT transitionsChanged();                                                  \
    }
QM_STACK_TRANSITION(pushEnter, setPushEnter, 0)
QM_STACK_TRANSITION(pushExit, setPushExit, 1)
QM_STACK_TRANSITION(popEnter, setPopEnter, 2)
QM_STACK_TRANSITION(popExit, setPopExit, 3)
QM_STACK_TRANSITION(replaceEnter, setReplaceEnter, 4)
QM_STACK_TRANSITION(replaceExit, setReplaceExit, 5)
#undef QM_STACK_TRANSITION
QQuickItem* PageStack::currentItem() const { return m_state->entries.currentItem(); }
QQuickItem* PageStack::initialItem() const { return m_state->initial; }
QQuickItem* PageStack::itemAt(int index) const {
    return m_state->entries.entry(m_state->entries.idAt(index)).item;
}
void PageStack::setInitialItem(QQuickItem* item) {
    if (isComponentComplete() || m_state->initial == item) return;
    m_state->initial = item;
    Q_EMIT initialItemChanged();
}
void PageStack::componentComplete() {
    QPointer<PageStack> guard(this);
    Control::componentComplete();
    if (guard && empty() && initialItem()) pushItem(initialItem(), Immediate);
}
bool PageStack::pushItem(QQuickItem* item, TransitionMode mode) { return navigate(0, item, mode); }
bool PageStack::replaceCurrentItem(QQuickItem* item, TransitionMode mode) {
    return navigate(1, item, mode);
}
bool PageStack::popCurrentItem(TransitionMode mode) { return navigate(2, nullptr, mode); }
bool PageStack::clear(TransitionMode mode) { return navigate(3, nullptr, mode); }
bool PageStack::navigate(int operation, QQuickItem* item, TransitionMode mode) {
    const auto state = m_state;
    if (state->operating || state->closing) return false;
    if (mode != Immediate && mode != Animated) return false;
    QPointer<PageStack>  guard(this);
    QPointer<QQuickItem> candidate(item);
    if (operation < 2) {
        if (! item || item == this || item->isAncestorOf(this)) return false;
        const auto properties = attached(item);
        if (properties->view() || properties->m_removing) return false;
    }
    state->operating = true;
    if (const auto transition = state->entries.transition()) {
        stopMotions();
        if (! guard) return false;
        state->entries.finish(transition->id);
        if (! guard) return false;
    }
    if (operation < 2 &&
        (! candidate || attached(candidate)->view() || attached(candidate)->m_removing)) {
        state->operating = false;
        return false;
    }
    state->mode   = mode;
    quint64 token = 0;
    switch (operation) {
    case 0: token = state->entries.push(item); break;
    case 1: token = state->entries.replace(item); break;
    case 2: token = state->entries.pop(); break;
    case 3: token = state->entries.clear(); break;
    }
    state->operating = false;
    return token != 0;
}
void PageStack::synchronize() {
    const auto state = m_state;
    if (state->closing) return;
    QPointer<PageStack> guard(this);
    if (! state->entries.transition() && state->motionId) {
        stopMotions();
        if (! guard) return;
    }
    if (state->notifiedBusy && ! state->entries.transition()) {
        state->notifiedBusy = false;
        setFiltersChildMouseEvents(false);
        Q_EMIT busyChanged();
        if (! guard) return;
    }
    for (const auto& entry : state->entries.entries()) {
        QPointer<QQuickItem> item = entry.item;
        if (! item) continue;
        if (! state->pages.contains(entry.id)) {
            auto p = QQuickItemPrivate::get(item);
            state->pages.insert(entry.id,
                                { item, item->parentItem(), p->widthValid(), p->heightValid() });
            attached(item)->m_view = this;
            Q_EMIT entryAdded(entry.id, item);
            if (! guard || ! item) return;
            // Publish ownership before reparenting can invoke application callbacks.
            attached(item)->update(this, state->entries.indexOf(entry.id), PageStack::Activating);
            if (! guard || ! item) return;
            item->setParentItem(this);
            if (! guard || ! item) return;
        }
        const auto status =
            entry.status == PageStackEntries::Status::Active     ? PageStack::Active
            : entry.status == PageStackEntries::Status::Entering ? PageStack::Activating
            : entry.status == PageStackEntries::Status::Exiting  ? PageStack::Deactivating
                                                                 : PageStack::Inactive;
        item->setVisible(status != PageStack::Inactive);
        if (! guard || ! item) return;
        attached(item)->update(this, state->entries.indexOf(entry.id), status);
        if (! guard) return;
    }
    layoutPages();
    if (! guard) return;
    if (state->notifiedDepth != depth()) {
        state->notifiedDepth = depth();
        Q_EMIT depthChanged();
        if (! guard) return;
    }
    if (state->notifiedCurrent != state->entries.currentId()) {
        state->notifiedCurrent = state->entries.currentId();
        if (currentItem()) currentItem()->setFocus(true);
        if (! guard) return;
        Q_EMIT currentItemChanged();
    }
}
bool PageStack::childMouseEventFilter(QQuickItem* item, QEvent* event) {
    if (! busy()) return false;
    if (event->type() == QEvent::MouseButtonPress) return true;
    if (event->type() == QEvent::UngrabMouse) return false;
    return item->window() && ! item->window()->mouseGrabberItem();
}
void PageStack::startTransition(quint64 id) {
    const auto state      = m_state;
    const auto transition = state->entries.transition();
    if (! transition || transition->id != id) return;
    if (state->mode == Immediate) {
        state->entries.finish(id);
        return;
    }
    const int offset = transition->operation == PageStackEntries::Operation::Push      ? 0
                       : transition->operation == PageStackEntries::Operation::Replace ? 4
                                                                                       : 2;
    const std::array<quint64, 2>              ids { transition->to, transition->from };
    std::array<QPointer<QQuickTransition>, 2> definitions { state->transitions[offset],
                                                            state->transitions[offset + 1] };
    state->motionId = id;
    QPointer<PageStack> guard(this);
    for (int i = 0; i < 2; ++i) {
        auto item = state->entries.entry(ids[i]).item;
        if (! item || ! definitions[i] || ! definitions[i]->enabled()) continue;
        auto runner  = std::make_shared<StackMotion>();
        runner->done = [guard, id] {
            if (guard)
                QTimer::singleShot(0, guard, [guard, id] {
                    if (guard) guard->motionFinished(id);
                });
        };
        state->motions.append({ item, item->position(), item->opacity(), item->scale(), runner });
        ++state->pendingMotions;
    }
    if (! state->pendingMotions) {
        state->motionId = 0;
        state->entries.finish(id);
        return;
    }
    state->notifiedBusy = true;
    setFiltersChildMouseEvents(true);
    Q_EMIT busyChanged();
    if (! guard || state->motionId != id) return;
    const auto motions = state->motions;
    for (const auto& motion : motions) {
        if (! guard || state->motionId != id) return;
        const int index = motion.item == state->entries.entry(transition->to).item ? 0 : 1;
        motion.runner->transition({}, definitions[index], motion.item);
    }
}
void PageStack::motionFinished(quint64 id) {
    const auto state = m_state;
    if (state->motionId != id || state->closing) return;
    if (--state->pendingMotions == 0) completeTransition();
}
void PageStack::stopMotions() {
    const auto state      = m_state;
    state->motionId       = 0;
    state->pendingMotions = 0;
    const auto motions    = std::exchange(state->motions, {});
    for (const auto& motion : motions) {
        motion.runner->done = {};
        motion.runner->cancel();
        if (motion.item) motion.item->setPosition(motion.position);
        if (motion.item) motion.item->setOpacity(motion.opacity);
        if (motion.item) motion.item->setScale(motion.scale);
    }
}
void PageStack::completeTransition() {
    const auto state = m_state;
    if (state->operating || state->closing) return;
    const auto transition = state->entries.transition();
    if (! transition) return;
    state->operating = true;
    QPointer<PageStack> guard(this);
    stopMotions();
    if (guard) state->entries.finish(transition->id);
    state->operating = false;
}
void PageStack::layoutPages() {
    const auto          state = m_state;
    const auto          ids   = state->pages.keys();
    QPointer<PageStack> guard(this);
    for (auto id : ids) {
        const auto page = state->pages.value(id);
        if (! page.item) continue;
        if (! page.widthExplicit) page.item->setWidth(width());
        if (! guard) return;
        if (! state->pages.contains(id)) continue;
        if (page.item && ! page.heightExplicit) page.item->setHeight(height());
        if (! guard) return;
    }
}
void PageStack::geometryChange(const QRectF& geometry, const QRectF& old) {
    QPointer<PageStack> guard(this);
    Control::geometryChange(geometry, old);
    if (guard) layoutPages();
}
void PageStack::removePage(quint64 id) {
    const auto                  state = m_state;
    const auto                  page  = state->pages.take(id);
    QPointer<PageStack>         guard(this);
    QPointer<PageStackAttached> properties = page.item ? attached(page.item) : nullptr;
    if (properties) properties->m_removing = true;
    if (page.item) page.item->setVisible(false);
    if (page.item && ! page.widthExplicit) page.item->resetWidth();
    if (page.item && ! page.heightExplicit) page.item->resetHeight();
    if (page.item) page.item->setParentItem(page.parent);
    if (properties) properties->update(nullptr, -1, PageStack::Inactive);
    if (properties) Q_EMIT properties->removed();
    if (properties) properties->m_removing = false;
    if (guard) Q_EMIT entryRemoved(id);
}
} // namespace qml_material
