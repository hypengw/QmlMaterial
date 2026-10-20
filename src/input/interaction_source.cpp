#include "qml_material/input/interaction_source.hpp"
#include <algorithm>
#include <atomic>
#include <utility>
#include <QThread>

namespace qml_material
{
namespace
{
quint64 nextIdentity() {
    static std::atomic<quint64> next { 0 };
    return ++next;
}
} // namespace
std::optional<QPointF> Interaction::positionFor(QQuickItem* target) const {
    if (! coordinateItem || ! target || coordinateItem->window() != target->window()) return {};
    if (! coordinateItem->window()) {
        auto root = [](QQuickItem* item) {
            while (item->parentItem()) item = item->parentItem();
            return item;
        };
        if (root(coordinateItem) != root(target)) return {};
    }
    return coordinateItem->mapToItem(target, position);
}
InteractionSource::InteractionSource(QObject* parent): QObject(parent) {}
InteractionSource::~InteractionSource() = default;
std::unique_ptr<InteractionEmitter> InteractionSource::emitter(QQuickItem* origin) {
    Q_ASSERT(thread() == QThread::currentThread());
    return std::unique_ptr<InteractionEmitter>(new InteractionEmitter(this, origin));
}
void InteractionSource::enqueue(const Interaction& event) {
    Q_ASSERT(thread() == QThread::currentThread());
    m_pending.append(event);
    if (std::exchange(m_scheduled, true)) return;
    QMetaObject::invokeMethod(this, &InteractionSource::deliver, Qt::QueuedConnection);
}
void InteractionSource::deliver() {
    QPointer<InteractionSource> guard(this);
    while (! m_pending.isEmpty()) {
        const auto event = m_pending.takeFirst();
        Q_EMIT interaction(event);
        if (! guard) return;
    }
    m_scheduled = false;
}
InteractionEmitter::InteractionEmitter(InteractionSource* source, QQuickItem* origin)
    : m_source(source), m_origin(origin), m_producer(nextIdentity()) {}
InteractionEmitter::~InteractionEmitter() { cancel(); }
quint64 InteractionEmitter::begin(Interaction::Kind kind, Interaction::Device device, int identity,
                                  QPointF position) {
    if (! m_source || kind == Interaction::None) return 0;
    Interaction event { kind,       Interaction::Begin, device,   nextIdentity(),
                        m_producer, identity,           position, m_origin };
    m_active.append(event);
    m_source->enqueue(event);
    return event.id;
}
void InteractionEmitter::end(quint64 id, Interaction::Phase phase) {
    if (! id || (phase != Interaction::End && phase != Interaction::Cancel)) return;
    const auto it = std::ranges::find(m_active, id, &Interaction::id);
    if (it == m_active.end()) return;
    auto event = *it;
    m_active.erase(it);
    event.phase = phase;
    if (m_source) m_source->enqueue(event);
}
void InteractionEmitter::cancel() {
    const auto active = std::exchange(m_active, {});
    if (! m_source) return;
    for (auto event : active) {
        event.phase = event.kind == Interaction::Hover || event.kind == Interaction::Focus
                          ? Interaction::End
                          : Interaction::Cancel;
        m_source->enqueue(event);
    }
}
bool InteractionEmitter::contains(quint64 id) const {
    return m_source && id && std::ranges::find(m_active, id, &Interaction::id) != m_active.end();
}
InteractionAccumulator::InteractionAccumulator(QObject* context, std::function<void()> changed)
    : m_context(context), m_changed(std::move(changed)) {}
InteractionAccumulator::~InteractionAccumulator() {
    QObject::disconnect(m_event);
    QObject::disconnect(m_destroyed);
}
void InteractionAccumulator::setSource(InteractionSource* source) {
    if (source == m_source) return;
    QObject::disconnect(m_event);
    QObject::disconnect(m_destroyed);
    m_source = source;
    m_active.clear();
    if (source) {
        m_event = QObject::connect(
            source, &InteractionSource::interaction, m_context, [this](const Interaction& event) {
                accept(event);
            });
        m_destroyed = QObject::connect(source, &QObject::destroyed, m_context, [this] {
            m_source = nullptr;
            m_active.clear();
            update();
        });
    }
    update();
}
void InteractionAccumulator::accept(const Interaction& event) {
    const auto it = std::ranges::find(m_active, event.id, &Interaction::id);
    if (event.phase == Interaction::Begin) {
        if (it != m_active.end()) return;
        m_active.append(event);
    } else {
        if (it == m_active.end()) return;
        m_active.erase(it);
    }
    update();
}
void InteractionAccumulator::update() {
    InteractionValues values;
    for (const auto& event : m_active) {
        values.pressed |= event.kind == Interaction::Press;
        values.hovered |= event.kind == Interaction::Hover;
        values.focused |= event.kind == Interaction::Focus;
        values.dragged |= event.kind == Interaction::Drag;
    }
    QPointer<QObject> guard(m_context);
    const auto        notify = m_changed;
    m_values                 = values;
    if (! guard) return;
    m_revision = m_revision.value() + 1;
    if (guard && notify) notify();
}
Interaction::Kind
InteractionAccumulator::latest(std::initializer_list<Interaction::Kind> kinds) const {
    (void)m_revision.value();
    for (auto it = m_active.crbegin(); it != m_active.crend(); ++it)
        if (std::ranges::find(kinds, it->kind) != kinds.end()) return it->kind;
    return Interaction::None;
}
QList<Interaction> InteractionAccumulator::active() const {
    (void)m_revision.value();
    return m_active;
}
InteractionObserver::InteractionObserver(QObject* parent)
    : QObject(parent), m_input(this, [this] {
          Q_EMIT changed();
      }) {}
void InteractionObserver::setSource(InteractionSource* source) {
    if (source == m_input.source()) return;
    QObject::disconnect(m_sourceDestroyed);
    QPointer<InteractionObserver> guard(this);
    QPointer<InteractionSource>   target(source);
    m_input.setSource(source);
    if (! guard || m_input.source() != target) return;
    if (target) {
        m_sourceDestroyed = connect(source, &QObject::destroyed, this, [this] {
            Q_EMIT sourceChanged();
        });
    }
    Q_EMIT sourceChanged();
}
} // namespace qml_material
