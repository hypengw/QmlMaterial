#include "qml_material/layout/width_provider.hpp"
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
#define X(Name) \
    connect(    \
        this, &WidthProvider::Name##Changed, this, &WidthProvider::refresh, Qt::DirectConnection)
    X(minimum);
    X(leftMargin);
    X(rightMargin);
    X(total);
    X(spacing);
#undef X
}

auto WidthProvider::width() const noexcept -> double { return m_width; }
auto WidthProvider::column() const noexcept -> qint32 { return m_column; }

auto WidthProvider::minimum() const noexcept -> double { return m_minimum; }
void WidthProvider::setMinimum(double v) {
    if (! qFuzzyCompare(m_minimum, v)) {
        m_minimum = v;
        minimumChanged();
    }
}
auto WidthProvider::leftMargin() const noexcept -> double { return m_left_margin; }
void WidthProvider::setLeftMargin(double v) {
    if (! qFuzzyCompare(m_left_margin, v)) {
        m_left_margin = v;
        leftMarginChanged();
    }
}
auto WidthProvider::rightMargin() const noexcept -> double { return m_right_margin; }
void WidthProvider::setrightMargin(double v) {
    if (! qFuzzyCompare(m_right_margin, v)) {
        m_right_margin = v;
        rightMarginChanged();
    }
}

auto WidthProvider::total() const noexcept -> double { return m_total; }
void WidthProvider::setTotal(double v) {
    if (! qFuzzyCompare(m_total, v)) {
        m_total = v;
        totalChanged();
    }
}
auto WidthProvider::spacing() const noexcept -> double { return m_spacing; }
void WidthProvider::setSpacing(double v) {
    if (! qFuzzyCompare(m_spacing, v)) {
        m_spacing = v;
        spacingChanged();
    }
}

auto WidthProvider::calculateX(qint32 column) noexcept -> qint32 {
    return m_left_margin + column * (m_width + m_spacing);
}

void WidthProvider::refresh() {
    const auto old_width  = m_width;
    const auto old_column = m_column;
    const auto total      = m_total - m_left_margin - m_right_margin;

    m_width  = 0;
    m_column = 1;

    if (m_minimum > 0 && total > 0 && m_minimum + m_spacing > 0) {
        // Find the maximum count satisfying:
        // count * minimum + (count - 1) * spacing <= total.
        const i64 count = std::max<i64>((total + m_spacing) / (m_minimum + m_spacing), 1);

        m_width  = static_cast<double>(total - (count - 1) * m_spacing) / count;
        m_column = count;
    }

    if (! qFuzzyCompare(old_width, m_width)) {
        widthChanged();
    }

    if (old_column != m_column) {
        columnChanged();
    }
}

} // namespace qml_material

#include "qml_material/layout/moc_width_provider.cpp"
