#include "qml_material/model/lazy_row.hpp"
#include <QPointer>

namespace qml_material
{
void LazyRow::bind(const ItemSnapshotPtr& snapshot, int index) {
    if (! snapshot || index < 0 || index >= snapshot->rows.size()) return;
    const auto generation = ++m_generation;
    const bool activating = ! m_active;
    m_record              = snapshot->rows[index];
    m_index               = index;
    m_revision            = snapshot->revision;
    m_active              = true;
    QPointer<LazyRow> guard(this);
    Q_EMIT changed();
    if (guard && m_generation == generation && activating) Q_EMIT activeChanged();
}
void LazyRow::pool() {
    if (! m_active) return;
    const auto generation = ++m_generation;
    m_active              = false;
    QPointer<LazyRow> guard(this);
    Q_EMIT activeChanged();
    if (guard && m_generation == generation) Q_EMIT pooled();
}
} // namespace qml_material
