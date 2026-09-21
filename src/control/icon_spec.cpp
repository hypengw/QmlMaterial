#include "qml_material/control/icon_spec.hpp"
#include "qml_material/util/qt.hpp"
#include <QQmlContext>
#include <QQmlEngine>

namespace qml_material
{
const ActionIcon* ActionIcon::contentOwner() const {
    return m_override && (m_override->m_local_name || m_override->m_local_source)
               ? m_override.data()
               : this;
}

QUrl ActionIcon::resolvedSource() const {
    const auto* owner   = contentOwner();
    const auto  source  = owner->m_local_source.value_or(QUrl());
    auto*       context = qmlContext(owner);
    if (! context && owner->parent()) context = qmlContext(owner->parent());
    return context && source.isRelative() && ! source.isEmpty() ? context->resolvedUrl(source)
                                                                : source;
}

void ActionIcon::setName(const QString& value) {
    if (m_local_name && *m_local_name == value) return;
    m_local_name = value;
    refresh();
}
void ActionIcon::resetName() {
    if (! m_local_name) return;
    m_local_name.reset();
    refresh();
}

void ActionIcon::setSource(const QUrl& value) {
    if (m_local_source && *m_local_source == value) return;
    m_local_source = value;
    refresh();
}
void ActionIcon::resetSource() {
    if (! m_local_source) return;
    m_local_source.reset();
    refresh();
}

void ActionIcon::setColor(const QColor& value) {
    if (m_local_color && *m_local_color == value) return;
    m_local_color = value;
    refresh();
}
void ActionIcon::resetColor() {
    if (! m_local_color) return;
    m_local_color.reset();
    refresh();
}

void ActionIcon::setCache(bool value) {
    if (m_local_cache && *m_local_cache == value) return;
    m_local_cache = value;
    refresh();
}
void ActionIcon::resetCache() {
    if (! m_local_cache) return;
    m_local_cache.reset();
    refresh();
}

void ActionIcon::setFill(bool value) {
    if (m_local_fill && *m_local_fill == value) return;
    m_local_fill = value;
    refresh();
}
void ActionIcon::resetFill() {
    if (! m_local_fill) return;
    m_local_fill.reset();
    refresh();
}

void ActionIcon::setWeight(int value) {
    if (m_local_weight && *m_local_weight == value) return;
    m_local_weight = value;
    refresh();
}
void ActionIcon::resetWeight() {
    if (! m_local_weight) return;
    m_local_weight.reset();
    refresh();
}

void ActionIcon::setRenderType(int value) {
    if (m_local_renderType && *m_local_renderType == value) return;
    m_local_renderType = value;
    refresh();
}
void ActionIcon::resetRenderType() {
    if (! m_local_renderType) return;
    m_local_renderType.reset();
    refresh();
}

void ActionIcon::setOverride(ActionIcon* value) {
    if (m_override == value || value == this) return;
    for (auto* current = value; current; current = current->m_override)
        if (current == this) return;
    utils::disconnectAll(m_connections);
    m_override = value;
    if (value) {
        m_connections.append(connect(value, &ActionIcon::changed, this, &ActionIcon::refresh));
        m_connections.append(connect(value, &QObject::destroyed, this, [this] {
            m_override = nullptr;
            refresh();
        }));
    }
    refresh();
}

void ActionIcon::refresh() {
    const auto  old_name            = m_name;
    const auto  old_source          = m_source;
    const auto  old_color           = m_color;
    const auto  old_cache           = m_cache;
    const auto  old_fill            = m_fill;
    const auto  old_weight          = m_weight;
    const auto  old_renderType      = m_renderType;
    const auto  old_resolved_source = m_resolved_source;
    const auto* content             = contentOwner();
    m_name                          = content->m_local_name.value_or(QString());
    m_source                        = content->m_local_source.value_or(QUrl());
    m_color                         = m_override && m_override->m_local_color
                                          ? *m_override->m_local_color
                                          : m_local_color.value_or(QColor(Qt::transparent));
    m_cache  = m_override && m_override->m_local_cache ? *m_override->m_local_cache
                                                       : m_local_cache.value_or(true);
    m_fill   = m_override && m_override->m_local_fill ? *m_override->m_local_fill
                                                      : m_local_fill.value_or(false);
    m_weight = m_override && m_override->m_local_weight ? *m_override->m_local_weight
                                                        : m_local_weight.value_or(QFont::Normal);
    m_renderType = m_override && m_override->m_local_renderType ? *m_override->m_local_renderType
                                                                : m_local_renderType.value_or(2);
    m_resolved_source = resolvedSource();
    QPointer<ActionIcon> guard(this);
    if (old_name != m_name) Q_EMIT nameChanged();
    if (! guard) return;
    if (old_source != m_source || old_resolved_source != m_resolved_source) Q_EMIT sourceChanged();
    if (! guard) return;
    if (old_color != m_color) Q_EMIT colorChanged();
    if (! guard) return;
    if (old_cache != m_cache) Q_EMIT cacheChanged();
    if (! guard) return;
    if (old_fill != m_fill) Q_EMIT fillChanged();
    if (! guard) return;
    if (old_weight != m_weight) Q_EMIT weightChanged();
    if (! guard) return;
    if (old_renderType != m_renderType) Q_EMIT renderTypeChanged();
    if (! guard) return;
    if (old_name != m_name || old_source != m_source) Q_EMIT contentChanged();
    if (! guard) return;
    // Explicit default values must also update controls consuming this action.
    Q_EMIT changed();
}

void IconSpec::setWidth(int value) {
    if (m_width && *m_width == value) return;
    m_width = value;
    QPointer<IconSpec> guard(this);
    Q_EMIT widthChanged();
    if (guard) Q_EMIT changed();
}
void IconSpec::resetWidth() {
    if (! m_width) return;
    m_width.reset();
    QPointer<IconSpec> guard(this);
    Q_EMIT widthChanged();
    if (guard) Q_EMIT changed();
}

void IconSpec::setHeight(int value) {
    if (m_height && *m_height == value) return;
    m_height = value;
    QPointer<IconSpec> guard(this);
    Q_EMIT heightChanged();
    if (guard) Q_EMIT changed();
}
void IconSpec::resetHeight() {
    if (! m_height) return;
    m_height.reset();
    QPointer<IconSpec> guard(this);
    Q_EMIT heightChanged();
    if (guard) Q_EMIT changed();
}
} // namespace qml_material
