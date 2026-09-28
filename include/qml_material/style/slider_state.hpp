#pragma once
#include <functional>
#include "qml_material/style/common_state.hpp"
Q_MOC_INCLUDE("qml_material/control/slider.hpp")
Q_MOC_INCLUDE("qml_material/control/range_slider.hpp")
namespace qml_material
{
class Slider;
class RangeSlider;
class Control;
class QML_MATERIAL_API SliderAppearance : public CommonState {
    Q_OBJECT
    QML_ANONYMOUS
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
    explicit SliderAppearance(QObject* parent = nullptr);
    ~SliderAppearance() override;
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
    Control*                     control() const;
    void                         setControl(Control*, std::function<bool()> pressed);
    Q_SIGNAL void                controlChanged();

private:
    void selectionChanged();
    struct Selection {
        Interaction state                              = Interaction::Base;
        quint64     generation                         = 0;
        bool        operator==(const Selection&) const = default;
    };
    QProperty<Control*>            m_control { nullptr };
    QProperty<bool>                m_pressed { false };
    QProperty<bool>                m_disabled { false };
    QList<QMetaObject::Connection> m_connections;
    bool                           m_ready = false;
    Q_OBJECT_BINDABLE_PROPERTY(SliderAppearance, Selection, m_selection,
                               &SliderAppearance::selectionChanged)
    Q_OBJECT_BINDABLE_PROPERTY(SliderAppearance, QColor, m_trackColor,
                               &SliderAppearance::trackColorChanged)
    PropertyKey<QColor> m_trackColorKey;
    Q_OBJECT_BINDABLE_PROPERTY(SliderAppearance, QColor, m_trackOverlayColor,
                               &SliderAppearance::trackOverlayColorChanged)
    PropertyKey<QColor> m_trackOverlayColorKey;
    Q_OBJECT_BINDABLE_PROPERTY(SliderAppearance, QColor, m_trackInactiveColor,
                               &SliderAppearance::trackInactiveColorChanged)
    PropertyKey<QColor> m_trackInactiveColorKey;
    Q_OBJECT_BINDABLE_PROPERTY(SliderAppearance, QColor, m_trackMarkInactiveColor,
                               &SliderAppearance::trackMarkInactiveColorChanged)
    PropertyKey<QColor> m_trackMarkInactiveColorKey;
    Q_OBJECT_BINDABLE_PROPERTY(SliderAppearance, QColor, m_trackMarkColor,
                               &SliderAppearance::trackMarkColorChanged)
    PropertyKey<QColor> m_trackMarkColorKey;
    Q_OBJECT_BINDABLE_PROPERTY(SliderAppearance, qreal, m_trackOverlayOpacity,
                               &SliderAppearance::trackOverlayOpacityChanged)
    PropertyKey<qreal>                     m_trackOverlayOpacityKey;
    StateBindingSet<Interaction>::Lifetime m_lifetime { m_bindings.lifetime() };
};
class QML_MATERIAL_API SliderHandleAppearance : public SliderAppearance {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(
        int handleLineWidth READ handleLineWidth WRITE setHandleLineWidth RESET resetHandleLineWidth
            NOTIFY handleLineWidthChanged BINDABLE bindableHandleLineWidth)
    Q_PROPERTY(int handleWidth READ handleWidth WRITE setHandleWidth RESET resetHandleWidth NOTIFY
                   handleWidthChanged BINDABLE bindableHandleWidth)
    Q_PROPERTY(int handleHeight READ handleHeight WRITE setHandleHeight RESET resetHandleHeight
                   NOTIFY handleHeightChanged BINDABLE bindableHandleHeight)
public:
    explicit SliderHandleAppearance(QObject* parent = nullptr);
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
    Q_OBJECT_BINDABLE_PROPERTY(SliderHandleAppearance, int, m_handleLineWidth,
                               &SliderHandleAppearance::handleLineWidthChanged)
    PropertyKey<int> m_handleLineWidthKey;
    Q_OBJECT_BINDABLE_PROPERTY(SliderHandleAppearance, int, m_handleWidth,
                               &SliderHandleAppearance::handleWidthChanged)
    PropertyKey<int> m_handleWidthKey;
    Q_OBJECT_BINDABLE_PROPERTY(SliderHandleAppearance, int, m_handleHeight,
                               &SliderHandleAppearance::handleHeightChanged)
    PropertyKey<int>                       m_handleHeightKey;
    StateBindingSet<Interaction>::Lifetime m_lifetime { m_bindings.lifetime() };
};
class QML_MATERIAL_API SliderM2State : public SliderAppearance {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateSliderM2)
    Q_PROPERTY(qml_material::Slider* item READ item WRITE setItem NOTIFY itemChanged FINAL)
public:
    explicit SliderM2State(QObject* parent = nullptr);
    Slider*       item() const;
    void          setItem(Slider*);
    Q_SIGNAL void itemChanged();
};
class QML_MATERIAL_API SliderState : public SliderHandleAppearance {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateSlider)
    Q_PROPERTY(qml_material::Slider* item READ item WRITE setItem NOTIFY itemChanged FINAL)
public:
    explicit SliderState(QObject* parent = nullptr);
    Slider*       item() const;
    void          setItem(Slider*);
    Q_SIGNAL void itemChanged();
};
class QML_MATERIAL_API RangeSliderState : public SliderHandleAppearance {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateRangeSlider)
    Q_PROPERTY(qml_material::RangeSlider* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(int handleIndex READ handleIndex WRITE setHandleIndex NOTIFY handleIndexChanged
                   BINDABLE bindableHandleIndex FINAL)
public:
    explicit RangeSliderState(QObject* parent = nullptr);
    RangeSlider*   item() const;
    void           setItem(RangeSlider*);
    Q_SIGNAL void  itemChanged();
    int            handleIndex() const { return m_handleIndex.value(); }
    void           setHandleIndex(int value) { m_handleIndex = value; }
    QBindable<int> bindableHandleIndex() { return QBindable<int>(&m_handleIndex); }
    Q_SIGNAL void  handleIndexChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(RangeSliderState, int, m_handleIndex, -1,
                                         &RangeSliderState::handleIndexChanged)
};
} // namespace qml_material
