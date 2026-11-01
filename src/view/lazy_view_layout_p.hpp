#pragma once

#include "viewport_controller.hpp"
#include "grid_layout_p.hpp"
#include "item_anchor_p.hpp"

namespace qml_material
{
class LazyViewLayout {
public:
    using Result                             = ViewportController::Result;
    virtual ~LazyViewLayout()                = default;
    virtual void            clear()          = 0;
    virtual ItemSnapshotPtr snapshot() const = 0;
    virtual Result          apply(ItemSnapshotPtr, qreal cross, qreal offset, qreal viewport) = 0;
    virtual Result applyValues(ItemSnapshotPtr next, const QVector<ItemChange>&, qreal cross,
                               qreal offset, qreal viewport) {
        return apply(std::move(next), cross, offset, viewport);
    }
    virtual void         setOrientation(Qt::Orientation) {}
    virtual bool         supportsOrientation(Qt::Orientation) const { return true; }
    virtual Result       measure(int index, qreal extent, qreal offset) = 0;
    virtual bool         needsMeasurement(int) const { return ! fixedItemExtent(); }
    virtual bool         estimated() const = 0;
    virtual bool         fixedItemExtent() const { return false; }
    virtual int          count() const                              = 0;
    virtual int          indexAt(qreal) const                       = 0;
    virtual qreal        offset(int) const                          = 0;
    virtual qreal        extent(int) const                          = 0;
    virtual qreal        totalExtent() const                        = 0;
    virtual QRectF       rect(int) const                            = 0;
    virtual ItemRange    visibleRange(qreal begin, qreal end) const = 0;
    virtual bool         discreteItems() const { return false; }
    virtual QVector<int> requestedIndices(qreal, qreal, qreal) const { return {}; }
    virtual bool         reachesEnd(qreal offset, qreal viewport, qreal cache) const {
        if (fixedItemExtent()) {
            const auto range = demandRange(offset, viewport, cache);
            return range.last >= 0 && range.last == count() - 1;
        }
        return offset + viewport + cache >= totalExtent();
    }
    virtual ItemRange requestedRange(qreal offset, qreal viewport, qreal cache) const {
        // Include zero-sized delegates so measurement can advance past them.
        return { indexAt(std::max(qreal(0), offset - cache)), count() - 1 };
    }
    virtual ItemRange demandRange(qreal offset, qreal viewport, qreal cache) const {
        return visibleRange(std::max(qreal(0), offset - cache), offset + viewport + cache);
    }
    virtual int neighbor(int index, int lineDelta, int slotDelta) const {
        return count() ? std::clamp(index + lineDelta + slotDelta, 0, count() - 1) : -1;
    }
    std::optional<qreal> positionAt(const QString& key, qreal viewport, qreal alignment,
                                    qreal extra) const {
        if (! snapshot() || ! std::isfinite(viewport) || viewport < 0 ||
            ! std::isfinite(alignment) || alignment < 0 || alignment > 1 || ! std::isfinite(extra))
            return {};
        const int index = snapshot()->indexOfKey(key);
        if (index < 0) return {};
        return offset(index) - (viewport - extent(index)) * alignment + extra;
    }
};

class LinearViewLayout final : public LazyViewLayout {
public:
    qreal           spacing  = 0;
    qreal           estimate = 48;
    void            clear() override { m_controller.clear(); }
    ItemSnapshotPtr snapshot() const override { return m_controller.snapshot(); }
    Result apply(ItemSnapshotPtr next, qreal cross, qreal offset, qreal viewport) override {
        auto result = m_controller.apply(std::move(next), cross, estimate, spacing, offset);
        if (result.accepted) m_cross = cross;
        return result;
    }
    Result applyValues(ItemSnapshotPtr next, const QVector<ItemChange>& changes, qreal cross,
                       qreal offset, qreal) override {
        auto result =
            m_controller.applyValues(std::move(next), changes, cross, estimate, spacing, offset);
        if (result.accepted) m_cross = cross;
        return result;
    }
    Result measure(int index, qreal extent, qreal offset) override {
        return m_controller.measure(index, extent, offset);
    }
    bool  needsMeasurement(int index) const override { return ! m_controller.measured(index); }
    bool  estimated() const override { return m_controller.estimated(); }
    int   count() const override { return m_controller.layout().count(); }
    int   indexAt(qreal position) const override { return m_controller.layout().indexAt(position); }
    qreal offset(int index) const override { return m_controller.layout().offset(index); }
    qreal extent(int index) const override { return m_controller.layout().extent(index); }
    qreal totalExtent() const override { return m_controller.layout().totalExtent(); }
    QRectF rect(int index) const override { return { 0, offset(index), m_cross, extent(index) }; }
    ItemRange visibleRange(qreal begin, qreal end) const override {
        auto range = m_controller.layout().visibleRange(begin, end);
        return { range.first, range.last };
    }
    ItemRange requestedRange(qreal offset, qreal viewport, qreal cache) const override {
        return LazyViewLayout::requestedRange(offset, viewport, cache);
    }

private:
    ViewportController m_controller;
    qreal              m_cross = 0;
};

class GridViewLayout final : public LazyViewLayout {
public:
    int               fixedSlotCount = 0;
    qreal             minimum = 140, itemExtent = 48, mainSpacing = 0, crossSpacing = 0;
    bool              mirrored    = false;
    Qt::Orientation   orientation = Qt::Vertical;
    void              setOrientation(Qt::Orientation value) override { orientation = value; }
    const GridLayout& grid() const { return m_grid; }
    void              clear() override {
        m_snapshot.reset();
        m_grid = {};
    }
    ItemSnapshotPtr snapshot() const override { return m_snapshot; }
    Result apply(ItemSnapshotPtr next, qreal cross, qreal offset, qreal viewport) override {
        if (! next || ! std::isfinite(offset)) return { false, offset };
        GridLayout grid;
        if (! grid.reset(next->count(),
                         cross,
                         fixedSlotCount,
                         minimum,
                         itemExtent,
                         mainSpacing,
                         crossSpacing,
                         mirrored && orientation == Qt::Vertical,
                         mirrored && orientation == Qt::Horizontal))
            return { false, offset };
        const auto visible = m_grid.visibleRange(offset, offset + m_grid.extent(0));
        const int  oldIndex =
            visible.first >= 0
                ? (m_grid.reversed() ? visible.last - visible.last % m_grid.fixedSlotCount()
                                     : visible.first)
                : m_grid.indexAt(offset);
        const auto anchor =
            ItemAnchor::capture(m_snapshot, next, oldIndex, offset, [this](int index) {
                return m_grid.offset(index);
            });
        qreal corrected = next->count() ? offset : 0;
        if (! anchor.key.isEmpty())
            corrected = grid.offset(next->indexOfKey(anchor.key)) - anchor.screenOffset;
        else if (! m_snapshot && grid.reversed())
            corrected = std::max(qreal(0), grid.totalExtent() - viewport);
        m_snapshot = std::move(next);
        m_grid     = grid;
        return { true, corrected };
    }
    Result    measure(int, qreal, qreal offset) override { return { true, offset }; }
    bool      estimated() const override { return false; }
    bool      fixedItemExtent() const override { return true; }
    int       count() const override { return m_grid.count(); }
    int       indexAt(qreal position) const override { return m_grid.indexAt(position); }
    qreal     offset(int index) const override { return m_grid.offset(index); }
    qreal     extent(int index) const override { return m_grid.extent(index); }
    qreal     totalExtent() const override { return m_grid.totalExtent(); }
    QRectF    rect(int index) const override { return m_grid.rect(index); }
    ItemRange visibleRange(qreal begin, qreal end) const override {
        return m_grid.visibleRange(begin, end);
    }
    ItemRange requestedRange(qreal offset, qreal viewport, qreal cache) const override {
        return m_grid.requestedRange(offset, viewport, cache);
    }
    ItemRange demandRange(qreal offset, qreal viewport, qreal cache) const override {
        return requestedRange(offset, viewport, cache);
    }
    int neighbor(int index, int lineDelta, int slotDelta) const override {
        return m_grid.neighbor(index, lineDelta, slotDelta);
    }

private:
    ItemSnapshotPtr m_snapshot;
    GridLayout      m_grid;
};
} // namespace qml_material
