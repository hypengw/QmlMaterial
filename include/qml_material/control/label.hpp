#pragma once

#include <QtQuick/private/qquicktext_p.h>
#include "qml_material/util/text_control_background.hpp"
#include "qml_material/util/control_environment.hpp"

namespace qml_material
{

/** @ingroup control */
class QML_MATERIAL_API Label : public QQuickText, public ControlEnvironment {
    Q_OBJECT
    QML_NAMED_ELEMENT(LabelBase)
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
    explicit Label(QQuickItem* parent = nullptr);
    ~Label() override;
    QFont         font() const;
    void          setFont(const QFont&);
    void          resetFont();
    QFont         effectiveFont() const override { return m_effective_font; }
    void          refreshInheritedEnvironment() override;
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

protected:
    void componentComplete() override;
    void itemChange(ItemChange, const ItemChangeData&) override;
    void geometryChange(const QRectF&, const QRectF&) override;

private:
    void                  updateFont();
    QFont                 m_requested_font;
    QFont                 m_effective_font;
    TextControlBackground m_background;
};

} // namespace qml_material
