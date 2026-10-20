#pragma once

#include "qml_material/control/control.hpp"
#include "qml_material/control/icon_spec.hpp"

namespace qml_material
{
class QML_MATERIAL_API ButtonPresentation : public Control {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QString text READ text WRITE setText RESET resetText NOTIFY textChanged FINAL)
    Q_PROPERTY(IconSpec* icon READ icon CONSTANT FINAL)
    Q_PROPERTY(
        QQuickItem* indicator READ indicator WRITE setIndicator NOTIFY indicatorChanged FINAL)
    Q_PROPERTY(qreal implicitIndicatorWidth READ implicitIndicatorWidth NOTIFY
                   implicitIndicatorWidthChanged FINAL)
    Q_PROPERTY(qreal implicitIndicatorHeight READ implicitIndicatorHeight NOTIFY
                   implicitIndicatorHeightChanged FINAL)
    Q_PROPERTY(Display display READ display WRITE setDisplay NOTIFY displayChanged FINAL)
public:
    enum Display
    {
        IconOnly,
        TextOnly,
        TextBesideIcon,
        TextUnderIcon
    };
    Q_ENUM(Display)
    explicit ButtonPresentation(QQuickItem* parent = nullptr);
    ~ButtonPresentation() override;
    QString     text() const;
    void        setText(const QString&);
    void        resetText();
    IconSpec*   icon() const { return m_icon; }
    QQuickItem* indicator() const { return m_indicator; }
    void        setIndicator(QQuickItem*);
    qreal implicitIndicatorWidth() const { return m_indicator ? m_indicator->implicitWidth() : 0; }
    qreal implicitIndicatorHeight() const {
        return m_indicator ? m_indicator->implicitHeight() : 0;
    }
    Display       display() const { return m_display; }
    void          setDisplay(Display);
    Q_SIGNAL void textChanged();
    Q_SIGNAL void indicatorChanged();
    Q_SIGNAL void implicitIndicatorWidthChanged();
    Q_SIGNAL void implicitIndicatorHeightChanged();
    Q_SIGNAL void displayChanged();

protected:
    virtual QString defaultText() const { return {}; }
    void            defaultTextChanged();
    void            disconnectPresentation();

private:
    IconSpec*                      m_icon;
    QPointer<QQuickItem>           m_indicator;
    QList<QMetaObject::Connection> m_indicatorConnections;
    std::optional<QString>         m_text;
    Display                        m_display           = TextBesideIcon;
    quint64                        m_indicatorRevision = 0;
};
} // namespace qml_material
