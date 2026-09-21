#pragma once

#include "qml_material/control/control.hpp"
#include <memory>
#include <QtQuick/private/qquicktransition_p.h>

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
    Q_PROPERTY(QQuickTransition* pushEnter READ pushEnter WRITE setPushEnter NOTIFY
                   transitionsChanged FINAL)
    Q_PROPERTY(
        QQuickTransition* pushExit READ pushExit WRITE setPushExit NOTIFY transitionsChanged FINAL)
    Q_PROPERTY(
        QQuickTransition* popEnter READ popEnter WRITE setPopEnter NOTIFY transitionsChanged FINAL)
    Q_PROPERTY(
        QQuickTransition* popExit READ popExit WRITE setPopExit NOTIFY transitionsChanged FINAL)
    Q_PROPERTY(QQuickTransition* replaceEnter READ replaceEnter WRITE setReplaceEnter NOTIFY
                   transitionsChanged FINAL)
    Q_PROPERTY(QQuickTransition* replaceExit READ replaceExit WRITE setReplaceExit NOTIFY
                   transitionsChanged FINAL)
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
    QQuickTransition*         pushEnter() const;
    QQuickTransition*         pushExit() const;
    QQuickTransition*         popEnter() const;
    QQuickTransition*         popExit() const;
    QQuickTransition*         replaceEnter() const;
    QQuickTransition*         replaceExit() const;
    void                      setPushEnter(QQuickTransition*);
    void                      setPushExit(QQuickTransition*);
    void                      setPopEnter(QQuickTransition*);
    void                      setPopExit(QQuickTransition*);
    void                      setReplaceEnter(QQuickTransition*);
    void                      setReplaceExit(QQuickTransition*);
    Q_INVOKABLE void          completeTransition();
    Q_SIGNAL void             busyChanged();
    Q_SIGNAL void             transitionsChanged();
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
    void componentComplete() override;
    void geometryChange(const QRectF&, const QRectF&) override;
    bool childMouseEventFilter(QQuickItem*, QEvent*) override;

private:
    struct State;
    std::shared_ptr<State> m_state;
    bool                   navigate(int operation, QQuickItem*, TransitionMode);
    void                   synchronize();
    void                   layoutPages();
    void                   removePage(quint64);
    void                   startTransition(quint64);
    void                   motionFinished(quint64);
    void                   stopMotions();
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
