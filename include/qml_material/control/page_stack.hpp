#pragma once

#include "qml_material/control/control.hpp"
#include <memory>
#include "qml_material/anim/size_transition.hpp"

#include "qml_material/control/page_motion.hpp"
#include "qml_material/input/input_host.hpp"
#include <QQmlComponent>

namespace qml_material
{
class PageStackAttached;
class QML_MATERIAL_API PageStack : public Control {
    Q_OBJECT
    QML_NAMED_ELEMENT(PageStackBase)
    QML_ATTACHED(PageStackAttached)
    Q_PROPERTY(int depth READ depth NOTIFY depthChanged FINAL)
    Q_PROPERTY(bool empty READ empty NOTIFY depthChanged FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged FINAL)
    Q_PROPERTY(SizeTransition* sizeTransition READ sizeTransition WRITE setSizeTransition NOTIFY
                   sizeTransitionChanged FINAL)
    Q_PROPERTY(
        QQmlComponent* pushMotion READ pushMotion WRITE setPushMotion NOTIFY motionsChanged FINAL)
    Q_PROPERTY(
        QQmlComponent* popMotion READ popMotion WRITE setPopMotion NOTIFY motionsChanged FINAL)
    Q_PROPERTY(QQmlComponent* replaceMotion READ replaceMotion WRITE setReplaceMotion NOTIFY
                   motionsChanged FINAL)
    Q_PROPERTY(PageMotion* currentMotion READ currentMotion NOTIFY currentMotionChanged FINAL)
    Q_PROPERTY(bool preview READ preview NOTIFY previewChanged FINAL)
    Q_PROPERTY(bool settling READ settling NOTIFY previewChanged FINAL)
    Q_PROPERTY(bool backGestureEnabled READ backGestureEnabled WRITE setBackGestureEnabled NOTIFY
                   backGestureEnabledChanged FINAL)
    Q_PROPERTY(QSizeF destinationSize READ destinationSize NOTIFY destinationSizeChanged FINAL)
    Q_PROPERTY(QQuickItem* currentItem READ currentItem NOTIFY currentItemChanged FINAL)
    Q_PROPERTY(QQuickItem* initialItem READ initialItem WRITE setInitialItem NOTIFY
                   initialItemChanged FINAL)
public:
    enum Status
    {
        Inactive,
        Deactivating,
        Activating,
        Active
    };
    Q_ENUM(Status)
    enum TransitionMode
    {
        Immediate,
        Animated
    };
    Q_ENUM(TransitionMode)
    explicit PageStack(QQuickItem* parent = nullptr);
    ~PageStack() override;
    static PageStackAttached* qmlAttachedProperties(QObject*);
    int                       depth() const;
    bool                      empty() const { return depth() == 0; }
    bool                      busy() const;
    SizeTransition*           sizeTransition() const;
    void                      setSizeTransition(SizeTransition*);
    Q_SIGNAL void             sizeTransitionChanged();
    QQmlComponent*            pushMotion() const;
    QQmlComponent*            popMotion() const;
    QQmlComponent*            replaceMotion() const;
    void                      setPushMotion(QQmlComponent*);
    void                      setPopMotion(QQmlComponent*);
    void                      setReplaceMotion(QQmlComponent*);
    PageMotion*               currentMotion() const;
    bool                      preview() const;
    bool                      settling() const;
    bool                      backGestureEnabled() const;
    void                      setBackGestureEnabled(bool);
    QSizeF                    destinationSize() const;
    Q_INVOKABLE QString       beginPopPreview();
    Q_INVOKABLE bool          updatePopPreview(QString id, qreal progress);
    Q_INVOKABLE bool          commitPopPreview(QString id, qreal velocity = 0);
    Q_INVOKABLE bool          cancelPopPreview(QString id, qreal velocity = 0);
    Q_INVOKABLE void          abortPreview();
    Q_INVOKABLE void          completeTransition();
    Q_SIGNAL void             busyChanged();
    Q_SIGNAL void             motionsChanged();
    Q_SIGNAL void             currentMotionChanged();
    Q_SIGNAL void             previewChanged();
    Q_SIGNAL void             backGestureEnabledChanged();
    Q_SIGNAL void             destinationSizeChanged();
    QQuickItem*               currentItem() const;
    QQuickItem*               initialItem() const;
    void                      setInitialItem(QQuickItem*);
    Q_INVOKABLE bool          pushItem(QQuickItem*, TransitionMode mode = Animated);
    Q_INVOKABLE bool          replaceCurrentItem(QQuickItem*, TransitionMode mode = Animated);
    Q_INVOKABLE bool          popCurrentItem(TransitionMode mode = Animated);
    Q_INVOKABLE bool          clear(TransitionMode mode = Animated);
    Q_INVOKABLE QQuickItem*   itemAt(int index) const;
    Q_SIGNAL void             depthChanged();
    Q_SIGNAL void             currentItemChanged();
    Q_SIGNAL void             initialItemChanged();
    Q_SIGNAL void             entryRemoved(quint64 id);
    Q_SIGNAL void             entryAdded(quint64 id, QQuickItem* item);

protected:
    void   enableDefaultMotion();
    void   componentComplete() override;
    void   geometryChange(const QRectF&, const QRectF&) override;
    bool   childMouseEventFilter(QQuickItem*, QEvent*) override;
    QSizeF measureImplicitContent() const override;

private:
    struct State;
    std::shared_ptr<State> m_state;
    bool                   navigate(int operation, QQuickItem*, TransitionMode);
    void                   synchronize();
    void                   layoutPages();
    void                   removePage(quint64);
    void                   startTransition(quint64);
    void                   applyMotion();
    bool                   settlePreview(QString, bool, qreal);
    void                   tryFinishTransition();
    void                   stopMotions();
    InputHost              m_input;
    QString                m_gesture;
    QPointF                m_gesturePoint;
    qreal                  m_gestureFraction = 0;
};

class QML_MATERIAL_API MaterialPageStack : public PageStack {
    Q_OBJECT
    QML_NAMED_ELEMENT(PageStack)
    QML_ATTACHED(PageStackAttached)
public:
    explicit MaterialPageStack(QQuickItem* parent = nullptr);
};

class QML_MATERIAL_API PageStackAttached : public QObject {
    Q_OBJECT
    Q_PROPERTY(PageStack* view READ view NOTIFY changed FINAL)
    Q_PROPERTY(int index READ index NOTIFY changed FINAL)
    Q_PROPERTY(PageStack::Status status READ status NOTIFY statusChanged FINAL)
public:
    using Status = PageStack::Status;
    explicit PageStackAttached(QObject* parent): QObject(parent) {}
    PageStack*    view() const { return m_view; }
    int           index() const { return m_index; }
    Status        status() const { return m_status; }
    Q_SIGNAL void changed();
    Q_SIGNAL void statusChanged();
    Q_SIGNAL void removed();

private:
    friend class PageStack;
    void                update(PageStack*, int, Status);
    QPointer<PageStack> m_view;
    int                 m_index    = -1;
    Status              m_status   = PageStack::Inactive;
    bool                m_removing = false;
};
} // namespace qml_material
QML_DECLARE_TYPEINFO(qml_material::PageStack, QML_HAS_ATTACHED_PROPERTIES)
QML_DECLARE_TYPEINFO(qml_material::MaterialPageStack, QML_HAS_ATTACHED_PROPERTIES)
