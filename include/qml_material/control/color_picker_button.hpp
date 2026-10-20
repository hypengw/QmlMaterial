#pragma once

#include "qml_material/control/command_button.hpp"
#include "qml_material/control/dialog.hpp"

namespace qml_material
{
class QML_MATERIAL_API ColorPickerButton : public CommandButton {
    Q_OBJECT
    QML_NAMED_ELEMENT(ColorPickerButtonBase)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(bool showAlpha READ showAlpha WRITE setShowAlpha NOTIFY showAlphaChanged FINAL)
    Q_PROPERTY(Dialog* dialog READ dialog NOTIFY dialogChanged FINAL)
public:
    explicit ColorPickerButton(QQuickItem* parent = nullptr);
    ~ColorPickerButton() override;
    QColor           color() const { return m_color; }
    void             setColor(const QColor&);
    bool             showAlpha() const { return m_showAlpha; }
    void             setShowAlpha(bool);
    Dialog*          dialog() const { return m_dialog; }
    Q_INVOKABLE void dismissPopup();
    Q_SIGNAL void    colorChanged();
    Q_SIGNAL void    showAlphaChanged();
    Q_SIGNAL void    dialogChanged();
    Q_SIGNAL void    accepted(const QColor& color);

protected:
    void activate() override;

private:
    void                           releaseDialog();
    void                           acceptColor();
    QColor                         m_color { QStringLiteral("#6750A4") };
    QPointer<Dialog>               m_dialog;
    QList<QPointer<Dialog>>        m_retiredDialogs;
    QList<QMetaObject::Connection> m_dialogConnections;
    QMetaObject::Connection        m_windowConnection;
    QMetaObject::Connection        m_visibilityConnection;
    quint64                        m_revision  = 0;
    bool                           m_showAlpha = true;
    bool                           m_creating  = false;
    bool                           m_committed = false;
};
} // namespace qml_material
