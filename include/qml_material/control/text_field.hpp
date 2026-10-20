#pragma once

#include <QtQuick/private/qquicktextinput_p.h>
#include "qml_material/util/text_control_background.hpp"
#include "qml_material/util/control_environment.hpp"
#include "qml_material/input/input_host.hpp"

namespace qml_material
{

/** @ingroup control */
class QML_MATERIAL_API TextField : public QQuickTextInput, public ControlEnvironment {
    Q_OBJECT
    Q_PROPERTY(InteractionSource* interactionSource READ interactionSource WRITE
                   setInteractionSource NOTIFY interactionSourceChanged FINAL)
    Q_PROPERTY(qreal implicitWidth READ implicitWidth WRITE setImplicitWidth NOTIFY
                   fieldImplicitWidthChanged FINAL)
    Q_PROPERTY(qreal implicitHeight READ implicitHeight WRITE setImplicitHeight NOTIFY
                   fieldImplicitHeightChanged FINAL)
    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText NOTIFY
                   placeholderTextChanged FINAL)
    Q_PROPERTY(QColor placeholderTextColor READ placeholderTextColor WRITE setPlaceholderTextColor
                   NOTIFY placeholderTextColorChanged FINAL)
    Q_PROPERTY(Qt::FocusReason focusReason READ focusReason WRITE setFocusReason NOTIFY
                   focusReasonChanged FINAL)
    Q_PROPERTY(bool hoverEnabled READ hoverEnabled WRITE setHoverEnabled RESET resetHoverEnabled
                   NOTIFY hoverEnabledChanged FINAL)
    QML_NAMED_ELEMENT(TextFieldBase)
    Q_PROPERTY(QFont font READ font WRITE setFont RESET resetFont NOTIFY fontChanged FINAL)
    Q_PROPERTY(
        QQuickItem* background READ background WRITE setBackground NOTIFY backgroundChanged FINAL)
    Q_PROPERTY(qreal implicitBackgroundWidth READ implicitBackgroundWidth NOTIFY
                   implicitBackgroundWidthChanged FINAL)
    Q_PROPERTY(qreal implicitBackgroundHeight READ implicitBackgroundHeight NOTIFY
                   implicitBackgroundHeightChanged FINAL)
    Q_PROPERTY(qreal topInset READ topInset WRITE setTopInset RESET resetTopInset NOTIFY
                   topInsetChanged FINAL)
    Q_PROPERTY(qreal leftInset READ leftInset WRITE setLeftInset RESET resetLeftInset NOTIFY
                   leftInsetChanged FINAL)
    Q_PROPERTY(qreal rightInset READ rightInset WRITE setRightInset RESET resetRightInset NOTIFY
                   rightInsetChanged FINAL)
    Q_PROPERTY(qreal bottomInset READ bottomInset WRITE setBottomInset RESET resetBottomInset NOTIFY
                   bottomInsetChanged FINAL)

public:
    explicit TextField(QQuickItem* parent = nullptr);
    ~TextField() override;
    InteractionSource* interactionSource() { return m_input.source(); }
    void          setInteractionSource(InteractionSource* source) { m_input.setSource(source); }
    Q_SIGNAL void interactionSourceChanged();
    QFont         font() const;
    void          setFont(const QFont&);
    void          resetFont();
    QFont         effectiveFont() const override { return m_effective_font; }
    void          inheritFont(const QFont&) override;
    void          inheritHoverEnabled(bool) override;
    QQuickItem*   background() const { return m_background.item(); }
    void          setBackground(QQuickItem*);
    qreal         implicitBackgroundWidth() const;
    qreal         implicitBackgroundHeight() const;
    qreal         topInset() const { return m_background.inset(Qt::TopEdge); }
    void          setTopInset(qreal);
    void          resetTopInset();
    Q_SIGNAL void topInsetChanged();
    qreal         leftInset() const { return m_background.inset(Qt::LeftEdge); }
    void          setLeftInset(qreal);
    void          resetLeftInset();
    Q_SIGNAL void leftInsetChanged();
    qreal         rightInset() const { return m_background.inset(Qt::RightEdge); }
    void          setRightInset(qreal);
    void          resetRightInset();
    Q_SIGNAL void rightInsetChanged();
    qreal         bottomInset() const { return m_background.inset(Qt::BottomEdge); }
    void          setBottomInset(qreal);
    void          resetBottomInset();
    Q_SIGNAL void bottomInsetChanged();

    Q_SIGNAL void fontChanged();
    Q_SIGNAL void backgroundChanged();
    Q_SIGNAL void implicitBackgroundWidthChanged();
    Q_SIGNAL void implicitBackgroundHeightChanged();

    QString             placeholderText() const { return m_placeholder; }
    void                setPlaceholderText(const QString&);
    QColor              placeholderTextColor() const { return m_placeholder_color; }
    void                setPlaceholderTextColor(const QColor&);
    Qt::FocusReason     focusReason() const { return m_focus_reason; }
    void                setFocusReason(Qt::FocusReason);
    bool                hoverEnabled() const { return acceptHoverEvents(); }
    void                setHoverEnabled(bool);
    void                resetHoverEnabled();
    std::optional<bool> effectiveHoverEnabled() const override { return hoverEnabled(); }
    Q_SIGNAL void       fieldImplicitWidthChanged();
    Q_SIGNAL void       fieldImplicitHeightChanged();
    Q_SIGNAL void       placeholderTextChanged();
    Q_SIGNAL void       placeholderTextColorChanged();
    Q_SIGNAL void       focusReasonChanged();
    Q_SIGNAL void       hoverEnabledChanged();

protected:
    InputHost&   inputHost() { return m_input; }
    virtual bool usesEditorInput() const { return true; }
    bool         event(QEvent*) override;
    QSGNode*     updatePaintNode(QSGNode*, UpdatePaintNodeData*) override;
    void         focusInEvent(QFocusEvent*) override;
    void         focusOutEvent(QFocusEvent*) override;
    void         hoverEnterEvent(QHoverEvent*) override;
    void         hoverLeaveEvent(QHoverEvent*) override;
    void         mouseUngrabEvent() override;
    void         componentComplete() override;
    void         itemChange(ItemChange, const ItemChangeData&) override;
    void         geometryChange(const QRectF&, const QRectF&) override;

private:
    void                  resolveHoverEnabled();
    void                  updateHoverEnabled(bool);
    QString               m_placeholder;
    QColor                m_placeholder_color;
    Qt::FocusReason       m_focus_reason = Qt::OtherFocusReason;
    std::optional<bool>   m_requested_hover;
    void                  resolveFont();
    QFont                 m_requested_font;
    QFont                 m_effective_font;
    TextControlBackground m_background;
    InputHost             m_input;
};

} // namespace qml_material
