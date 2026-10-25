#pragma once
#include "qml_material/anim/motion_spec.hpp"
#include <QPointer>
namespace qml_material
{
class QML_MATERIAL_API MotionSettings : public QObject {
    Q_OBJECT
    QML_ANONYMOUS
public:
    explicit MotionSettings(QObject* parent = nullptr): QObject(parent) {}
    void          inherit(MotionSettings*);
    Q_SIGNAL void changed();
    Q_PROPERTY(
        AnimationSpec spatial READ spatial WRITE setSpatial RESET resetSpatial NOTIFY changed FINAL)
    AnimationSpec spatial() const;
    void          setSpatial(AnimationSpec);
    void          resetSpatial();
    Q_PROPERTY(
        AnimationSpec effects READ effects WRITE setEffects RESET resetEffects NOTIFY changed FINAL)
    AnimationSpec effects() const;
    void          setEffects(AnimationSpec);
    void          resetEffects();
    Q_PROPERTY(AnimationSpec page READ page WRITE setPage RESET resetPage NOTIFY changed FINAL)
    AnimationSpec page() const;
    void          setPage(AnimationSpec);
    void          resetPage();
    Q_PROPERTY(AnimationSpec container READ container WRITE setContainer RESET resetContainer NOTIFY
                   changed FINAL)
    AnimationSpec container() const;
    void          setContainer(AnimationSpec);
    void          resetContainer();
    Q_PROPERTY(qreal durationScale READ durationScale WRITE setDurationScale RESET
                   resetDurationScale NOTIFY changed FINAL)
    qreal durationScale() const;
    void  setDurationScale(qreal);
    void  resetDurationScale();
    Q_PROPERTY(bool reducedMotion READ reducedMotion WRITE setReducedMotion RESET resetReducedMotion
                   NOTIFY changed FINAL)
    bool reducedMotion() const;
    void setReducedMotion(bool);
    void resetReducedMotion();

private:
    QPointer<MotionSettings>     m_inherited;
    QMetaObject::Connection      m_connection, m_destroyed;
    std::optional<AnimationSpec> m_spatial;
    std::optional<AnimationSpec> m_effects;
    std::optional<AnimationSpec> m_page;
    std::optional<AnimationSpec> m_container;
    std::optional<qreal>         m_durationScale;
    std::optional<bool>          m_reducedMotion;
};
} // namespace qml_material
