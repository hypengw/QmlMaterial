#pragma once
#include "qml_material/style/common_state.hpp"
#include <QQuickItem>
namespace qml_material
{
class QML_MATERIAL_API InputState : public CommonState {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(bool error READ error WRITE setError RESET resetError NOTIFY errorChanged BINDABLE
                   bindableError)
public:
    ~InputState() override;
    bool            error() const;
    void            setError(bool);
    void            resetError();
    QBindable<bool> bindableError();
    Q_SIGNAL void   errorChanged();
    Q_SIGNAL void   itemChanged();

protected:
    explicit InputState(QObject* parent = nullptr);
    enum class Interaction
    {
        Base,
        Disabled,
        ErrorHover,
        Error,
        Focus,
        Hovered
    };
    StateBindingSet<Interaction> m_bindings { Interaction::Base };
    QProperty<bool>              m_acceptable { true }, m_focused { false }, m_hovered { false };
    QQuickItem*                  inputItem() const;
    void                         setInputItem(QQuickItem*);
    void bindInputAppearance(const PropertyKey<QColor>& label, const PropertyKey<qreal>& opacity);

private:
    struct Selection {
        Interaction state                              = Interaction::Base;
        quint64     generation                         = 0;
        bool        operator==(const Selection&) const = default;
    };
    void                           selectionChanged();
    QProperty<QQuickItem*>         m_item { nullptr };
    QProperty<bool>                m_disabled { false };
    QList<QMetaObject::Connection> m_connections;
    bool                           m_ready = false;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(InputState, bool, m_error, false,
                                         &InputState::errorChanged)
    Q_OBJECT_BINDABLE_PROPERTY(InputState, Selection, m_selection, &InputState::selectionChanged)
};
} // namespace qml_material
