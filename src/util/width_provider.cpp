#include "qml_material/util/width_provider.hpp"
#include "qml_material/core.hpp"

namespace qml_material
{
WidthProvider::WidthProvider(QObject* parent)
    : QObject(parent),
      m_column(1),
      m_width(0),
      m_total(0),
      m_minimum(160),
      m_spacing(0),
      m_left_margin(0),
      m_right_margin(0) {
    connect(
        this, &WidthProvider::totalChanged, this, &WidthProvider::refresh, Qt::DirectConnection);
}

auto WidthProvider::width() const noexcept -> qint32 { return m_width; }
auto WidthProvider::column() const noexcept -> qint32 { return m_column; }

auto WidthProvider::minimum() const noexcept -> qint32 { return m_minimum; }
void WidthProvider::setMinimum(qint32 v) {
    if (m_minimum != v) {
        m_minimum = v;
        minimumChanged();
    }
}
auto WidthProvider::leftMargin() const noexcept -> qint32 { return m_left_margin; }
void WidthProvider::setLeftMargin(qint32 v) {
    if (m_left_margin != v) {
        m_left_margin = v;
        leftMarginChanged();
    }
}
auto WidthProvider::rightMargin() const noexcept -> qint32 { return m_right_margin; }
void WidthProvider::setrightMargin(qint32 v) {
    if (m_right_margin != v) {
        m_right_margin = v;
        rightMarginChanged();
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

auto WidthProvider::calculateX(qint32 column) noexcept -> qint32 {
    return m_left_margin + column * (m_width + m_spacing);
}

void WidthProvider::refresh() {
    auto old   = m_width;
    auto total = m_total - m_left_margin - m_right_margin;

    if (m_minimum <= 0 || total <= 0) {
        return;
    }

    auto d     = (double)total / (m_minimum + m_spacing);
    auto count = (i64)(d);

    if (count == 0) {
        m_width = std::max(total, m_minimum);
    } else {
        auto spacing = (double)m_spacing / total;
        auto scale   = (d - count) / count + 1.0 + spacing;
        m_width      = m_minimum * scale;
    }
    if (old != m_width) {
        widthChanged();
    }
    count = std::max<i64>(count, 1);
    if (count != m_column) {
        m_column = count;
        columnChanged();
    }
}

} // namespace qml_material

#include "qml_material/util/moc_width_provider.cpp"