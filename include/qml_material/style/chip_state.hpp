#pragma once
#include "qml_material/style/button_interaction_state.hpp"
Q_MOC_INCLUDE("qml_material/control/button.hpp")
namespace qml_material
{
class Button;
class QML_MATERIAL_API ChipState : public ButtonInteractionState {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(qml_material::Button* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(bool elevated READ elevated WRITE setElevated NOTIFY elevatedChanged BINDABLE
                   bindableElevated FINAL)

public:
    Button*         item() const;
    void            setItem(Button*);
    bool            elevated() const;
    void            setElevated(bool);
    QBindable<bool> bindableElevated();
    Q_SIGNAL void   elevatedChanged();

protected:
    enum class Kind
    {
        Assist,
        Suggestion,
        Filter,
        Input,
        Embed
    };
    ChipState(Kind, QObject*);
    QColor resolveIconColor(bool leading) const;

private:
    bool       selected() const;
    QColor     defaultTextColor() const;
    const Kind m_kind;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(ChipState, bool, m_elevated, false,
                                         &ChipState::elevatedChanged)
    BindingLifetime m_bindingLifetime { bindingSet().lifetime() };
};
class QML_MATERIAL_API SingleIconChipState : public ChipState {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QColor iconColor READ iconColor WRITE setIconColor RESET resetIconColor NOTIFY
                   iconColorChanged BINDABLE bindableIconColor FINAL)

public:
    QColor            iconColor() const;
    void              setIconColor(const QColor&);
    QBindable<QColor> bindableIconColor();
    Q_INVOKABLE void  resetIconColor();
    Q_SIGNAL void     iconColorChanged();

protected:
    SingleIconChipState(Kind, QObject*);

private:
    Q_OBJECT_BINDABLE_PROPERTY(SingleIconChipState, QColor, m_iconColor,
                               &SingleIconChipState::iconColorChanged)
    PropertyKey<QColor> m_iconKey;
    BindingLifetime     m_bindingLifetime { bindingSet().lifetime() };
};
class QML_MATERIAL_API DualIconChipState : public ChipState {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QColor leadingIconColor READ leadingIconColor WRITE setLeadingIconColor RESET
                   resetLeadingIconColor NOTIFY leadingIconColorChanged BINDABLE
                       bindableLeadingIconColor FINAL)
    Q_PROPERTY(QColor trailingIconColor READ trailingIconColor WRITE setTrailingIconColor RESET
                   resetTrailingIconColor NOTIFY trailingIconColorChanged BINDABLE
                       bindableTrailingIconColor FINAL)

public:
    QColor            leadingIconColor() const;
    void              setLeadingIconColor(const QColor&);
    QBindable<QColor> bindableLeadingIconColor();
    Q_INVOKABLE void  resetLeadingIconColor();
    Q_SIGNAL void     leadingIconColorChanged();
    QColor            trailingIconColor() const;
    void              setTrailingIconColor(const QColor&);
    QBindable<QColor> bindableTrailingIconColor();
    Q_INVOKABLE void  resetTrailingIconColor();
    Q_SIGNAL void     trailingIconColorChanged();

protected:
    DualIconChipState(Kind, QObject*);

private:
    Q_OBJECT_BINDABLE_PROPERTY(DualIconChipState, QColor, m_leadingIconColor,
                               &DualIconChipState::leadingIconColorChanged)
    Q_OBJECT_BINDABLE_PROPERTY(DualIconChipState, QColor, m_trailingIconColor,
                               &DualIconChipState::trailingIconColorChanged)
    PropertyKey<QColor> m_leadingIconKey, m_trailingIconKey;
    BindingLifetime     m_bindingLifetime { bindingSet().lifetime() };
};
class QML_MATERIAL_API AssistChipState : public SingleIconChipState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateAssistChip)

public:
    explicit AssistChipState(QObject* parent = nullptr);
};
class QML_MATERIAL_API SuggestionChipState : public SingleIconChipState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateSuggestionChip)

public:
    explicit SuggestionChipState(QObject* parent = nullptr);
};
class QML_MATERIAL_API FilterChipState : public DualIconChipState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateFilterChip)

public:
    explicit FilterChipState(QObject* parent = nullptr);
};
class QML_MATERIAL_API InputChipState : public DualIconChipState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateInputChip)

public:
    explicit InputChipState(QObject* parent = nullptr);
};
class QML_MATERIAL_API EmbedChipState : public DualIconChipState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateEmbedChip)
    Q_PROPERTY(int borderWidth READ borderWidth WRITE setBorderWidth NOTIFY borderWidthChanged
                   BINDABLE bindableBorderWidth FINAL)

public:
    explicit EmbedChipState(QObject* parent = nullptr);
    int            borderWidth() const;
    void           setBorderWidth(int);
    QBindable<int> bindableBorderWidth();
    Q_SIGNAL void  borderWidthChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(EmbedChipState, int, m_borderWidth, 0,
                                         &EmbedChipState::borderWidthChanged)
};
} // namespace qml_material
