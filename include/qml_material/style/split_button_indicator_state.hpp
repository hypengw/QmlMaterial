#pragma once
#include "qml_material/style/button_interaction_state.hpp"
#include "qml_material/token/split_button.hpp"
namespace qml_material
{
class QML_MATERIAL_API SplitButtonIndicatorState : public ButtonValueState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateSplitButtonIndicator)
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
    qreal                      innerCorner() const;
    token::SplitButtonSizeItem sizeToken() const;
    Q_SIGNAL void              innerCornerChanged();
    Q_SIGNAL void              baseCornerChanged();
    Q_INVOKABLE qreal          calcRadius(int size, bool round, bool pressed) const;

private:
    void                           observeGeometry();
    void                           notifyGeometry();
    QList<QMetaObject::Connection> m_geometryConnections;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SplitButtonIndicatorState, int, m_type,
                                         int(Enum::ButtonType::BtFilled),
                                         &SplitButtonIndicatorState::typeChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SplitButtonIndicatorState, int, m_size,
                                         int(Enum::ButtonSize::S),
                                         &SplitButtonIndicatorState::sizeChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SplitButtonIndicatorState, bool, m_isRound, true,
                                         &SplitButtonIndicatorState::isRoundChanged)
    BindingLifetime m_lifetime { bindingSet().lifetime() };
};
} // namespace qml_material
