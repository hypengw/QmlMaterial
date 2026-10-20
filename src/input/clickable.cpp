#include "qml_material/input/clickable.hpp"
#include <QGuiApplication>
#include <QStyleHints>

namespace qml_material
{
Clickable::Clickable(QQuickItem* parent)
    : InteractiveItem(parent),
      m_longClickInterval(QGuiApplication::styleHints()->mousePressAndHoldInterval()) {}
void Clickable::updateCombined() {
    inputHost().cancel();
    if (! m_longClickEnabled && ! m_doubleClickEnabled) {
        inputHost().click().resetCombined();
        return;
    }
    auto& combined              = inputHost().click().combined();
    combined.longClickEnabled   = m_longClickEnabled;
    combined.doubleClickEnabled = m_doubleClickEnabled;
    combined.longClickInterval  = m_longClickInterval;
    combined.longClicked        = [this] {
        Q_EMIT longClicked();
    };
    combined.doubleClicked = [this] {
        Q_EMIT doubleClicked();
    };
}
void Clickable::setLongClickEnabled(bool value) {
    if (m_longClickEnabled == value) return;
    m_longClickEnabled = value;
    updateCombined();
    Q_EMIT longClickEnabledChanged();
}
void Clickable::setDoubleClickEnabled(bool value) {
    if (m_doubleClickEnabled == value) return;
    m_doubleClickEnabled = value;
    updateCombined();
    Q_EMIT doubleClickEnabledChanged();
}
void Clickable::setLongClickInterval(int value) {
    value = std::max(1, value);
    if (m_longClickInterval == value) return;
    m_longClickInterval = value;
    updateCombined();
    Q_EMIT longClickIntervalChanged();
}
} // namespace qml_material
