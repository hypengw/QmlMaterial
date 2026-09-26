#pragma once

#include <QPointer>
#include <QQuickItem>
#include <qqmlregistration.h>
#include "qml_material/export.hpp"

namespace qml_material
{
class NestedScrollHandler;

class QML_MATERIAL_API NestedScroll : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("NestedScroll is an attached property")
    QML_ATTACHED(NestedScroll)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(Qt::Orientations axes READ axes WRITE setAxes NOTIFY axesChanged FINAL)
    Q_PROPERTY(bool wheelEnabled MEMBER m_wheelEnabled NOTIFY wheelEnabledChanged FINAL)
    Q_PROPERTY(bool restoreOnReverse MEMBER m_restoreOnReverse NOTIFY restoreOnReverseChanged FINAL)
public:
    explicit NestedScroll(QObject* parent);
    ~NestedScroll() override;
    static NestedScroll* qmlAttachedProperties(QObject*);
    static bool          coordinates(QQuickItem* receiver, QQuickItem* ancestor);
    bool                 enabled() const { return m_enabled; }
    void                 setEnabled(bool);
    Qt::Orientations     axes() const { return m_axes; }
    void                 setAxes(Qt::Orientations);
    QQuickItem*          item() const { return m_item; }
    Q_SIGNAL void        enabledChanged();
    Q_SIGNAL void        axesChanged();
    Q_SIGNAL void        wheelEnabledChanged();
    Q_SIGNAL void        restoreOnReverseChanged();

private:
    friend class NestedScrollHandler;
    QPointer<QQuickItem>          m_item;
    QPointer<NestedScrollHandler> m_handler;
    QPointer<NestedScrollHandler> m_controller;
    bool                          m_enabled          = false;
    bool                          m_wheelEnabled     = true;
    bool                          m_restoreOnReverse = false;
    Qt::Orientations              m_axes             = Qt::Horizontal | Qt::Vertical;
};
} // namespace qml_material

QML_DECLARE_TYPEINFO(qml_material::NestedScroll, QML_HAS_ATTACHED_PROPERTIES)
