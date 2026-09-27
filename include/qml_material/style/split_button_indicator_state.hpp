#pragma once
#include "qml_material/style/common_state.hpp"
#include "qml_material/token/split_button.hpp"
#include <optional>
Q_MOC_INCLUDE("qml_material/control/button.hpp")
namespace qml_material
{
class Button;
class QML_MATERIAL_API SplitButtonIndicatorState : public CommonState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateSplitButtonIndicator)
    Q_PROPERTY(qml_material::Button* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(int type READ type WRITE setType NOTIFY typeChanged BINDABLE bindableType)
    Q_PROPERTY(int size READ size WRITE setSize NOTIFY sizeChanged BINDABLE bindableSize)
    Q_PROPERTY(
        bool isRound READ isRound WRITE setIsRound NOTIFY isRoundChanged BINDABLE bindableIsRound)
    Q_PROPERTY(qreal baseCorner READ baseCorner NOTIFY baseCornerChanged)
    Q_PROPERTY(qreal innerCorner READ innerCorner NOTIFY innerCornerChanged)
    Q_PROPERTY(qml_material::token::SplitButtonSizeItem sizeToken READ sizeToken NOTIFY sizeChanged)
public:
    explicit SplitButtonIndicatorState(QObject* parent = nullptr);
    ~SplitButtonIndicatorState() override;
    Button*                    item() const;
    void                       setItem(Button*);
    Q_SIGNAL void              itemChanged();
    int                        type() const;
    void                       setType(int);
    QBindable<int>             bindableType();
    Q_SIGNAL void              typeChanged();
    int                        size() const;
    void                       setSize(int);
    QBindable<int>             bindableSize();
    Q_SIGNAL void              sizeChanged();
    bool                       isRound() const;
    void                       setIsRound(bool);
    QBindable<bool>            bindableIsRound();
    Q_SIGNAL void              isRoundChanged();
    qreal                      baseCorner() const;
    qreal                      innerCorner() const { return m_innerCorner.value(); }
    token::SplitButtonSizeItem sizeToken() const;
    Q_SIGNAL void              innerCornerChanged();
    Q_SIGNAL void              baseCornerChanged();
    Q_INVOKABLE qreal          calcRadius(int size, bool round, bool pressed) const;

private:
    enum class Interaction
    {
        Base,
        Disabled
    };
    struct Selection {
        Interaction state                              = Interaction::Base;
        quint64     generation                         = 0;
        bool        operator==(const Selection&) const = default;
    };
    void                            selectionChanged();
    void                            updateBackground();
    StateBindingSet<Interaction>    m_bindings { Interaction::Base };
    QProperty<Button*>              m_item { nullptr };
    QProperty<bool>                 m_disabled { false };
    QList<QMetaObject::Connection>  m_connections;
    QList<QMetaObject::Connection>  m_backgroundConnections;
    QProperty<std::optional<qreal>> m_backgroundHeight;
    bool                            m_ready = false;
    Q_OBJECT_BINDABLE_PROPERTY(SplitButtonIndicatorState, Selection, m_selection,
                               &SplitButtonIndicatorState::selectionChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SplitButtonIndicatorState, int, m_type,
                                         int(Enum::ButtonType::BtFilled),
                                         &SplitButtonIndicatorState::typeChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SplitButtonIndicatorState, int, m_size,
                                         int(Enum::ButtonSize::S),
                                         &SplitButtonIndicatorState::sizeChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SplitButtonIndicatorState, bool, m_isRound, true,
                                         &SplitButtonIndicatorState::isRoundChanged)
    Q_OBJECT_BINDABLE_PROPERTY(SplitButtonIndicatorState, qreal, m_baseCorner,
                               &SplitButtonIndicatorState::baseCornerChanged)
    Q_OBJECT_BINDABLE_PROPERTY(SplitButtonIndicatorState, qreal, m_innerCorner,
                               &SplitButtonIndicatorState::innerCornerChanged)
    StateBindingSet<Interaction>::Lifetime m_lifetime { m_bindings.lifetime() };
};
} // namespace qml_material
