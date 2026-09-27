#pragma once
#include "qml_material/style/common_state.hpp"
#include "qml_material/token/type_scale.hpp"
Q_MOC_INCLUDE("qml_material/control/tool_bar.hpp")
namespace qml_material
{
class ToolBar;
class QML_MATERIAL_API AppBarState : public CommonState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateAppBar)
    Q_PROPERTY(qml_material::ToolBar* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(int type READ type WRITE setType NOTIFY typeChanged BINDABLE bindableType)
    Q_PROPERTY(bool showBackground READ showBackground WRITE setShowBackground NOTIFY
                   showBackgroundChanged BINDABLE bindableShowBackground)
    Q_PROPERTY(qml_material::token::TypeScaleItem typescale READ typescale WRITE setTypescale RESET
                   resetTypescale NOTIFY typescaleChanged BINDABLE bindableTypescale)
    Q_PROPERTY(
        int containerHeight READ containerHeight WRITE setContainerHeight RESET resetContainerHeight
            NOTIFY containerHeightChanged BINDABLE bindableContainerHeight)
public:
    explicit AppBarState(QObject* parent = nullptr);
    ~AppBarState() override;
    ToolBar*                        item() const;
    void                            setItem(ToolBar*);
    Q_SIGNAL void                   itemChanged();
    int                             type() const;
    void                            setType(int);
    QBindable<int>                  bindableType();
    Q_SIGNAL void                   typeChanged();
    bool                            showBackground() const;
    void                            setShowBackground(bool);
    QBindable<bool>                 bindableShowBackground();
    Q_SIGNAL void                   showBackgroundChanged();
    token::TypeScaleItem            typescale() const;
    void                            setTypescale(const token::TypeScaleItem&);
    QBindable<token::TypeScaleItem> bindableTypescale();
    void                            resetTypescale();
    Q_SIGNAL void                   typescaleChanged();
    int                             containerHeight() const;
    void                            setContainerHeight(int);
    QBindable<int>                  bindableContainerHeight();
    void                            resetContainerHeight();
    Q_SIGNAL void                   containerHeightChanged();

private:
    enum class Specification
    {
        Base,
        Small,
        Medium,
        Large
    };
    struct Selection {
        Specification state                              = Specification::Base;
        quint64       generation                         = 0;
        bool          operator==(const Selection&) const = default;
    };
    void                           selectionChanged();
    StateBindingSet<Specification> m_bindings { Specification::Base };
    QProperty<ToolBar*>            m_item { nullptr };
    QMetaObject::Connection        m_destroyed;
    bool                           m_ready = false;
    Q_OBJECT_BINDABLE_PROPERTY(AppBarState, Selection, m_selection, &AppBarState::selectionChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(AppBarState, int, m_type,
                                         int(Enum::AppBarType::AppBarCenterAligned),
                                         &AppBarState::typeChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(AppBarState, bool, m_showBackground, false,
                                         &AppBarState::showBackgroundChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AppBarState, token::TypeScaleItem, m_typescale,
                               &AppBarState::typescaleChanged)
    Q_OBJECT_BINDABLE_PROPERTY(AppBarState, int, m_containerHeight,
                               &AppBarState::containerHeightChanged)
    PropertyKey<token::TypeScaleItem>        m_typescaleKey;
    PropertyKey<int>                         m_heightKey;
    StateBindingSet<Specification>::Lifetime m_lifetime { m_bindings.lifetime() };
};
} // namespace qml_material
