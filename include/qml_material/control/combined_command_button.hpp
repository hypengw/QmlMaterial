#pragma once
#include "qml_material/control/command_button.hpp"

namespace qml_material
{
class QML_MATERIAL_API CombinedCommandButton : public CommandButton {
    Q_OBJECT
    QML_NAMED_ELEMENT(CombinedCommandButtonBase)
    Q_PROPERTY(bool longClickEnabled READ longClickEnabled WRITE setLongClickEnabled NOTIFY
                   combinedClickChanged FINAL)
    Q_PROPERTY(bool doubleClickEnabled READ doubleClickEnabled WRITE setDoubleClickEnabled NOTIFY
                   combinedClickChanged FINAL)
    Q_PROPERTY(int longClickInterval READ longClickInterval WRITE setLongClickInterval NOTIFY
                   combinedClickChanged FINAL)
public:
    explicit CombinedCommandButton(QQuickItem* parent = nullptr): CommandButton(parent) {}
    bool longClickEnabled() const { return m_long; }
    bool doubleClickEnabled() const { return m_double; }
    int  longClickInterval() const { return m_interval; }
    void setLongClickEnabled(bool value) {
        if (m_long != value) {
            m_long = value;
            configure();
        }
    }
    void setDoubleClickEnabled(bool value) {
        if (m_double != value) {
            m_double = value;
            configure();
        }
    }
    void setLongClickInterval(int value) {
        value = qMax(1, value);
        if (m_interval != value) {
            m_interval = value;
            configure();
        }
    }
    Q_SIGNAL void combinedClickChanged();
    Q_SIGNAL void longClicked();
    Q_SIGNAL void doubleClicked();

private:
    void configure() {
        const QPointer<CombinedCommandButton> guard(this);
        inputHost().cancelActivation();
        if (! guard) return;
        if (! m_long && ! m_double)
            inputHost().click().resetCombined();
        else {
            auto& combined              = inputHost().click().combined();
            combined.longClickEnabled   = m_long;
            combined.doubleClickEnabled = m_double;
            combined.longClickInterval  = m_interval;
            combined.longClicked        = [this] {
                Q_EMIT longClicked();
            };
            combined.doubleClicked = [this] {
                Q_EMIT doubleClicked();
            };
        }
        Q_EMIT combinedClickChanged();
    }
    bool m_long = false, m_double = false;
    int  m_interval = 0;
};
} // namespace qml_material
