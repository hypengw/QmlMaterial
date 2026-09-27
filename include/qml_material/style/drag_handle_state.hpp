#pragma once
#include "qml_material/style/common_state.hpp"
#include <QQuickItem>
namespace qml_material
{
class QML_MATERIAL_API DragHandleState : public CommonState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateDragHandle)
    Q_PROPERTY(QQuickItem* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(
        bool pressed READ pressed WRITE setPressed NOTIFY pressedChanged BINDABLE bindablePressed)
    Q_PROPERTY(
        bool hovered READ hovered WRITE setHovered NOTIFY hoveredChanged BINDABLE bindableHovered)
    Q_PROPERTY(bool visualFocus READ visualFocus WRITE setVisualFocus NOTIFY visualFocusChanged
                   BINDABLE bindableVisualFocus)
    Q_PROPERTY(int handlePressedWidth READ handlePressedWidth WRITE setHandlePressedWidth NOTIFY
                   handlePressedWidthChanged BINDABLE bindableHandlePressedWidth)
    Q_PROPERTY(int handlePressedHeight READ handlePressedHeight WRITE setHandlePressedHeight NOTIFY
                   handlePressedHeightChanged BINDABLE bindableHandlePressedHeight)
    Q_PROPERTY(int radius READ radius WRITE setRadius RESET resetRadius NOTIFY radiusChanged
                   BINDABLE bindableRadius)
    Q_PROPERTY(int handleWidth READ handleWidth WRITE setHandleWidth RESET resetHandleWidth NOTIFY
                   handleWidthChanged BINDABLE bindableHandleWidth)
    Q_PROPERTY(int handleHeight READ handleHeight WRITE setHandleHeight RESET resetHandleHeight
                   NOTIFY handleHeightChanged BINDABLE bindableHandleHeight)
public:
    explicit DragHandleState(QObject* parent = nullptr);
    ~DragHandleState() override;
    QQuickItem*     item() const;
    void            setItem(QQuickItem*);
    Q_SIGNAL void   itemChanged();
    bool            pressed() const;
    void            setPressed(bool);
    QBindable<bool> bindablePressed();
    Q_SIGNAL void   pressedChanged();
    bool            hovered() const;
    void            setHovered(bool);
    QBindable<bool> bindableHovered();
    Q_SIGNAL void   hoveredChanged();
    bool            visualFocus() const;
    void            setVisualFocus(bool);
    QBindable<bool> bindableVisualFocus();
    Q_SIGNAL void   visualFocusChanged();
    int             handlePressedWidth() const;
    void            setHandlePressedWidth(int);
    QBindable<int>  bindableHandlePressedWidth();
    Q_SIGNAL void   handlePressedWidthChanged();
    int             handlePressedHeight() const;
    void            setHandlePressedHeight(int);
    QBindable<int>  bindableHandlePressedHeight();
    Q_SIGNAL void   handlePressedHeightChanged();
    int             radius() const;
    void            setRadius(int);
    QBindable<int>  bindableRadius();
    Q_SIGNAL void   radiusChanged();
    void            resetRadius();
    int             handleWidth() const;
    void            setHandleWidth(int);
    QBindable<int>  bindableHandleWidth();
    Q_SIGNAL void   handleWidthChanged();
    void            resetHandleWidth();
    int             handleHeight() const;
    void            setHandleHeight(int);
    QBindable<int>  bindableHandleHeight();
    Q_SIGNAL void   handleHeightChanged();
    void            resetHandleHeight();

private:
    enum class Interaction
    {
        Base,
        Disabled,
        Pressed,
        Hovered,
        Focus
    };
    struct Selection {
        Interaction state                              = Interaction::Base;
        quint64     generation                         = 0;
        bool        operator==(const Selection&) const = default;
    };
    void                           selectionChanged();
    StateBindingSet<Interaction>   m_bindings { Interaction::Base };
    QProperty<QQuickItem*>         m_item { nullptr };
    QProperty<bool>                m_disabled { false };
    QList<QMetaObject::Connection> m_connections;
    bool                           m_ready = false;
    Q_OBJECT_BINDABLE_PROPERTY(DragHandleState, Selection, m_selection,
                               &DragHandleState::selectionChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(DragHandleState, bool, m_pressed, false,
                                         &DragHandleState::pressedChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(DragHandleState, bool, m_hovered, false,
                                         &DragHandleState::hoveredChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(DragHandleState, bool, m_visualFocus, false,
                                         &DragHandleState::visualFocusChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(DragHandleState, int, m_handlePressedWidth, 12,
                                         &DragHandleState::handlePressedWidthChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(DragHandleState, int, m_handlePressedHeight, 52,
                                         &DragHandleState::handlePressedHeightChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(DragHandleState, int, m_radius, 100,
                                         &DragHandleState::radiusChanged)
    PropertyKey<int> m_radiusKey;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(DragHandleState, int, m_handleWidth, 4,
                                         &DragHandleState::handleWidthChanged)
    PropertyKey<int> m_handleWidthKey;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(DragHandleState, int, m_handleHeight, 48,
                                         &DragHandleState::handleHeightChanged)
    PropertyKey<int>                       m_handleHeightKey;
    StateBindingSet<Interaction>::Lifetime m_lifetime { m_bindings.lifetime() };
};
} // namespace qml_material
