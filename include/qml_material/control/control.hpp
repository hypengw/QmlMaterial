#pragma once

#include <QFont>
#include <QLocale>
#include <QPointer>
#include <QQuickItem>
#include <qqmlregistration.h>

#include <optional>

#include "qml_material/export.hpp"
#include "qml_material/util/control_environment.hpp"

class QFocusEvent;
class QHoverEvent;

namespace qml_material
{

class QML_MATERIAL_API Control : public QQuickItem, public ControlEnvironment {
    Q_OBJECT
    QML_NAMED_ELEMENT(ControlBase)

    Q_PROPERTY(QFont font READ font WRITE setFont RESET resetFont NOTIFY fontChanged FINAL)
    Q_PROPERTY(
        QLocale locale READ locale WRITE setLocale RESET resetLocale NOTIFY localeChanged FINAL)
    Q_PROPERTY(Qt::LayoutDirection layoutDirection READ layoutDirection WRITE setLayoutDirection
                   RESET resetLayoutDirection NOTIFY layoutDirectionChanged FINAL)
    Q_PROPERTY(bool mirrored READ mirrored NOTIFY mirroredChanged FINAL)

    Q_PROPERTY(qreal availableWidth READ availableWidth NOTIFY availableWidthChanged FINAL)
    Q_PROPERTY(qreal availableHeight READ availableHeight NOTIFY availableHeightChanged FINAL)
    Q_PROPERTY(
        qreal padding READ padding WRITE setPadding RESET resetPadding NOTIFY paddingChanged FINAL)
    Q_PROPERTY(qreal horizontalPadding READ horizontalPadding WRITE setHorizontalPadding RESET
                   resetHorizontalPadding NOTIFY horizontalPaddingChanged FINAL)
    Q_PROPERTY(qreal verticalPadding READ verticalPadding WRITE setVerticalPadding RESET
                   resetVerticalPadding NOTIFY verticalPaddingChanged FINAL)
    Q_PROPERTY(qreal topPadding READ topPadding WRITE setTopPadding RESET resetTopPadding NOTIFY
                   topPaddingChanged FINAL)
    Q_PROPERTY(qreal leftPadding READ leftPadding WRITE setLeftPadding RESET resetLeftPadding NOTIFY
                   leftPaddingChanged FINAL)
    Q_PROPERTY(qreal rightPadding READ rightPadding WRITE setRightPadding RESET resetRightPadding
                   NOTIFY rightPaddingChanged FINAL)
    Q_PROPERTY(qreal bottomPadding READ bottomPadding WRITE setBottomPadding RESET
                   resetBottomPadding NOTIFY bottomPaddingChanged FINAL)
    Q_PROPERTY(
        qreal spacing READ spacing WRITE setSpacing RESET resetSpacing NOTIFY spacingChanged FINAL)

    Q_PROPERTY(qreal topInset READ topInset WRITE setTopInset RESET resetTopInset NOTIFY
                   topInsetChanged FINAL)
    Q_PROPERTY(qreal leftInset READ leftInset WRITE setLeftInset RESET resetLeftInset NOTIFY
                   leftInsetChanged FINAL)
    Q_PROPERTY(qreal rightInset READ rightInset WRITE setRightInset RESET resetRightInset NOTIFY
                   rightInsetChanged FINAL)
    Q_PROPERTY(qreal bottomInset READ bottomInset WRITE setBottomInset RESET resetBottomInset NOTIFY
                   bottomInsetChanged FINAL)

    Q_PROPERTY(
        QQuickItem* background READ background WRITE setBackground NOTIFY backgroundChanged FINAL)
    Q_PROPERTY(QQuickItem* contentItem READ contentItem WRITE setContentItem NOTIFY
                   contentItemChanged FINAL)
    Q_PROPERTY(LayoutFlags backgroundLayout READ backgroundLayout WRITE setBackgroundLayout RESET
                   resetBackgroundLayout NOTIFY backgroundLayoutChanged FINAL)
    Q_PROPERTY(LayoutFlags contentItemLayout READ contentItemLayout WRITE setContentItemLayout RESET
                   resetContentItemLayout NOTIFY contentItemLayoutChanged FINAL)

    Q_PROPERTY(qreal implicitBackgroundWidth READ implicitBackgroundWidth NOTIFY
                   implicitBackgroundWidthChanged FINAL)
    Q_PROPERTY(qreal implicitBackgroundHeight READ implicitBackgroundHeight NOTIFY
                   implicitBackgroundHeightChanged FINAL)
    Q_PROPERTY(qreal implicitContentWidth READ implicitContentWidth NOTIFY
                   implicitContentWidthChanged FINAL)
    Q_PROPERTY(qreal implicitContentHeight READ implicitContentHeight NOTIFY
                   implicitContentHeightChanged FINAL)
    Q_PROPERTY(
        qreal implicitLayoutWidth READ implicitLayoutWidth NOTIFY implicitLayoutWidthChanged FINAL)
    Q_PROPERTY(qreal implicitLayoutHeight READ implicitLayoutHeight NOTIFY
                   implicitLayoutHeightChanged FINAL)
    Q_PROPERTY(qreal extraImplicitWidth READ extraImplicitWidth WRITE setExtraImplicitWidth RESET
                   resetExtraImplicitWidth NOTIFY extraImplicitWidthChanged FINAL)
    Q_PROPERTY(qreal extraImplicitHeight READ extraImplicitHeight WRITE setExtraImplicitHeight RESET
                   resetExtraImplicitHeight NOTIFY extraImplicitHeightChanged FINAL)

    Q_PROPERTY(qreal baselineOffset READ baselineOffset WRITE setBaselineOffset RESET
                   resetBaselineOffset NOTIFY baselineOffsetChanged FINAL)
    Q_PROPERTY(bool hoverEnabled READ hoverEnabled WRITE setHoverEnabled RESET resetHoverEnabled
                   NOTIFY hoverEnabledChanged FINAL)
    Q_PROPERTY(bool hovered READ hovered NOTIFY hoveredChanged FINAL)
    Q_PROPERTY(Qt::FocusReason focusReason READ focusReason WRITE setFocusReason NOTIFY
                   focusReasonChanged FINAL)
    Q_PROPERTY(bool visualFocus READ visualFocus NOTIFY visualFocusChanged FINAL)

public:
    enum LayoutFlag
    {
        LayoutNone     = 0x0,
        LayoutX        = 0x1,
        LayoutY        = 0x2,
        LayoutWidth    = 0x4,
        LayoutHeight   = 0x8,
        LayoutPosition = LayoutX | LayoutY,
        LayoutSize     = LayoutWidth | LayoutHeight,
        LayoutAll      = LayoutPosition | LayoutSize
    };
    Q_DECLARE_FLAGS(LayoutFlags, LayoutFlag)
    Q_FLAG(LayoutFlags)

    explicit Control(QQuickItem* parent = nullptr);
    ~Control() override;

    QFont font() const;
    QFont effectiveFont() const override { return m_font; }
    void  refreshInheritedEnvironment() override;
    void  setFont(const QFont& value);
    void  resetFont();

    QLocale locale() const;
    void    setLocale(const QLocale& value);
    void    resetLocale();

    Qt::LayoutDirection layoutDirection() const;
    void                setLayoutDirection(Qt::LayoutDirection value);
    void                resetLayoutDirection();
    bool                mirrored() const;

    qreal availableWidth() const;
    qreal availableHeight() const;

    qreal padding() const;
    void  setPadding(qreal value);
    void  resetPadding();

    qreal horizontalPadding() const;
    void  setHorizontalPadding(qreal value);
    void  resetHorizontalPadding();

    qreal verticalPadding() const;
    void  setVerticalPadding(qreal value);
    void  resetVerticalPadding();

    qreal topPadding() const;
    void  setTopPadding(qreal value);
    void  resetTopPadding();

    qreal leftPadding() const;
    void  setLeftPadding(qreal value);
    void  resetLeftPadding();

    qreal rightPadding() const;
    void  setRightPadding(qreal value);
    void  resetRightPadding();

    qreal bottomPadding() const;
    void  setBottomPadding(qreal value);
    void  resetBottomPadding();

    qreal spacing() const;
    void  setSpacing(qreal value);
    void  resetSpacing();

    qreal topInset() const;
    void  setTopInset(qreal value);
    void  resetTopInset();

    qreal leftInset() const;
    void  setLeftInset(qreal value);
    void  resetLeftInset();

    qreal rightInset() const;
    void  setRightInset(qreal value);
    void  resetRightInset();

    qreal bottomInset() const;
    void  setBottomInset(qreal value);
    void  resetBottomInset();

    QQuickItem* background() const;
    void        setBackground(QQuickItem* item);

    QQuickItem* contentItem() const;
    void        setContentItem(QQuickItem* item);

    LayoutFlags backgroundLayout() const;
    void        setBackgroundLayout(LayoutFlags value);
    void        resetBackgroundLayout();

    LayoutFlags contentItemLayout() const;
    void        setContentItemLayout(LayoutFlags value);
    void        resetContentItemLayout();

    qreal implicitBackgroundWidth() const;
    qreal implicitBackgroundHeight() const;
    qreal implicitContentWidth() const;
    qreal implicitContentHeight() const;
    qreal implicitLayoutWidth() const;
    qreal implicitLayoutHeight() const;

    qreal extraImplicitWidth() const;
    void  setExtraImplicitWidth(qreal value);
    void  resetExtraImplicitWidth();

    qreal extraImplicitHeight() const;
    void  setExtraImplicitHeight(qreal value);
    void  resetExtraImplicitHeight();

    qreal baselineOffset() const;
    void  setBaselineOffset(qreal value);
    void  resetBaselineOffset();

    bool hoverEnabled() const;
    std::optional<bool> effectiveHoverEnabled() const override { return hoverEnabled(); }
    void setHoverEnabled(bool value);
    void resetHoverEnabled();

    bool hovered() const;

    Qt::FocusReason focusReason() const;
    void            setFocusReason(Qt::FocusReason value);
    bool            visualFocus() const;

    Q_SIGNAL void fontChanged();
    Q_SIGNAL void localeChanged();
    Q_SIGNAL void layoutDirectionChanged();
    Q_SIGNAL void mirroredChanged();
    Q_SIGNAL void availableWidthChanged();
    Q_SIGNAL void availableHeightChanged();
    Q_SIGNAL void paddingChanged();
    Q_SIGNAL void horizontalPaddingChanged();
    Q_SIGNAL void verticalPaddingChanged();
    Q_SIGNAL void topPaddingChanged();
    Q_SIGNAL void leftPaddingChanged();
    Q_SIGNAL void rightPaddingChanged();
    Q_SIGNAL void bottomPaddingChanged();
    Q_SIGNAL void spacingChanged();
    Q_SIGNAL void topInsetChanged();
    Q_SIGNAL void leftInsetChanged();
    Q_SIGNAL void rightInsetChanged();
    Q_SIGNAL void bottomInsetChanged();
    Q_SIGNAL void backgroundChanged();
    Q_SIGNAL void contentItemChanged();
    Q_SIGNAL void backgroundLayoutChanged();
    Q_SIGNAL void contentItemLayoutChanged();
    Q_SIGNAL void implicitBackgroundWidthChanged();
    Q_SIGNAL void implicitBackgroundHeightChanged();
    Q_SIGNAL void implicitContentWidthChanged();
    Q_SIGNAL void implicitContentHeightChanged();
    Q_SIGNAL void implicitLayoutWidthChanged();
    Q_SIGNAL void implicitLayoutHeightChanged();
    Q_SIGNAL void extraImplicitWidthChanged();
    Q_SIGNAL void extraImplicitHeightChanged();
    Q_SIGNAL void baselineOffsetChanged();
    Q_SIGNAL void hoverEnabledChanged();
    Q_SIGNAL void hoveredChanged();
    Q_SIGNAL void focusReasonChanged();
    Q_SIGNAL void visualFocusChanged();

protected:
    virtual QRectF contentRect() const;
    void           layoutContentItem();
    virtual QSizeF measureImplicitContent() const;
    void           updateImplicitMetrics();
    void           componentComplete() override;
    void           geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
    void           itemChange(ItemChange change, const ItemChangeData& value) override;
    void           focusInEvent(QFocusEvent* event) override;
    void           focusOutEvent(QFocusEvent* event) override;
    void           hoverEnterEvent(QHoverEvent* event) override;
    void           hoverMoveEvent(QHoverEvent* event) override;
    void           hoverLeaveEvent(QHoverEvent* event) override;
    bool           eventFilter(QObject* watched, QEvent* event) override;

private:
    struct PaddingState {
        qreal top;
        qreal left;
        qreal right;
        qreal bottom;
        qreal horizontal;
        qreal vertical;
        qreal availableWidth;
        qreal availableHeight;
        qreal implicitWidth;
        qreal implicitHeight;
    };

    PaddingState paddingState() const;
    void         finishPaddingChange(const PaddingState& oldState);
    void         finishInsetChange(qreal oldImplicitWidth, qreal oldImplicitHeight);
    void         updateBaselineOffset();
    void         updateEnvironment(bool propagate = true);
    void         updateVisualFocus();
    void         setHovered(bool value);
    void         layoutItems();
    void         layoutBackground();
    void         orderManagedItems();
    void         disconnectBackground();
    void         disconnectContentItem();
    Control*     parentControl() const;
    void         updateDescendantControls();

    qreal                m_padding { 0 };
    std::optional<qreal> m_horizontal_padding;
    std::optional<qreal> m_vertical_padding;
    std::optional<qreal> m_top_padding;
    std::optional<qreal> m_left_padding;
    std::optional<qreal> m_right_padding;
    std::optional<qreal> m_bottom_padding;
    qreal                m_spacing { 0 };
    qreal                m_top_inset { 0 };
    qreal                m_left_inset { 0 };
    qreal                m_right_inset { 0 };
    qreal                m_bottom_inset { 0 };

    QPointer<QQuickItem>           m_background;
    QPointer<QQuickItem>           m_content_item;
    QList<QMetaObject::Connection> m_control_connections;
    QList<QMetaObject::Connection> m_background_connections;
    QList<QMetaObject::Connection> m_content_connections;
    LayoutFlags                    m_background_layout { LayoutAll };
    LayoutFlags                    m_content_item_layout { LayoutAll };

    qreal m_implicit_background_width { 0 };
    qreal m_implicit_background_height { 0 };
    qreal m_implicit_content_width { 0 };
    qreal m_implicit_content_height { 0 };
    qreal m_extra_implicit_width { 0 };
    qreal m_extra_implicit_height { 0 };

    QFont               m_requested_font;
    QFont               m_font;
    QLocale             m_requested_locale;
    QLocale             m_locale;
    Qt::LayoutDirection m_requested_layout_direction { Qt::LeftToRight };
    Qt::LayoutDirection m_layout_direction { Qt::LeftToRight };
    std::optional<bool> m_requested_hover_enabled;
    bool                m_font_explicit { false };
    bool                m_locale_explicit { false };
    bool                m_layout_direction_explicit { false };
    bool                m_hover_enabled { false };
    bool                m_hovered { false };
    bool                m_baseline_explicit { false };
    bool                m_visual_focus { false };
    Qt::FocusReason     m_focus_reason { Qt::OtherFocusReason };
};

} // namespace qml_material

Q_DECLARE_OPERATORS_FOR_FLAGS(qml_material::Control::LayoutFlags)
