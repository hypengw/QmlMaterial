#include "qml_material/control/color_picker_button.hpp"
#include "qml_material/control/overlay.hpp"
#include "qml_material/util/qt.hpp"
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlInfo>
#include <QQmlProperty>
#include <QScopeGuard>
#include <utility>

namespace qml_material
{
ColorPickerButton::ColorPickerButton(QQuickItem* parent): CommandButton(parent) {
    m_windowConnection =
        connect(this, &QQuickItem::windowChanged, this, &ColorPickerButton::dismissPopup);
    m_visibilityConnection = connect(this, &QQuickItem::visibleChanged, this, [this] {
        if (! isVisible()) dismissPopup();
    });
}
ColorPickerButton::~ColorPickerButton() {
    disconnect(m_windowConnection);
    disconnect(m_visibilityConnection);
    utils::disconnectAll(m_dialogConnections);
    delete m_dialog.data();
    for (auto dialog : std::as_const(m_retiredDialogs)) delete dialog.data();
}
void ColorPickerButton::setColor(const QColor& value) {
    if (m_color == value) return;
    m_color = value;
    const QPointer<ColorPickerButton> guard(this);
    if (m_dialog) QQmlProperty::write(m_dialog, QStringLiteral("color"), value);
    if (guard) Q_EMIT colorChanged();
}
void ColorPickerButton::setShowAlpha(bool value) {
    if (m_showAlpha == value) return;
    m_showAlpha = value;
    const QPointer<ColorPickerButton> guard(this);
    if (m_dialog) QQmlProperty::write(m_dialog, QStringLiteral("showAlpha"), value);
    if (guard) Q_EMIT showAlphaChanged();
}
void ColorPickerButton::dismissPopup() {
    ++m_revision;
    const QPointer<ColorPickerButton> guard(this);
    cancelActivation();
    if (guard) releaseDialog();
}
void ColorPickerButton::releaseDialog() {
    const auto target = std::exchange(m_dialog, {});
    if (! target && m_dialogConnections.isEmpty()) return;
    const auto revision = ++m_revision;
    utils::disconnectAll(m_dialogConnections);
    const QPointer<ColorPickerButton> guard(this);
    if (target) {
        m_retiredDialogs.removeIf([](const auto& dialog) {
            return ! dialog;
        });
        m_retiredDialogs.append(target);
        // Retire the session now; release QML content after the close notification returns.
        target->deleteLater();
        target->setPresentationAllowed(this, false);
    }
    if (guard && revision == m_revision) Q_EMIT dialogChanged();
}
void ColorPickerButton::acceptColor() {
    if (! m_dialog || m_committed) return;
    m_committed                                = true;
    const auto                        target   = m_dialog;
    const auto                        revision = m_revision;
    const QPointer<ColorPickerButton> guard(this);
    const auto                        selected =
        QQmlProperty::read(target, QStringLiteral("selectedColor")).value<QColor>();
    if (! guard || ! target || m_dialog != target || revision != m_revision) return;
    setColor(selected);
    if (guard && target && m_dialog == target && revision == m_revision) Q_EMIT accepted(selected);
}
void ColorPickerButton::activate() {
    if (! canActivate()) return;
    const QPointer<ColorPickerButton> guard(this);
    const QPointer<QQuickWindow>      targetWindow(window());
    const auto                        revision = m_revision;
    CommandButton::activate();
    if (! guard || revision != m_revision || ! canActivate() || ! targetWindow ||
        window() != targetWindow || m_dialog || m_creating)
        return;
    auto* engine = qmlEngine(this);
    if (! engine) return;
    m_creating                         = true;
    const auto                 cleanup = qScopeGuard([guard] {
        if (guard) guard->m_creating = false;
    });
    const QPointer<QQuickItem> overlay(OverlayManager::get(targetWindow));
    if (! guard || ! overlay || revision != m_revision) return;
    QQmlComponent component(engine, "Qcm.Material", "ColorPickerDialog");
    if (! component.isReady()) {
        qmlWarning(this) << component.errorString();
        return;
    }
    QPointer<QObject> object = component.createWithInitialProperties(
        { { QStringLiteral("color"), m_color },
          { QStringLiteral("showAlpha"), m_showAlpha },
          { QStringLiteral("parent"), QVariant::fromValue(overlay.data()) } },
        qmlContext(this));
    QPointer<Dialog> target = qobject_cast<Dialog*>(object.data());
    if (! guard || ! target || revision != m_revision || ! canActivate() ||
        window() != targetWindow) {
        if (guard && component.isError()) qmlWarning(this) << component.errorString();
        delete object.data();
        return;
    }
    QQmlEngine::setObjectOwnership(target, QQmlEngine::CppOwnership);
    target->acquirePresentation(this);
    if (! guard || ! target || revision != m_revision) {
        delete target.data();
        return;
    }
    target->setPresentationAllowed(this, true);
    const auto dialogRevision = ++m_revision;
    m_dialog                  = target;
    m_committed               = false;
    m_creating                = false;
    m_dialogConnections.append(
        connect(target, &Dialog::accepted, this, &ColorPickerButton::acceptColor));
    m_dialogConnections.append(
        connect(target, &Popup::closed, this, &ColorPickerButton::releaseDialog));
    m_dialogConnections.append(
        connect(target, &QObject::destroyed, this, &ColorPickerButton::releaseDialog));
    Q_EMIT dialogChanged();
    if (guard && target && m_dialog == target && dialogRevision == m_revision && canActivate())
        target->open();
}
} // namespace qml_material
