#pragma once

#include "qml_material/control/popup.hpp"

namespace qml_material
{
class QML_MATERIAL_API ToolTipPopup : public Popup {
    Q_OBJECT
    QML_NAMED_ELEMENT(ToolTipPopupBase)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged FINAL)
    Q_PROPERTY(int delay READ delay WRITE setDelay NOTIFY delayChanged FINAL)
    Q_PROPERTY(int timeout READ timeout WRITE setTimeout NOTIFY timeoutChanged FINAL)
public:
    explicit ToolTipPopup(QObject* parent = nullptr);
    QString          text() const { return m_text; }
    void             setText(const QString&);
    int              delay() const { return m_delay; }
    void             setDelay(int);
    int              timeout() const { return m_timeout; }
    void             setTimeout(int);
    void             open() override;
    void             close() override;
    void             dismissImmediately() override;
    Q_INVOKABLE void show(const QString& text, int timeout = -1);
    Q_INVOKABLE void hide();
    Q_SIGNAL void    textChanged();
    Q_SIGNAL void    delayChanged();
    Q_SIGNAL void    timeoutChanged();

protected:
    bool inheritsHoverEnabled() const override { return false; }

private:
    void                    startTimeout();
    QString                 m_text;
    int                     m_delay   = 0;
    int                     m_timeout = -1;
    QTimer                  m_delayTimer, m_timeoutTimer;
    QMetaObject::Connection m_windowVisibility;
};
} // namespace qml_material
