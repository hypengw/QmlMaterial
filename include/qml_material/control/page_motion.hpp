#pragma once
#include "qml_material/anim/seekable_transition.hpp"
#include <QQuickItem>
namespace qml_material
{
class QML_MATERIAL_API PageMotionContext : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by PageStack")
    Q_PROPERTY(QQuickItem* sourceItem READ sourceItem CONSTANT FINAL)
    Q_PROPERTY(QQuickItem* targetItem READ targetItem CONSTANT FINAL)
    Q_PROPERTY(int operation READ operation CONSTANT FINAL)
    Q_PROPERTY(QSizeF viewport READ viewport NOTIFY geometryChanged FINAL)
    Q_PROPERTY(QSizeF sourceSize READ sourceSize NOTIFY geometryChanged FINAL)
    Q_PROPERTY(QSizeF targetSize READ targetSize NOTIFY geometryChanged FINAL)
public:
    enum Operation
    {
        Push,
        Replace,
        Pop,
        Clear
    };
    Q_ENUM(Operation)
    PageMotionContext(QQuickItem* from, QQuickItem* to, int operation, QObject* parent);
    QQuickItem* sourceItem() const { return m_source; }
    QQuickItem* targetItem() const { return m_target; }
    int         operation() const { return m_operation; }
    QSizeF      viewport() const { return m_viewport; }
    QSizeF      sourceSize() const;
    QSizeF      targetSize() const;
    void        setViewport(QSizeF v) {
        if (v == m_viewport) return;
        m_viewport = v;
        emit geometryChanged();
    }
    Q_SIGNAL void geometryChanged();

private:
    QPointer<QQuickItem> m_source, m_target;
    int                  m_operation;
    QSizeF               m_viewport;
};
class QML_MATERIAL_API PageMotion : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(PageMotionContext* context READ context WRITE setContext NOTIFY contextChanged FINAL)
    Q_PROPERTY(
        SeekableTransition* timeline READ timeline WRITE setTimeline NOTIFY timelineChanged FINAL)
    Q_PROPERTY(AnimationSpec spec READ spec WRITE setSpec NOTIFY specChanged FINAL)
    Q_PROPERTY(qreal enterOpacity READ enterOpacity WRITE setEnterOpacity NOTIFY frameChanged FINAL)
    Q_PROPERTY(qreal exitOpacity READ exitOpacity WRITE setExitOpacity NOTIFY frameChanged FINAL)
    Q_PROPERTY(qreal enterScale READ enterScale WRITE setEnterScale NOTIFY frameChanged FINAL)
    Q_PROPERTY(qreal exitScale READ exitScale WRITE setExitScale NOTIFY frameChanged FINAL)
    Q_PROPERTY(QPointF enterOffset READ enterOffset WRITE setEnterOffset NOTIFY frameChanged FINAL)
    Q_PROPERTY(QPointF exitOffset READ exitOffset WRITE setExitOffset NOTIFY frameChanged FINAL)
public:
    explicit PageMotion(QObject* parent = nullptr);
    ~PageMotion() override;
    PageMotionContext*  context() const { return m_context; }
    void                setContext(PageMotionContext*);
    SeekableTransition* timeline() const { return m_timeline; }
    void                setTimeline(SeekableTransition*);
    AnimationSpec       spec() const { return m_spec; }
    void                setSpec(AnimationSpec);
    void                setSettings(MotionSettings*);
    qreal               enterOpacity() const;
    qreal               exitOpacity() const;
    qreal               enterScale() const;
    qreal               exitScale() const;
    QPointF             enterOffset() const { return m_enterOffset; }
    QPointF             exitOffset() const { return m_exitOffset; }
    void                setEnterOpacity(qreal);
    void                setExitOpacity(qreal);
    void                setEnterScale(qreal);
    void                setExitScale(qreal);
    void                setEnterOffset(QPointF);
    void                setExitOffset(QPointF);
    Q_SIGNAL void       contextChanged();
    Q_SIGNAL void       timelineChanged();
    Q_SIGNAL void       specChanged();
    Q_SIGNAL void       frameChanged();

private:
    QPointer<PageMotionContext> m_context;
    SeekableTransition          m_default { this };
    RealTrack m_enterOpacityTrack { this }, m_exitOpacityTrack { this }, m_enterScaleTrack { this },
        m_exitScaleTrack { this };
    QPointer<SeekableTransition> m_timeline;
    QMetaObject::Connection      m_frame, m_destroyed;
    AnimationSpec                m_spec;
    bool                         m_explicitSpec = false;
    std::optional<qreal>         m_enterOpacity, m_exitOpacity, m_enterScale, m_exitScale;
    QPointF                      m_enterOffset, m_exitOffset;
};
} // namespace qml_material
