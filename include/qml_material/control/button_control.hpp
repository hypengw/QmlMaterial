#pragma once

#include "qml_material/control/button_presentation.hpp"
#include "qml_material/input/input_host.hpp"

namespace qml_material
{
class QML_MATERIAL_API ButtonControl : public ButtonPresentation {
    friend class NavigationRail;
    friend class MenuEntryController;
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(InteractionSource* interactionSource READ interactionSource WRITE
                   setInteractionSource NOTIFY interactionSourceChanged FINAL)
    Q_PROPERTY(bool focusVisible READ focusVisible NOTIFY focusVisibleChanged FINAL)
public:
    explicit ButtonControl(QQuickItem* parent = nullptr);
    InteractionSource* interactionSource() { return m_input.source(); }
    void             setInteractionSource(InteractionSource* source) { m_input.setSource(source); }
    bool             focusVisible() const { return m_input.focusVisible(); }
    Q_INVOKABLE void click();
    void             cancelActivation() { m_input.cancelActivation(); }
    Q_SIGNAL void    interactionSourceChanged();
    Q_SIGNAL void    focusVisibleChanged();

protected:
    virtual bool canActivate() const;
    virtual void activate() = 0;
    virtual void inputSourceChange() {}
    InputHost&   inputHost() { return m_input; }
    void         cancelInput() { m_input.cancel(); }
    bool         event(QEvent*) override;
    void         mousePressEvent(QMouseEvent*) override;
    void         mouseMoveEvent(QMouseEvent*) override;
    void         mouseReleaseEvent(QMouseEvent*) override;
    void         mouseDoubleClickEvent(QMouseEvent*) override;
    void         touchEvent(QTouchEvent*) override;
    void         keyPressEvent(QKeyEvent*) override;
    void         keyReleaseEvent(QKeyEvent*) override;
    void         mouseUngrabEvent() override;
    void         touchUngrabEvent() override;
    void         focusInEvent(QFocusEvent*) override;
    void         focusOutEvent(QFocusEvent*) override;
    void         hoverEnterEvent(QHoverEvent*) override;
    void         hoverMoveEvent(QHoverEvent*) override;
    void         hoverLeaveEvent(QHoverEvent*) override;

private:
    InputHost m_input;
};
} // namespace qml_material
