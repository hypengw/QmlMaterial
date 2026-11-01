#include "qml_material/model/lazy_row.hpp"
#include <QPointer>

namespace qml_material
{
void LazyRow::bind(const ItemSnapshotPtr& snapshot, int index, QQuickItem* view, Usage usage) {
    if (! snapshot || index < 0 || index >= snapshot->count()) return;
    const auto  generation             = ++m_generation;
    const bool  activating             = ! m_active;
    const auto& descriptor             = snapshot->descriptorAt(index);
    const bool  sameContent            = m_snapshot && m_record.key == descriptor.key &&
                                         m_record.reuseType == descriptor.reuseType &&
                                         m_record.contentRevision == descriptor.contentRevision;
    const bool  changedKey             = m_record.key != descriptor.key;
    const bool  changedIndex           = m_index != index;
    const bool  changedRevision        = m_revision != snapshot->revision;
    const bool  changedContentRevision = m_record.contentRevision != descriptor.contentRevision;
    const bool  changedUsage           = m_usage != usage;
    const bool  changedView            = m_view != view;
    const bool  reuseValue = sameContent && snapshot->sharesValueAt(index, *m_snapshot, m_index);
    m_usage                = usage;
    m_view                 = view;
    m_snapshot             = snapshot;
    if (! reuseValue) m_record = snapshot->recordAt(index);
    m_index    = index;
    m_revision = snapshot->revision;
    m_active   = true;
    QPointer<LazyRow> guard(this);
    const auto        current = [&] {
        return guard && m_generation == generation;
    };
    if (changedKey) Q_EMIT keyChanged();
    if (! current()) return;
    if (changedIndex) Q_EMIT indexChanged();
    if (! current()) return;
    if (changedContentRevision) Q_EMIT contentRevisionChanged();
    if (! current()) return;
    if (changedRevision) Q_EMIT revisionChanged();
    if (! current()) return;
    if (! sameContent) Q_EMIT valueChanged();
    if (! current()) return;
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
