#pragma once
#include "qml_material/style/common_state.hpp"
Q_MOC_INCLUDE("qml_material/control/slider.hpp")
namespace qml_material
{
class Slider;
class QML_MATERIAL_API SliderM2State : public CommonState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateSliderM2)
    Q_PROPERTY(qml_material::Slider* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(QColor trackColor READ trackColor WRITE setTrackColor RESET resetTrackColor NOTIFY
                   trackColorChanged BINDABLE bindableTrackColor)
    Q_PROPERTY(QColor trackOverlayColor READ trackOverlayColor WRITE setTrackOverlayColor RESET
                   resetTrackOverlayColor NOTIFY trackOverlayColorChanged BINDABLE
                       bindableTrackOverlayColor)
    Q_PROPERTY(QColor trackInactiveColor READ trackInactiveColor WRITE setTrackInactiveColor RESET
                   resetTrackInactiveColor NOTIFY trackInactiveColorChanged BINDABLE
                       bindableTrackInactiveColor)
    Q_PROPERTY(QColor trackMarkInactiveColor READ trackMarkInactiveColor WRITE
                   setTrackMarkInactiveColor RESET resetTrackMarkInactiveColor NOTIFY
                       trackMarkInactiveColorChanged BINDABLE bindableTrackMarkInactiveColor)
    Q_PROPERTY(QColor trackMarkColor READ trackMarkColor WRITE setTrackMarkColor RESET
                   resetTrackMarkColor NOTIFY trackMarkColorChanged BINDABLE bindableTrackMarkColor)
    Q_PROPERTY(qreal trackOverlayOpacity READ trackOverlayOpacity WRITE setTrackOverlayOpacity RESET
                   resetTrackOverlayOpacity NOTIFY trackOverlayOpacityChanged BINDABLE
                       bindableTrackOverlayOpacity)
public:
    explicit SliderM2State(QObject* parent = nullptr);
    ~SliderM2State() override;
    Slider*           item() const;
    void              setItem(Slider*);
    Q_SIGNAL void     itemChanged();
    QColor            trackColor() const;
    void              setTrackColor(const QColor&);
    void              resetTrackColor();
    QBindable<QColor> bindableTrackColor();
    Q_SIGNAL void     trackColorChanged();
    QColor            trackOverlayColor() const;
    void              setTrackOverlayColor(const QColor&);
    void              resetTrackOverlayColor();
    QBindable<QColor> bindableTrackOverlayColor();
    Q_SIGNAL void     trackOverlayColorChanged();
    QColor            trackInactiveColor() const;
    void              setTrackInactiveColor(const QColor&);
    void              resetTrackInactiveColor();
    QBindable<QColor> bindableTrackInactiveColor();
    Q_SIGNAL void     trackInactiveColorChanged();
    QColor            trackMarkInactiveColor() const;
    void              setTrackMarkInactiveColor(const QColor&);
    void              resetTrackMarkInactiveColor();
    QBindable<QColor> bindableTrackMarkInactiveColor();
    Q_SIGNAL void     trackMarkInactiveColorChanged();
    QColor            trackMarkColor() const;
    void              setTrackMarkColor(const QColor&);
    void              resetTrackMarkColor();
    QBindable<QColor> bindableTrackMarkColor();
    Q_SIGNAL void     trackMarkColorChanged();
    qreal             trackOverlayOpacity() const;
    void              setTrackOverlayOpacity(const qreal&);
    void              resetTrackOverlayOpacity();
    QBindable<qreal>  bindableTrackOverlayOpacity();
    Q_SIGNAL void     trackOverlayOpacityChanged();

protected:
    enum class Interaction
    {
        Base,
        Disabled,
        Pressed,
        Hovered
    };
    StateBindingSet<Interaction> m_bindings { Interaction::Base };
    bool                         active() const;

private:
    void selectionChanged();
    struct Selection {
        Interaction state                              = Interaction::Base;
        quint64     generation                         = 0;
        bool        operator==(const Selection&) const = default;
    };
    QProperty<Slider*>             m_item { nullptr };
    QProperty<bool>                m_disabled { false };
    QList<QMetaObject::Connection> m_connections;
    bool                           m_ready = false;
    Q_OBJECT_BINDABLE_PROPERTY(SliderM2State, Selection, m_selection,
                               &SliderM2State::selectionChanged)
    Q_OBJECT_BINDABLE_PROPERTY(SliderM2State, QColor, m_trackColor,
                               &SliderM2State::trackColorChanged)
    PropertyKey<QColor> m_trackColorKey;
    Q_OBJECT_BINDABLE_PROPERTY(SliderM2State, QColor, m_trackOverlayColor,
                               &SliderM2State::trackOverlayColorChanged)
    PropertyKey<QColor> m_trackOverlayColorKey;
    Q_OBJECT_BINDABLE_PROPERTY(SliderM2State, QColor, m_trackInactiveColor,
                               &SliderM2State::trackInactiveColorChanged)
    PropertyKey<QColor> m_trackInactiveColorKey;
    Q_OBJECT_BINDABLE_PROPERTY(SliderM2State, QColor, m_trackMarkInactiveColor,
                               &SliderM2State::trackMarkInactiveColorChanged)
    PropertyKey<QColor> m_trackMarkInactiveColorKey;
    Q_OBJECT_BINDABLE_PROPERTY(SliderM2State, QColor, m_trackMarkColor,
                               &SliderM2State::trackMarkColorChanged)
    PropertyKey<QColor> m_trackMarkColorKey;
    Q_OBJECT_BINDABLE_PROPERTY(SliderM2State, qreal, m_trackOverlayOpacity,
                               &SliderM2State::trackOverlayOpacityChanged)
    PropertyKey<qreal>                     m_trackOverlayOpacityKey;
    StateBindingSet<Interaction>::Lifetime m_lifetime { m_bindings.lifetime() };
};
class QML_MATERIAL_API SliderState : public SliderM2State {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateSlider)
    Q_PROPERTY(
        int handleLineWidth READ handleLineWidth WRITE setHandleLineWidth RESET resetHandleLineWidth
            NOTIFY handleLineWidthChanged BINDABLE bindableHandleLineWidth)
    Q_PROPERTY(int handleWidth READ handleWidth WRITE setHandleWidth RESET resetHandleWidth NOTIFY
                   handleWidthChanged BINDABLE bindableHandleWidth)
    Q_PROPERTY(int handleHeight READ handleHeight WRITE setHandleHeight RESET resetHandleHeight
                   NOTIFY handleHeightChanged BINDABLE bindableHandleHeight)
public:
    explicit SliderState(QObject* parent = nullptr);
    int            handleLineWidth() const;
    void           setHandleLineWidth(const int&);
    void           resetHandleLineWidth();
    QBindable<int> bindableHandleLineWidth();
    Q_SIGNAL void  handleLineWidthChanged();
    int            handleWidth() const;
    void           setHandleWidth(const int&);
    void           resetHandleWidth();
    QBindable<int> bindableHandleWidth();
    Q_SIGNAL void  handleWidthChanged();
    int            handleHeight() const;
    void           setHandleHeight(const int&);
    void           resetHandleHeight();
    QBindable<int> bindableHandleHeight();
    Q_SIGNAL void  handleHeightChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY(SliderState, int, m_handleLineWidth,
                               &SliderState::handleLineWidthChanged)
    PropertyKey<int> m_handleLineWidthKey;
    Q_OBJECT_BINDABLE_PROPERTY(SliderState, int, m_handleWidth, &SliderState::handleWidthChanged)
    PropertyKey<int> m_handleWidthKey;
    Q_OBJECT_BINDABLE_PROPERTY(SliderState, int, m_handleHeight, &SliderState::handleHeightChanged)
    PropertyKey<int>                       m_handleHeightKey;
    StateBindingSet<Interaction>::Lifetime m_lifetime { m_bindings.lifetime() };
};
} // namespace qml_material
