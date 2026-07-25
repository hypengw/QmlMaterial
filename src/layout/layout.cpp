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

QQuickItem* LayoutAttached::visibilitySource() const { return m_visibility_source; }

void LayoutAttached::setVisibilitySource(QQuickItem* source) {
    if (m_visibility_source_set && m_visibility_source == source) {
        return;
    }

    QObject::disconnect(m_visibility_source_destroyed);
    m_visibility_source_set = true;
    m_visibility_source     = source;
    if (source) {
        m_visibility_source_destroyed = connect(source, &QObject::destroyed, this, [this]() {
            m_visibility_source = nullptr;
            Q_EMIT visibilitySourceChanged();
        });
    } else {
        m_visibility_source_destroyed = {};
    }
    Q_EMIT visibilitySourceChanged();
}

void LayoutAttached::resetVisibilitySource() {
    if (! m_visibility_source_set) {
        return;
    }

    QObject::disconnect(m_visibility_source_destroyed);
    m_visibility_source_destroyed = {};
    m_visibility_source           = nullptr;
    m_visibility_source_set       = false;
    Q_EMIT visibilitySourceChanged();
}

bool LayoutAttached::isVisibilitySourceSet() const { return m_visibility_source_set; }

Layout::Layout(QObject* parent): QObject(parent) {}

LayoutAttached* Layout::qmlAttachedProperties(QObject* object) {
    return new LayoutAttached(object);
}

} // namespace qml_material
