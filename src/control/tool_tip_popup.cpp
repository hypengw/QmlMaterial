#include "qml_material/control/tool_tip_popup.hpp"
#include <QQuickWindow>

namespace qml_material
{
ToolTipPopup::ToolTipPopup(QObject* parent): Popup(parent) {
    m_delayTimer.setSingleShot(true);
    m_timeoutTimer.setSingleShot(true);
    connect(&m_delayTimer, &QTimer::timeout, this, [this] {
        Popup::open();
    });
    connect(&m_timeoutTimer, &QTimer::timeout, this, &ToolTipPopup::close);
    connect(this, &Popup::opened, this, &ToolTipPopup::startTimeout);
}
void ToolTipPopup::setText(const QString& value) {
    if (m_text == value) return;
    m_text = value;
    Q_EMIT textChanged();
}
void ToolTipPopup::setDelay(int value) {
    if (m_delay == value) return;
    m_delay = value;
    Q_EMIT delayChanged();
}
void ToolTipPopup::setTimeout(int value) {
    if (m_timeout == value) return;
    m_timeout = value;
    if (value <= 0)
        m_timeoutTimer.stop();
    else if (isOpened())
        startTimeout();
    Q_EMIT timeoutChanged();
}
void ToolTipPopup::startTimeout() {
    if (m_timeout > 0) m_timeoutTimer.start(m_timeout);
}
void ToolTipPopup::open() {
    if (entering() || isOpened()) return;
    disconnect(m_windowVisibility);
    if (parentItem() && parentItem()->window()) {
        m_windowVisibility =
            connect(parentItem()->window(), &QWindow::visibleChanged, this, [this](bool visible) {
                if (! visible) dismissImmediately();
            });
    } else {
        Popup::open();
        return;
    }
    if (! isVisible() && m_delay > 0)
        m_delayTimer.start(m_delay);
    else {
        m_delayTimer.stop();
        Popup::open();
    }
}
void ToolTipPopup::close() {
    m_delayTimer.stop();
    m_timeoutTimer.stop();
    Popup::close();
}
void ToolTipPopup::dismissImmediately() {
    m_delayTimer.stop();
    m_timeoutTimer.stop();
    Popup::dismissImmediately();
}
void ToolTipPopup::show(const QString& text, int timeout) {
    QPointer<ToolTipPopup> guard(this);
    if (timeout >= 0) setTimeout(timeout);
    if (! guard) return;
    setText(text);
    if (guard) open();
}
void ToolTipPopup::hide() { close(); }
} // namespace qml_material
