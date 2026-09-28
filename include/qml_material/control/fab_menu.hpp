#pragma once

#include "qml_material/control/button.hpp"
#include "qml_material/control/container.hpp"

namespace qml_material
{
class Flickable;

class QML_MATERIAL_API FABMenuItem : public Button {
    Q_OBJECT
    QML_NAMED_ELEMENT(FABMenuItemBase)
    Q_PROPERTY(bool revealed READ revealed NOTIFY presentationChanged FINAL)
    Q_PROPERTY(bool motionEnabled READ motionEnabled NOTIFY presentationChanged FINAL)
    Q_PROPERTY(bool alignRight READ alignRight NOTIFY presentationChanged FINAL)
    Q_PROPERTY(qreal fullWidth READ fullWidth NOTIFY fullWidthChanged FINAL)
    Q_PROPERTY(
        qreal widthProgress READ widthProgress WRITE setWidthProgress NOTIFY progressChanged FINAL)
    Q_PROPERTY(
        qreal alphaProgress READ alphaProgress WRITE setAlphaProgress NOTIFY progressChanged FINAL)
    Q_PROPERTY(bool animating READ animating WRITE setAnimating NOTIFY animatingChanged FINAL)
    Q_PROPERTY(bool inputEnabled READ inputEnabled NOTIFY inputEnabledChanged FINAL)
public:
    explicit FABMenuItem(QQuickItem* parent = nullptr): Button(parent) {}
    bool          revealed() const { return m_revealed; }
    bool          motionEnabled() const { return m_motionEnabled; }
    bool          alignRight() const { return m_alignRight; }
    qreal         fullWidth() const { return m_fullWidth; }
    qreal         widthProgress() const { return m_widthProgress; }
    qreal         alphaProgress() const { return m_alphaProgress; }
    bool          animating() const { return m_animating; }
    bool          inputEnabled() const { return m_inputEnabled; }
    bool          contains(const QPointF&) const override;
    void          setWidthProgress(qreal);
    void          setAlphaProgress(qreal);
    void          setAnimating(bool);
    Q_SIGNAL void presentationChanged();
    Q_SIGNAL void fullWidthChanged();
    Q_SIGNAL void progressChanged();
    Q_SIGNAL void animatingChanged();
    Q_SIGNAL void inputEnabledChanged();

protected:
    void focusInEvent(QFocusEvent*) override;

private:
    friend class FABMenu;
    void  setInputEnabled(bool);
    bool  m_revealed      = true;
    bool  m_motionEnabled = false;
    bool  m_alignRight    = true;
    bool  m_animating     = false;
    bool  m_inputEnabled  = true;
    qreal m_fullWidth     = 0;
    qreal m_widthProgress = 1;
    qreal m_alphaProgress = 1;
};

/** @ingroup control
 * End-aligned FAB actions with a fixed trigger and a scrolling item viewport.
 */
class QML_MATERIAL_API FABMenu : public Container {
    Q_OBJECT
    QML_NAMED_ELEMENT(FABMenuBase)
    Q_PROPERTY(bool expanded READ expanded WRITE setExpanded NOTIFY expandedChanged FINAL)
    Q_PROPERTY(QQuickItem* button READ button WRITE setButton NOTIFY buttonChanged FINAL)
    Q_PROPERTY(Qt::Alignment horizontalAlignment READ horizontalAlignment WRITE
                   setHorizontalAlignment NOTIFY horizontalAlignmentChanged FINAL)
    Q_PROPERTY(qreal buttonSpacing READ buttonSpacing WRITE setButtonSpacing NOTIFY
                   buttonSpacingChanged FINAL)
    Q_PROPERTY(int visibleCount READ visibleCount NOTIFY visibleCountChanged FINAL)
    Q_PROPERTY(
        qreal revealCount READ revealCount WRITE setRevealCount NOTIFY revealCountChanged FINAL)
    Q_PROPERTY(bool motionEnabled READ motionEnabled NOTIFY motionEnabledChanged FINAL)
    Q_PROPERTY(bool sequencing READ sequencing WRITE setSequencing NOTIFY sequencingChanged FINAL)
    Q_PROPERTY(bool animationsEnabled READ animationsEnabled WRITE setAnimationsEnabled NOTIFY
                   animationsEnabledChanged FINAL)
    Q_PROPERTY(bool transitioning READ transitioning NOTIFY transitioningChanged FINAL)
    Q_PROPERTY(QQuickItem* viewport READ viewport CONSTANT FINAL)
public:
    explicit FABMenu(QQuickItem* parent = nullptr);
    ~FABMenu() override;
    bool             expanded() const { return m_expanded; }
    void             setExpanded(bool);
    QQuickItem*      button() const { return m_button; }
    void             setButton(QQuickItem*);
    Qt::Alignment    horizontalAlignment() const { return m_alignment; }
    void             setHorizontalAlignment(Qt::Alignment);
    qreal            buttonSpacing() const { return m_buttonSpacing; }
    void             setButtonSpacing(qreal);
    int              visibleCount() const { return m_visibleCount; }
    qreal            revealCount() const { return m_revealCount; }
    void             setRevealCount(qreal);
    bool             motionEnabled() const { return m_ready; }
    bool             sequencing() const { return m_sequencing; }
    bool             animationsEnabled() const { return m_animationsEnabled; }
    void             setAnimationsEnabled(bool);
    void             setSequencing(bool);
    bool             transitioning() const { return m_transitioning; }
    QQuickItem*      viewport() const;
    Q_INVOKABLE void open() { setExpanded(true); }
    Q_INVOKABLE void close() { setExpanded(false); }
    Q_INVOKABLE void toggle() { setExpanded(! m_expanded); }
    Q_SIGNAL void    expandedChanged();
    Q_SIGNAL void    buttonChanged();
    Q_SIGNAL void    horizontalAlignmentChanged();
    Q_SIGNAL void    buttonSpacingChanged();
    Q_SIGNAL void    visibleCountChanged();
    Q_SIGNAL void    revealCountChanged();
    Q_SIGNAL void    motionEnabledChanged();
    Q_SIGNAL void    sequencingChanged();
    Q_SIGNAL void    animationsEnabledChanged();
    Q_SIGNAL void    transitioningChanged();

protected:
    bool isContent(QQuickItem*) const override;
    void itemAdded(QQuickItem*) override;
    void itemRemoved(QQuickItem*) override;
    void updatePolish() override;
    bool eventFilter(QObject*, QEvent*) override;

private:
    void                                               updateTransitioning();
    void                                               updateInput();
    QList<FABMenuItem*>                                visibleItems() const;
    Flickable*                                         m_viewport;
    QPointer<QQuickItem>                               m_button;
    QList<QMetaObject::Connection>                     m_buttonConnections;
    QHash<QQuickItem*, QList<QMetaObject::Connection>> m_itemConnections;
    Qt::Alignment                                      m_alignment         = Qt::AlignRight;
    qreal                                              m_buttonSpacing     = 8;
    qreal                                              m_revealCount       = 0;
    int                                                m_visibleCount      = 0;
    bool                                               m_expanded          = false;
    bool                                               m_ready             = false;
    bool                                               m_sequencing        = false;
    bool                                               m_animationsEnabled = true;
    bool                                               m_transitioning     = false;
};
} // namespace qml_material
