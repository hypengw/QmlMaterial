#pragma once
#include <QColor>
#include <QObject>
#include <QProperty>
#include <QQmlListProperty>
#include <QQmlParserStatus>
#include <qqmlregistration.h>
#include <memory>
#include "qml_material/core/state_bindings.hpp"
#include "qml_material/export.hpp"
#include "qml_material/token/token.hpp"
#include <QPointer>
#include "qml_material/util/corner.hpp"

Q_MOC_INCLUDE("QQuickItem")
Q_MOC_INCLUDE("qml_material/token/color.hpp")
Q_MOC_INCLUDE("qml_material/token/token.hpp")
Q_MOC_INCLUDE("qml_material/style/theme.hpp")
QT_BEGIN_NAMESPACE
class QQuickItem;
QT_END_NAMESPACE
namespace qml_material
{
class MdColorMgr;
class Theme;
struct StateValues {
    Q_GADGET_EXPORT(QML_MATERIAL_API)
    QML_VALUE_TYPE(state_values)
    Q_PROPERTY(QString state MEMBER state CONSTANT FINAL)
    Q_PROPERTY(QColor textColor MEMBER textColor CONSTANT FINAL)
    Q_PROPERTY(QColor backgroundColor MEMBER backgroundColor CONSTANT FINAL)
    Q_PROPERTY(QColor stateLayerColor MEMBER stateLayerColor CONSTANT FINAL)
    Q_PROPERTY(QColor outlineColor MEMBER outlineColor CONSTANT FINAL)
    Q_PROPERTY(QColor supportTextColor MEMBER supportTextColor CONSTANT FINAL)
    Q_PROPERTY(qreal contentOpacity MEMBER contentOpacity CONSTANT FINAL)
    Q_PROPERTY(qreal backgroundOpacity MEMBER backgroundOpacity CONSTANT FINAL)
    Q_PROPERTY(qreal stateLayerOpacity MEMBER stateLayerOpacity CONSTANT FINAL)
    Q_PROPERTY(qreal elevation MEMBER elevation CONSTANT FINAL)
    Q_PROPERTY(qreal corner MEMBER corner CONSTANT FINAL)
public:
    QString state;
    QColor  textColor { Qt::transparent }, backgroundColor { Qt::transparent };
    QColor  outlineColor { Qt::transparent }, supportTextColor { Qt::transparent };
    QColor  stateLayerColor { Qt::transparent };
    qreal   contentOpacity = 1, backgroundOpacity = 1, stateLayerOpacity = 0;
    qreal   elevation = 0, corner = 0;
    bool    operator==(const StateValues&) const = default;
};

class QML_MATERIAL_API CommonState : public QObject, public QQmlParserStatus {
    Q_OBJECT
    QML_NAMED_ELEMENT(CommonState)
    QML_UNCREATABLE("Base class for native control states")
    Q_INTERFACES(QQmlParserStatus)
    Q_CLASSINFO("DefaultProperty", "datas")
    Q_PROPERTY(QQmlListProperty<QObject> datas READ datas FINAL)
    Q_PROPERTY(qml_material::MdColorMgr* colors READ colors WRITE setColors NOTIFY colorsChanged
                   RESET resetColors BINDABLE bindableColors FINAL)
    Q_PROPERTY(qml_material::token::State stateTokens READ stateTokens WRITE setStateTokens NOTIFY
                   stateTokensChanged BINDABLE bindableStateTokens FINAL)
    Q_PROPERTY(
        qml_material::token::Elevation elevationTokens READ elevationTokens WRITE setElevationTokens
            NOTIFY elevationTokensChanged BINDABLE bindableElevationTokens FINAL)
    Q_PROPERTY(qml_material::StateValues values READ values NOTIFY valuesChanged BINDABLE
                   bindableValues FINAL)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor RESET resetTextColor NOTIFY
                   textColorChanged BINDABLE bindableTextColor)
    Q_PROPERTY(
        QColor backgroundColor READ backgroundColor WRITE setBackgroundColor RESET
            resetBackgroundColor NOTIFY backgroundColorChanged BINDABLE bindableBackgroundColor)
    Q_PROPERTY(
        QColor stateLayerColor READ stateLayerColor WRITE setStateLayerColor RESET
            resetStateLayerColor NOTIFY stateLayerColorChanged BINDABLE bindableStateLayerColor)
    Q_PROPERTY(QColor outlineColor READ outlineColor WRITE setOutlineColor RESET resetOutlineColor
                   NOTIFY outlineColorChanged BINDABLE bindableOutlineColor)
    Q_PROPERTY(
        QColor supportTextColor READ supportTextColor WRITE setSupportTextColor RESET
            resetSupportTextColor NOTIFY supportTextColorChanged BINDABLE bindableSupportTextColor)
    Q_PROPERTY(qreal elevation READ elevation WRITE setElevation RESET resetElevation NOTIFY
                   elevationChanged BINDABLE bindableElevation)
    Q_PROPERTY(qreal corner READ corner WRITE setCorner RESET resetCorner NOTIFY cornerChanged
                   BINDABLE bindableCorner)
    Q_PROPERTY(qml_material::CornersGroup corners READ corners WRITE setCorners RESET resetCorners
                   NOTIFY cornersChanged BINDABLE bindableCorners)
    Q_PROPERTY(qreal contentOpacity READ contentOpacity WRITE setContentOpacity RESET
                   resetContentOpacity NOTIFY contentOpacityChanged BINDABLE bindableContentOpacity)
    Q_PROPERTY(qreal backgroundOpacity READ backgroundOpacity WRITE setBackgroundOpacity RESET
                   resetBackgroundOpacity NOTIFY backgroundOpacityChanged BINDABLE
                       bindableBackgroundOpacity)
    Q_PROPERTY(qreal stateLayerOpacity READ stateLayerOpacity WRITE setStateLayerOpacity RESET
                   resetStateLayerOpacity NOTIFY stateLayerOpacityChanged BINDABLE
                       bindableStateLayerOpacity)
    Q_PROPERTY(
        qml_material::Theme* ctx READ ctx WRITE setCtx RESET resetCtx NOTIFY ctxChanged FINAL)
    Q_PROPERTY(QObject* target READ target NOTIFY targetChanged)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged FINAL)

public:
    ~CommonState() override;
    QObject*                    target() const;
    MdColorMgr*                 colors() const;
    void                        setColors(MdColorMgr*);
    QBindable<MdColorMgr*>      bindableColors();
    StateValues                 values() const;
    QBindable<StateValues>      bindableValues() const;
    QQmlListProperty<QObject>   datas();
    void                        classBegin() override;
    void                        componentComplete() override;
    token::State                stateTokens() const;
    void                        setStateTokens(const token::State&);
    QBindable<token::State>     bindableStateTokens();
    token::Elevation            elevationTokens() const;
    void                        setElevationTokens(const token::Elevation&);
    QBindable<token::Elevation> bindableElevationTokens();

    QColor                  textColor() const;
    void                    setTextColor(const QColor&);
    Q_INVOKABLE void        resetTextColor();
    QBindable<QColor>       bindableTextColor();
    Q_SIGNAL void           textColorChanged();
    QColor                  backgroundColor() const;
    void                    setBackgroundColor(const QColor&);
    Q_INVOKABLE void        resetBackgroundColor();
    QBindable<QColor>       bindableBackgroundColor();
    Q_SIGNAL void           backgroundColorChanged();
    QColor                  stateLayerColor() const;
    void                    setStateLayerColor(const QColor&);
    Q_INVOKABLE void        resetStateLayerColor();
    QBindable<QColor>       bindableStateLayerColor();
    Q_SIGNAL void           stateLayerColorChanged();
    QColor                  outlineColor() const;
    void                    setOutlineColor(const QColor&);
    Q_INVOKABLE void        resetOutlineColor();
    QBindable<QColor>       bindableOutlineColor();
    Q_SIGNAL void           outlineColorChanged();
    QColor                  supportTextColor() const;
    void                    setSupportTextColor(const QColor&);
    Q_INVOKABLE void        resetSupportTextColor();
    QBindable<QColor>       bindableSupportTextColor();
    Q_SIGNAL void           supportTextColorChanged();
    qreal                   elevation() const;
    void                    setElevation(const qreal&);
    Q_INVOKABLE void        resetElevation();
    QBindable<qreal>        bindableElevation();
    Q_SIGNAL void           elevationChanged();
    qreal                   corner() const;
    void                    setCorner(const qreal&);
    Q_INVOKABLE void        resetCorner();
    QBindable<qreal>        bindableCorner();
    Q_SIGNAL void           cornerChanged();
    CornersGroup            corners() const;
    void                    setCorners(const CornersGroup&);
    Q_INVOKABLE void        resetCorners();
    QBindable<CornersGroup> bindableCorners();
    Q_SIGNAL void           cornersChanged();
    qreal                   contentOpacity() const;
    void                    setContentOpacity(const qreal&);
    Q_INVOKABLE void        resetContentOpacity();
    QBindable<qreal>        bindableContentOpacity();
    Q_SIGNAL void           contentOpacityChanged();
    qreal                   backgroundOpacity() const;
    void                    setBackgroundOpacity(const qreal&);
    Q_INVOKABLE void        resetBackgroundOpacity();
    QBindable<qreal>        bindableBackgroundOpacity();
    Q_SIGNAL void           backgroundOpacityChanged();
    qreal                   stateLayerOpacity() const;
    void                    setStateLayerOpacity(const qreal&);
    Q_INVOKABLE void        resetStateLayerOpacity();
    QBindable<qreal>        bindableStateLayerOpacity();
    Q_SIGNAL void           stateLayerOpacityChanged();
    Theme*                  ctx() const;
    void                    setCtx(Theme*);
    Q_INVOKABLE void        resetCtx();
    Q_INVOKABLE void        resetColors();
    QString                 state() const;
    Q_SIGNAL void           ctxChanged();

    Q_SIGNAL void colorsChanged();
    Q_SIGNAL void targetChanged();
    Q_SIGNAL void stateTokensChanged();
    Q_SIGNAL void elevationTokensChanged();
    Q_SIGNAL void valuesChanged();
    Q_SIGNAL void stateChanged();

protected:
    explicit CommonState(QObject* parent = nullptr);
    void    setTarget(QObject*);
    void    publishState(const QString&, std::function<void()> apply);
    quint64 targetGeneration() const { return m_targetGeneration.value(); }
    template<typename Domain>
    void initializeAppearance(StateBindingSet<Domain>& bindings) {
#define REGISTER(Name, Upper)                                                            \
    m_appearance.Name = bindings.template property<&CommonState::bindable##Upper>(this); \
    m_appearance.Name.reset();
        REGISTER(textColor, TextColor)
        REGISTER(backgroundColor, BackgroundColor)
        REGISTER(stateLayerColor, StateLayerColor)
        REGISTER(outlineColor, OutlineColor)
        REGISTER(supportTextColor, SupportTextColor)
        REGISTER(elevation, Elevation)
        REGISTER(corner, Corner)
        REGISTER(corners, Corners)
        REGISTER(contentOpacity, ContentOpacity)
        REGISTER(backgroundOpacity, BackgroundOpacity)
        REGISTER(stateLayerOpacity, StateLayerOpacity)
#undef REGISTER
        m_applyState = [&bindings] {
            bindings.select(Domain {});
        };
    }
    template<typename Role>
    QColor color(Role role) const {
        const auto* palette = colors();
        return palette ? std::invoke(role, palette) : QColor(Qt::transparent);
    }
    template<typename Role>
    auto colorBinding(Role role) const {
        return [this, role] {
            return color(role);
        };
    }

    struct AppearanceKeys {
        PropertyKey<QColor> textColor, backgroundColor, stateLayerColor, outlineColor,
            supportTextColor;
        PropertyKey<qreal> elevation, corner, contentOpacity, backgroundOpacity, stateLayerOpacity;
        PropertyKey<CornersGroup> corners;
    } m_appearance;

private:
    void                  updateContext(Theme*);
    void                  colorsChange();
    QProperty<QString>    m_state;
    std::function<void()> m_applyState;
    bool                  m_canApply = true;

    QPointer<QObject>       m_target;
    QMetaObject::Connection m_targetDestroyed;
    QProperty<Theme*>       m_context { nullptr };
    Q_OBJECT_BINDABLE_PROPERTY(CommonState, MdColorMgr*, m_colors, &CommonState::colorsChange)
    bool                           m_explicitContext = false;
    QList<QMetaObject::Connection> m_contextConnections, m_colorConnections;
    QList<QObject*>                m_datas;
    Q_OBJECT_BINDABLE_PROPERTY(CommonState, token::State, m_stateTokens,
                               &CommonState::stateTokensChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CommonState, token::Elevation, m_elevationTokens,
                               &CommonState::elevationTokensChanged)
    QProperty<quint64> m_targetGeneration { 0 };
    Q_OBJECT_BINDABLE_PROPERTY(CommonState, StateValues, m_values, &CommonState::valuesChanged)

    Q_OBJECT_BINDABLE_PROPERTY(CommonState, QColor, m_textColorValue,
                               &CommonState::textColorChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CommonState, QColor, m_backgroundColorValue,
                               &CommonState::backgroundColorChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CommonState, QColor, m_stateLayerColorValue,
                               &CommonState::stateLayerColorChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CommonState, QColor, m_outlineColorValue,
                               &CommonState::outlineColorChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CommonState, QColor, m_supportTextColorValue,
                               &CommonState::supportTextColorChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CommonState, qreal, m_elevationValue, &CommonState::elevationChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CommonState, qreal, m_cornerValue, &CommonState::cornerChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CommonState, CornersGroup, m_cornersValue,
                               &CommonState::cornersChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CommonState, qreal, m_contentOpacityValue,
                               &CommonState::contentOpacityChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CommonState, qreal, m_backgroundOpacityValue,
                               &CommonState::backgroundOpacityChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CommonState, qreal, m_stateLayerOpacityValue,
                               &CommonState::stateLayerOpacityChanged)
};
} // namespace qml_material
