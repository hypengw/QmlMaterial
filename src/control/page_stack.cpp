#include "qml_material/control/page_stack.hpp"
#include "page_stack_entries_p.hpp"
#include "qml_material/style/theme.hpp"
#include <QtQuick/private/qquickitem_p.h>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTimer>
#include <array>
#include <cmath>

namespace qml_material
{
struct PageStack::State {
    struct Page {
        QPointer<QQuickItem>           item;
        QPointer<QQuickItem>           parent;
        QPointer<QQuickItem>           presentation;
        bool                           widthExplicit  = false;
        bool                           heightExplicit = false;
        QList<QMetaObject::Connection> sizeConnections;
    };
    PageStackEntries                       entries;
    QMap<quint64, Page>                    pages;
    QPointer<QQuickItem>                   initial;
    quint64                                notifiedCurrent = 0;
    int                                    notifiedDepth   = 0;
    bool                                   operating       = false;
    bool                                   closing         = false;
    bool                                   notifiedBusy    = false;
    std::array<QPointer<QQmlComponent>, 3> definitions;
    QPointer<PageMotion>                   motion;
    QMetaObject::Connection                previewPolicy;
    quint64                                motionId = 0, nextPreview = 0;
    QString                                previewId, seekId, sizeSeek;
    std::optional<bool>                    decision;
    bool                                   defaultMotion = false, backGesture = false;
    TransitionMode                         mode = Animated;
    QPointer<SizeTransition>               sizeTransition;
    QList<QMetaObject::Connection>         sizeConnections;
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

PageStack::PageStack(QQuickItem* parent)
    : Control(parent), m_state(std::make_shared<State>()), m_input(this) {
    setFlag(ItemIsFocusScope);
    connect(&m_state->entries, &PageStackEntries::changed, this, &PageStack::synchronize);
    connect(&m_state->entries, &PageStackEntries::entryRemoved, this, [this](quint64 id) {
        removePage(id);
    });
    connect(&m_state->entries,
            &PageStackEntries::transitionRequested,
            this,
            &PageStack::startTransition);
    m_input.acceptsInput = [this] {
        return backGestureEnabled() && depth() > 1 &&
               (! m_gesture.isEmpty() || ! busy() || (preview() && m_state->decision == false));
    };
    m_input.click().keyboardEnabled = false;
    auto& drag                      = m_input.drag();
    drag.axes                       = Qt::Horizontal;
    drag.canStart                   = [this](QPointF start, QPointF current) {
        return (mirrored() ? start.x() >= width() - 24 && current.x() < start.x()
                           : start.x() <= 24 && current.x() > start.x());
    };
    drag.started = [this](QPointF point) {
        const QPointer<PageStack> guard(this);
        const auto                id = beginPopPreview();
        if (! guard) return;
        m_gesture         = id;
        m_gesturePoint    = point;
        m_gestureFraction = currentMotion() && currentMotion()->timeline()
                                ? currentMotion()->timeline()->fraction()
                                : 0;
        if (m_gesture.isEmpty()) m_input.cancelActivation();
    };
    drag.moved = [this](QPointF point) {
        const auto delta  = point.x() - std::exchange(m_gesturePoint, point).x();
        m_gestureFraction = std::clamp(m_gestureFraction + (mirrored() ? -delta : delta) /
                                                               std::max(qreal(1), width()),
                                       qreal(0),
                                       qreal(1));
        updatePopPreview(m_gesture, m_gestureFraction);
    };
    drag.finished = [this](QPointF, bool cancelled) {
        const auto id = std::exchange(m_gesture, {});
        const auto velocity =
            (mirrored() ? -1 : 1) * m_input.drag().velocity.x() / std::max(qreal(1), width());
        if (! cancelled && (velocity > .7 || (velocity >= -.7 && m_gestureFraction > .5)))
            commitPopPreview(id, velocity);
        else
            cancelPopPreview(id, cancelled ? 0 : velocity);
    };
    m_input.observeChildPointerInput();
}
MaterialPageStack::MaterialPageStack(QQuickItem* parent): PageStack(parent) {
    enableDefaultMotion();
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
int             PageStack::depth() const { return m_state->entries.depth(); }
bool            PageStack::busy() const { return m_state->notifiedBusy; }
SizeTransition* PageStack::sizeTransition() const { return m_state->sizeTransition; }
void            PageStack::setSizeTransition(SizeTransition* transition) {
    const auto state = m_state;
    if (state->sizeTransition == transition) return;
    const QPointer<PageStack>      owner(this);
    const QPointer<SizeTransition> candidate(transition);
    const auto                     previous = state->sizeTransition;
    if (preview()) abortPreview();
    if (! owner || state->sizeTransition != previous) return;
    transition = candidate;
    for (const auto& connection : state->sizeConnections) disconnect(connection);
    state->sizeConnections.clear();
    state->sizeTransition = transition;
    if (transition) {
        state->sizeConnections.append(
            connect(transition, &SizeTransition::activeChanged, this, [this] {
                QTimer::singleShot(0, this, &PageStack::tryFinishTransition);
            }));
        state->sizeConnections.append(connect(transition, &QObject::destroyed, this, [this] {
            QPointer<PageStack> guard(this);
            Q_EMIT sizeTransitionChanged();
            if (guard) QTimer::singleShot(0, this, &PageStack::tryFinishTransition);
        }));
    }
    QPointer<PageStack> guard(this);
    Q_EMIT sizeTransitionChanged();
    if (guard) QTimer::singleShot(0, this, &PageStack::tryFinishTransition);
}
QSizeF PageStack::destinationSize() const {
    const auto t    = m_state->entries.transition();
    const auto item = t && t->preview && m_state->decision != false
                          ? m_state->entries.entry(t->to).item.data()
                          : currentItem();
    return item ? QSizeF(item->implicitWidth(), item->implicitHeight()) : QSizeF(0, 0);
}
QSizeF PageStack::measureImplicitContent() const { return destinationSize(); }
#define QM_STACK_MOTION(Name, Setter, Index)                                       \
    QQmlComponent* PageStack::Name() const { return m_state->definitions[Index]; } \
    void           PageStack::Setter(QQmlComponent* c) {                           \
        if (m_state->definitions[Index] == c) return;                              \
        m_state->definitions[Index] = c;                                           \
        emit motionsChanged();                                                     \
    }
QM_STACK_MOTION(pushMotion, setPushMotion, 0)
QM_STACK_MOTION(popMotion, setPopMotion, 1)
QM_STACK_MOTION(replaceMotion, setReplaceMotion, 2)
#undef QM_STACK_MOTION
void        PageStack::enableDefaultMotion() { m_state->defaultMotion = true; }
PageMotion* PageStack::currentMotion() const { return m_state->motion; }
bool        PageStack::preview() const {
    const auto t = m_state->entries.transition();
    return t && t->preview;
}
bool PageStack::settling() const { return preview() && m_state->decision.has_value(); }
bool PageStack::backGestureEnabled() const { return m_state->backGesture; }
void PageStack::setBackGestureEnabled(bool v) {
    if (v == m_state->backGesture) return;
    m_state->backGesture = v;
    QPointer<PageStack> guard(this);
    if (! v) m_input.cancelActivation();
    if (guard) emit backGestureEnabledChanged();
}
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
        state->entries.finish(transition->id, ! transition->preview);
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
        state->decision.reset();
        emit previewChanged();
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
                                { item,
                                  item->parentItem(),
                                  new QQuickItem(this),
                                  p->widthValid(),
                                  p->heightValid() });
            auto&      page    = state->pages[entry.id];
            const auto measure = [this, item] {
                if (currentItem() == item ||
                    (m_state->entries.transition() &&
                     m_state->entries.entry(m_state->entries.transition()->to).item == item)) {
                    QPointer<PageStack> guard(this);
                    updateImplicitMetrics();
                    if (guard) emit destinationSizeChanged();
                }
            };
            page.sizeConnections.append(
                connect(item, &QQuickItem::implicitWidthChanged, this, measure));
            page.sizeConnections.append(
                connect(item, &QQuickItem::implicitHeightChanged, this, measure));
            attached(item)->m_view = this;
            Q_EMIT entryAdded(entry.id, item);
            if (! guard || ! item) return;
            // Publish ownership before reparenting can invoke application callbacks.
            attached(item)->update(this, state->entries.indexOf(entry.id), PageStack::Activating);
            if (! guard || ! item) return;
            // Reparenting from a hidden cache must not reveal an unsized page.
            item->setVisible(false);
            if (! guard || ! item) return;
            item->setParentItem(state->pages.value(entry.id).presentation);
            if (! guard || ! item) return;
        }
    }
    layoutPages();
    if (! guard) return;
    updateImplicitMetrics();
    if (! guard) return;
    emit destinationSizeChanged();
    if (! guard) return;
    for (const auto& entry : state->entries.entries()) {
        QPointer<QQuickItem> item = entry.item;
        if (! item) continue;
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
    if (! busy() || preview()) return false;
    if (event->type() == QEvent::MouseButtonPress) return true;
    if (event->type() == QEvent::UngrabMouse) return false;
    return item->window() && ! item->window()->mouseGrabberItem();
}
void PageStack::startTransition(quint64 id) {
    const auto state = m_state;
    const auto t     = state->entries.transition();
    if (! t || t->id != id) return;
    QPointer<PageStack> guard(this);
    if (state->mode == Immediate && ! t->preview) {
        if (sizeTransition()) sizeTransition()->complete();
        if (guard) state->entries.finish(id);
        return;
    }
    state->motionId   = id;
    const int index   = t->operation == PageStackEntries::Operation::Push      ? 0
                        : t->operation == PageStackEntries::Operation::Replace ? 2
                                                                               : 1;
    auto*     context = new PageMotionContext(
        state->entries.entry(t->from).item,
        state->entries.entry(t->to).item,
        t->operation == PageStackEntries::Operation::Push      ? PageMotionContext::Push
        : t->operation == PageStackEntries::Operation::Replace ? PageMotionContext::Replace
        : t->operation == PageStackEntries::Operation::Pop     ? PageMotionContext::Pop
                                                               : PageMotionContext::Clear,
        this);
    context->setViewport(size());
    PageMotion* motion = nullptr;
    if (auto* definition = state->definitions[index].data()) {
        auto* object = definition->createWithInitialProperties(
            { { "context", QVariant::fromValue(context) } }, qmlContext(this));
        if (! guard) {
            delete object;
            return;
        }
        motion = qobject_cast<PageMotion*>(object);
        if (! motion) {
            qWarning() << "PageStack motion must create PageMotion:" << definition->errorString();
            delete object;
        }
    }
    if (! motion) {
        motion = new PageMotion(this);
        motion->setContext(context);
        if (! state->defaultMotion && ! t->preview) motion->setSpec(Motion::snap());
    }
    motion->setParent(this);
    context->setParent(motion);
    state->motion  = motion;
    auto* settings = qobject_cast<Theme*>(qmlAttachedPropertiesObject<Theme>(this, true))->motion();
    motion->setSettings(settings);
    auto* timeline = motion->timeline();
    connect(motion, &PageMotion::frameChanged, this, &PageStack::applyMotion);
    connect(motion, &PageMotion::timelineChanged, this, [this, id] {
        if (m_state->motionId == id) completeTransition();
    });
    connect(motion, &QObject::destroyed, this, [this, id] {
        if (m_state->motionId == id) completeTransition();
    });
    state->previewPolicy = connect(settings, &MotionSettings::changed, this, [this, settings, id] {
        if (m_state->motionId == id && preview() && ! settling() &&
            (settings->durationScale() == 0 || settings->reducedMotion()))
            abortPreview();
    });
    if (timeline) {
        connect(timeline, &SeekableTransition::changed, this, [this, id] {
            if (m_state->motionId == id)
                QTimer::singleShot(0, this, &PageStack::tryFinishTransition);
        });
        connect(timeline, &SeekableTransition::ended, this, [this, id] {
            if (m_state->motionId == id) tryFinishTransition();
        });
        if (t->preview) {
            state->seekId = timeline->beginSeek("to");
            if (! guard || state->motionId != id) return;
            if (sizeTransition())
                state->sizeSeek = sizeTransition()->beginSeek(sizeTransition()->targetSize());
        } else
            timeline->animateTo("to");
    }
    if (! guard || state->motionId != id) return;
    applyMotion();
    if (! guard || state->motionId != id) return;
    state->notifiedBusy = true;
    setFiltersChildMouseEvents(true);
    emit currentMotionChanged();
    if (! guard) return;
    emit busyChanged();
    if (! guard) return;
    emit previewChanged();
    if (guard) QTimer::singleShot(0, this, &PageStack::tryFinishTransition);
}
void PageStack::applyMotion() {
    const auto                 state  = m_state;
    const auto                 t      = state->entries.transition();
    const QPointer<PageMotion> motion = state->motion;
    if (! t || ! motion) return;
    const QPointer<PageStack> guard(this);
    for (auto id : { t->from, t->to }) {
        const auto wrapper = state->pages.value(id).presentation;
        if (! wrapper) continue;
        const bool entering = id == t->to;
        wrapper->setPosition(entering ? motion->enterOffset() : motion->exitOffset());
        if (! guard || ! motion || ! wrapper) return;
        wrapper->setScale(entering ? motion->enterScale() : motion->exitScale());
        if (! guard || ! motion || ! wrapper) return;
        wrapper->setOpacity(entering ? motion->enterOpacity() : motion->exitOpacity());
        if (! guard || ! motion) return;
    }
    if (sizeTransition() && ! state->sizeSeek.isEmpty() && motion->timeline())
        sizeTransition()->seek(state->sizeSeek, motion->timeline()->fraction());
}
QString PageStack::beginPopPreview() {
    const auto                state = m_state;
    const QPointer<PageStack> guard(this);
    if (state->closing || state->operating || depth() < 2) return {};
    if (state->entries.transition()) {
        if (! preview() || state->decision != false || ! state->motion ||
            ! state->motion->timeline() || (state->sizeSeek.isEmpty() && sizeTransition()))
            return {};
        state->decision.reset();
        state->seekId = state->motion->timeline()->beginSeek("to");
    } else {
        state->decision.reset();
        state->mode         = Animated;
        state->operating    = true;
        const auto accepted = state->entries.beginPopPreview();
        state->operating    = false;
        if (! guard || ! accepted) return {};
    }
    if (! guard) return {};
    if (! state->motion || state->seekId.isEmpty()) {
        abortPreview();
        return {};
    }
    state->previewId  = QString::number(++state->nextPreview);
    const auto result = state->previewId;
    emit       previewChanged();
    return result;
}
bool PageStack::updatePopPreview(QString id, qreal progress) {
    const auto state = m_state;
    if (id.isEmpty() || id != state->previewId || state->decision || ! state->motion ||
        ! state->motion->timeline())
        return false;
    return state->motion->timeline()->seek(state->seekId, progress);
}
bool PageStack::commitPopPreview(QString id, qreal velocity) {
    return settlePreview(id, true, velocity);
}
bool PageStack::cancelPopPreview(QString id, qreal velocity) {
    return settlePreview(id, false, velocity);
}
bool PageStack::settlePreview(QString id, bool commit, qreal velocity) {
    const auto state = m_state;
    if (! std::isfinite(velocity) || id.isEmpty() || id != state->previewId || state->decision ||
        ! state->motion || ! state->motion->timeline())
        return false;
    state->decision = commit;
    const QPointer<PageStack> guard(this);
    auto*                     settings = state->motion->timeline()->settings();
    const auto                spec     = settings ? settings->spatial() : Motion::spring();
    state->motion->timeline()->settle(state->seekId,
                                      commit ? SeekableTransition::Destination
                                             : SeekableTransition::Origin,
                                      velocity,
                                      spec);
    if (guard) emit previewChanged();
    return true;
}
void PageStack::abortPreview() {
    if (! preview()) return;
    m_state->decision = false;
    completeTransition();
}
void PageStack::tryFinishTransition() {
    const auto state = m_state;
    if (state->closing || state->operating || ! state->motionId) return;
    if (state->motion && state->motion->timeline() && state->motion->timeline()->active()) return;
    if (preview() && ! state->decision) {
        abortPreview();
        return;
    }
    const QPointer<PageStack> guard(this);
    if (! state->sizeSeek.isEmpty()) {
        updateImplicitMetrics();
        if (! guard) return;
        emit destinationSizeChanged();
        if (! guard) return;
        const auto seek = std::exchange(state->sizeSeek, {});
        if (sizeTransition()) sizeTransition()->endSeek(seek, true);
        if (! guard) return;
    }
    if (sizeTransition() && sizeTransition()->active()) return;
    completeTransition();
}
void PageStack::stopMotions() {
    const auto state = m_state;
    state->motionId  = 0;
    disconnect(state->previewPolicy);
    state->previewPolicy = {};
    state->previewId.clear();
    state->seekId.clear();
    const QPointer<PageStack> guard(this);
    const auto                motion = state->motion;
    state->motion                    = nullptr;
    if (motion) {
        disconnect(motion, nullptr, this, nullptr);
        if (motion->timeline()) disconnect(motion->timeline(), nullptr, this, nullptr);
        if (motion->timeline()) motion->timeline()->stop();
        if (! guard) return;
        if (motion) motion->deleteLater();
    }
    const auto seek = std::exchange(state->sizeSeek, {});
    if (sizeTransition() && ! seek.isEmpty()) sizeTransition()->endSeek(seek, false);
    if (! guard) return;
    const auto pages = state->pages;
    for (const auto& page : pages) {
        if (page.presentation) page.presentation->setPosition({});
        if (! guard) return;
        if (page.presentation) page.presentation->setOpacity(1);
        if (! guard) return;
        if (page.presentation) page.presentation->setScale(1);
        if (! guard) return;
    }
    if (! state->closing) emit currentMotionChanged();
}
void PageStack::completeTransition() {
    const auto state = m_state;
    if (state->operating || state->closing) return;
    const auto t = state->entries.transition();
    if (! t) {
        if (sizeTransition()) sizeTransition()->complete();
        return;
    }
    state->operating  = true;
    const bool commit = ! t->preview || state->decision.value_or(false);
    state->decision   = commit;
    QPointer<PageStack> guard(this);
    updateImplicitMetrics();
    if (! guard) return;
    emit destinationSizeChanged();
    if (! guard) return;
    const auto seek = std::exchange(state->sizeSeek, {});
    if (sizeTransition() && ! seek.isEmpty()) sizeTransition()->endSeek(seek, true);
    if (! guard) return;
    stopMotions();
    if (guard && sizeTransition()) sizeTransition()->complete();
    if (guard) state->entries.finish(t->id, commit);
    state->operating = false;
    state->decision.reset();
    if (guard) emit previewChanged();
}
void PageStack::layoutPages() {
    const auto          state = m_state;
    const auto          ids   = state->pages.keys();
    QPointer<PageStack> guard(this);
    for (auto id : ids) {
        const auto page = state->pages.value(id);
        if (! page.item) continue;
        if (page.presentation) page.presentation->setSize(size());
        if (! guard) return;
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
    if (guard && currentMotion() && currentMotion()->context())
        currentMotion()->context()->setViewport(size());
}
void PageStack::removePage(quint64 id) {
    const auto state = m_state;
    const auto page  = state->pages.take(id);
    for (const auto& connection : page.sizeConnections) disconnect(connection);
    QPointer<PageStack>         guard(this);
    QPointer<PageStackAttached> properties = page.item ? attached(page.item) : nullptr;
    if (properties) properties->m_removing = true;
    if (page.item) page.item->setVisible(false);
    if (page.item && ! page.widthExplicit) page.item->resetWidth();
    if (page.item && ! page.heightExplicit) page.item->resetHeight();
    if (page.item) page.item->setParentItem(page.parent);
    if (page.presentation) page.presentation->deleteLater();
    if (properties) properties->update(nullptr, -1, PageStack::Inactive);
    if (properties) Q_EMIT properties->removed();
    if (properties) properties->m_removing = false;
    if (guard) Q_EMIT entryRemoved(id);
}
} // namespace qml_material
