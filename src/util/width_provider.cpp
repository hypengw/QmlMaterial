#include "qml_material/util/width_provider.hpp"
#include "qml_material/core.hpp"

namespace qml_material
{
WidthProvider::WidthProvider(QObject* parent)
    : QObject(parent), m_width(0), m_total(0), m_minimum(160), m_spacing(0) {
    connect(
        this, &WidthProvider::totalChanged, this, &WidthProvider::refresh, Qt::DirectConnection);
}

auto WidthProvider::width() const noexcept -> qint32 { return m_width; }

auto WidthProvider::minimum() const noexcept -> qint32 { return m_minimum; }
void WidthProvider::setMinimum(qint32 v) {
    if (m_minimum != v) {
        m_minimum = v;
        minimumChanged();
    }
}
auto WidthProvider::total() const noexcept -> qint32 { return m_total; }
void WidthProvider::setTotal(qint32 v) {
    if (m_total != v) {
        m_total = v;
        totalChanged();
    }
}
auto WidthProvider::spacing() const noexcept -> qint32 { return m_spacing; }
void WidthProvider::setSpacing(qint32 v) {
    if (m_spacing != v) {
        m_spacing = v;
        spacingChanged();
    }
}

void WidthProvider::refresh() {
    auto old = m_width;
    if (m_minimum <= 0 || m_total <= 0) {
        return;
    }

    auto d     = (double)m_total / (m_minimum + m_spacing);
    auto count = (u64)(d);

    if (count == 0) {
        m_width = std::max(m_total, m_minimum);
    } else {
        auto spacing = (double)m_spacing / m_total;
        auto scale   = (d - count) / count + 1.0 + spacing;
        m_width      = m_minimum * scale;
    }
    if (old != m_width) {
        widthChanged();
    }
}

} // namespace qml_material

#include "qml_material/util/moc_width_provider.cpp"