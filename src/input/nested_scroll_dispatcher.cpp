#include "nested_scroll_dispatcher_p.hpp"
#include "scroll_coordinates_p.hpp"

namespace qml_material
{
QPointF dispatchNestedScroll(const ScrollDispatchContext& context, QPointF sceneDelta,
                             ScrollParticipant::Activity activity, QPointF velocity) {
    QPointF remaining = sceneDelta;
    auto    chain     = context.chain;
    if (activity == ScrollParticipant::Activity::Scroll) {
        for (qsizetype i = 0; i < chain.size(); ++i) {
            if (! chain[i] || ! wheelEnabledFor(chain[i])) {
                chain = chain.first(i);
                break;
            }
        }
    }
    const auto source = activity == ScrollParticipant::Activity::Drag ? NestedScrollConnection::Drag
                        : activity == ScrollParticipant::Activity::Scroll
                            ? NestedScrollConnection::Wheel
                            : NestedScrollConnection::Fling;
    QHash<NestedScroll*, QPointF> afterPre;
    auto                          current         = context.current;
    auto                          stop            = context.cancel;
    auto                          applyConnection = [&](QPointer<NestedScroll> config, bool pre) {
        if (! config || ! config->item()) {
            stop();
            return false;
        }
        if (! connectionFor(config)) return current();
        auto* item  = config->item();
        auto  local = vectorFromScene(item, remaining);
        if (! axesFor(config).testFlag(Qt::Horizontal)) local.setX(0);
        if (! axesFor(config).testFlag(Qt::Vertical)) local.setY(0);
        const auto basisX = vectorToScene(item, { 1, 0 });
        const auto basisY = vectorToScene(item, { 0, 1 });
        if (! finite(local) || ! finite(basisX) || ! finite(basisY) ||
            std::abs(basisX.x() * basisY.y() - basisX.y() * basisY.x()) < 1e-9) {
            stop();
            return false;
        }
        const auto consumed = vectorFromScene(item, afterPre.value(config) - remaining);
        context.applying(true);
        const auto requested = pre ? connectionFor(config)->preScroll(local, source)
                                   : connectionFor(config)->postScroll(consumed, local, source);
        const auto used      = boundedConsumption(requested, local);
        remaining -= basisX * used.x() + basisY * used.y();
        if (! current()) return false;
        context.applying(false);
        return true;
    };
    for (auto it = chain.crbegin(); it != chain.crend(); ++it) {
        if (! applyConnection(*it, true)) return sceneDelta - remaining;
        if (connectionFor(*it)) afterPre.insert(*it, remaining);
    }
    auto order = chain;
    if (activity == ScrollParticipant::Activity::Drag) {
        for (auto config : chain)
            if (config && config->enabled() && config->restoreOnReverse()) order.prepend(config);
    }
    const auto restoreCount = order.size() - chain.size();
    for (qsizetype index = 0; index < order.size(); ++index) {
        auto config = order[index];
        if (! config || ! config->item()) {
            stop();
            return sceneDelta - remaining;
        }
        QPointer<QQuickItem> item = config->item();
        ScrollParticipant    participant(item);
        if (! participant.available()) {
            stop();
            return sceneDelta - remaining;
        }
        auto local = vectorFromScene(item, remaining);
        if (activity == ScrollParticipant::Activity::Scroll && ! wheelEnabledFor(config)) break;
        if (index < restoreCount) {
            const auto back    = context.origins.value(config) - participant.position();
            const auto restore = [](qreal delta, qreal back) {
                return delta * back > 0
                           ? std::copysign(std::min(std::abs(delta), std::abs(back)), delta)
                           : 0;
            };
            local = { restore(local.x(), back.x()), restore(local.y(), back.y()) };
        }
        if (! axesFor(config).testFlag(Qt::Horizontal)) local.setX(0);
        if (! axesFor(config).testFlag(Qt::Vertical)) local.setY(0);
        const auto basisX = vectorToScene(item, { 1, 0 });
        const auto basisY = vectorToScene(item, { 0, 1 });
        if (! finite(local) || std::abs(basisX.x() * basisY.y() - basisX.y() * basisY.x()) < 1e-9) {
            qWarning("NestedScroll cannot consume a non-invertible item transform");
            stop();
            return sceneDelta - remaining;
        }
        context.applying(true);
        const auto used =
            participant.consume(local, activity, vectorFromScene(item, velocity), current);
        remaining -= basisX * used.x() + basisY * used.y();
        if (! current()) return sceneDelta - remaining;
        context.applying(false);
        if (config && ! used.isNull()) emit config->scrollConsumed(used);
        if (! current()) return sceneDelta - remaining;
        if (index >= restoreCount && ! applyConnection(config, false))
            return sceneDelta - remaining;
    }
    return sceneDelta - remaining;
}
} // namespace qml_material
