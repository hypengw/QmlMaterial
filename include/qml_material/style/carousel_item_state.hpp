#pragma once
#include "qml_material/style/common_state.hpp"
#include <QQuickItem>
namespace qml_material
{
class QML_MATERIAL_API CarouselItemState : public CommonState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateCarouselItem)
    Q_PROPERTY(QQuickItem* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(bool down READ down WRITE setDown NOTIFY downChanged BINDABLE bindableDown)
    Q_PROPERTY(
        bool hovered READ hovered WRITE setHovered NOTIFY hoveredChanged BINDABLE bindableHovered)
public:
    explicit CarouselItemState(QObject* parent = nullptr);
    ~CarouselItemState() override;
    QQuickItem*     item() const;
    void            setItem(QQuickItem*);
    Q_SIGNAL void   itemChanged();
    bool            down() const;
    void            setDown(bool);
    QBindable<bool> bindableDown();
    Q_SIGNAL void   downChanged();
    bool            hovered() const;
    void            setHovered(bool);
    QBindable<bool> bindableHovered();
    Q_SIGNAL void   hoveredChanged();

private:
    enum class Interaction
    {
        Base,
        Disabled,
        Pressed,
        Hovered
    };
    struct Selection {
        Interaction state                              = Interaction::Base;
        quint64     generation                         = 0;
        bool        operator==(const Selection&) const = default;
    };
    void                         selectionChanged();
    StateBindingSet<Interaction> m_bindings { Interaction::Base };
    QProperty<bool>              m_disabled { false };
    QMetaObject::Connection      m_enabledConnection;
    bool                         m_ready = false;
    Q_OBJECT_BINDABLE_PROPERTY(CarouselItemState, Selection, m_selection,
                               &CarouselItemState::selectionChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(CarouselItemState, bool, m_down, false,
                                         &CarouselItemState::downChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(CarouselItemState, bool, m_hovered, false,
                                         &CarouselItemState::hoveredChanged)
    StateBindingSet<Interaction>::Lifetime m_lifetime { m_bindings.lifetime() };
};
} // namespace qml_material
