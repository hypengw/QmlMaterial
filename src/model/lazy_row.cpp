#include "qml_material/model/lazy_row.hpp"
#include <QPointer>

namespace qml_material
{
void LazyRow::bind(const ItemSnapshotPtr& snapshot, int index) {
    if (! snapshot || index < 0 || index >= snapshot->count()) return;
    const auto generation   = ++m_generation;
    const bool activating   = ! m_active;
    const auto record       = snapshot->recordAt(index);
    const bool changedUsage = ! m_active || m_record.key != record.key ||
                              m_record.contentRevision != record.contentRevision;
    const bool changedView  = changedUsage && m_view;
    if (changedUsage) {
        m_usage = Cached;
        m_view  = nullptr;
    }
    m_snapshot = snapshot;
    m_record   = record;
    m_index    = index;
    m_revision = snapshot->revision;
    m_active   = true;
    QPointer<LazyRow> guard(this);
    Q_EMIT changed();
    if (guard && m_generation == generation && changedUsage) Q_EMIT usageChanged();
    if (guard && m_generation == generation && changedView) Q_EMIT viewChanged();
    if (guard && m_generation == generation && activating) Q_EMIT activeChanged();
}
void LazyRow::pool() {
    if (! m_active) return;
    const auto generation  = ++m_generation;
    m_active               = false;
    m_usage                = Pooled;
    const bool changedView = ! m_view.isNull();
    m_view                 = nullptr;
    QPointer<LazyRow> guard(this);
    Q_EMIT usageChanged();
    if (guard && m_generation == generation && changedView) Q_EMIT viewChanged();
    if (guard && m_generation == generation) Q_EMIT activeChanged();
}
void LazyRow::setUsage(Usage usage, QQuickItem* view) {
    if (! m_active) return;
    const auto generation   = m_generation;
    const bool changedUsage = m_usage != usage;
    const bool changedView  = m_view != view;
    m_usage                 = usage;
    m_view                  = view;
    QPointer<LazyRow> guard(this);
    if (changedView) Q_EMIT viewChanged();
    if (guard && m_generation == generation && changedUsage) Q_EMIT usageChanged();
}
} // namespace qml_material
