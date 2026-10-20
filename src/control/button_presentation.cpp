#include "qml_material/control/button_presentation.hpp"
#include "qml_material/util/qt.hpp"

namespace qml_material
{
ButtonPresentation::ButtonPresentation(QQuickItem* parent)
    : Control(parent), m_icon(new IconSpec(this)) {}
ButtonPresentation::~ButtonPresentation() { disconnectPresentation(); }
void ButtonPresentation::disconnectPresentation() {
    ++m_indicatorRevision;
    utils::disconnectAll(m_indicatorConnections);
}
QString ButtonPresentation::text() const { return m_text ? *m_text : defaultText(); }
void    ButtonPresentation::setText(const QString& value) {
    const auto old = text();
    m_text         = value;
    if (old != text()) Q_EMIT textChanged();
}
void ButtonPresentation::resetText() {
    const auto old = text();
    m_text.reset();
    if (old != text()) Q_EMIT textChanged();
}
void ButtonPresentation::defaultTextChanged() {
    if (! m_text) Q_EMIT textChanged();
}
void ButtonPresentation::setDisplay(Display value) {
    if (m_display == value) return;
    m_display = value;
    Q_EMIT displayChanged();
}
void ButtonPresentation::setIndicator(QQuickItem* value) {
    if (m_indicator == value) return;
    QPointer<ButtonPresentation> guard(this);
    QPointer<QQuickItem>         target(value);
    disconnectPresentation();
    const auto revision = m_indicatorRevision;
    auto       old      = m_indicator;
    m_indicator         = value;
    if (old && old->parentItem() == this) old->setParentItem(nullptr);
    if (! guard || revision != m_indicatorRevision) return;
    if (target) {
        if (! target->parentItem()) target->setParentItem(this);
        if (! guard || revision != m_indicatorRevision) return;
        if (target) {
            m_indicatorConnections.append(
                connect(target,
                        &QQuickItem::implicitWidthChanged,
                        this,
                        &ButtonPresentation::implicitIndicatorWidthChanged));
            m_indicatorConnections.append(
                connect(target,
                        &QQuickItem::implicitHeightChanged,
                        this,
                        &ButtonPresentation::implicitIndicatorHeightChanged));
            m_indicatorConnections.append(connect(target, &QObject::destroyed, this, [this] {
                m_indicator                           = nullptr;
                const auto                   revision = ++m_indicatorRevision;
                QPointer<ButtonPresentation> guard(this);
                Q_EMIT indicatorChanged();
                if (! guard || revision != m_indicatorRevision) return;
                Q_EMIT implicitIndicatorWidthChanged();
                if (guard && revision == m_indicatorRevision)
                    Q_EMIT implicitIndicatorHeightChanged();
            }));
        }
    }
    Q_EMIT indicatorChanged();
    if (! guard || revision != m_indicatorRevision) return;
    Q_EMIT implicitIndicatorWidthChanged();
    if (guard && revision == m_indicatorRevision) Q_EMIT implicitIndicatorHeightChanged();
}
} // namespace qml_material
