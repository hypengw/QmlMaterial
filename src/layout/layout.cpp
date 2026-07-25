#include "qml_material/layout/layout.hpp"

namespace qml_material
{

LayoutAttached::LayoutAttached(QObject* parent): QObject(parent) {}

bool LayoutAttached::fillWidth() const { return m_fill_width; }

void LayoutAttached::setFillWidth(bool fill) {
    if (m_fill_width == fill) {
        return;
    }
    m_fill_width = fill;
    Q_EMIT fillWidthChanged();
}

bool LayoutAttached::fillHeight() const { return m_fill_height; }

void LayoutAttached::setFillHeight(bool fill) {
    if (m_fill_height == fill) {
        return;
    }
    m_fill_height = fill;
    Q_EMIT fillHeightChanged();
}

Layout::Layout(QObject* parent): QObject(parent) {}

LayoutAttached* Layout::qmlAttachedProperties(QObject* object) {
    return new LayoutAttached(object);
}

} // namespace qml_material
