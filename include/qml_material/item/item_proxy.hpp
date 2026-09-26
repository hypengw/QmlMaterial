#pragma once

#include <QPointer>
#include <QQuickItem>
#include <qqmlregistration.h>
#include "qml_material/export.hpp"

namespace qml_material
{
class QML_MATERIAL_API ItemProxy : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QQuickItem* target READ target WRITE setTarget NOTIFY targetChanged FINAL)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged FINAL)
    Q_PROPERTY(bool controlling READ controlling NOTIFY controllingChanged FINAL)
public:
    explicit ItemProxy(QQuickItem* parent = nullptr);
    ~ItemProxy() override;
    QQuickItem*   target() const { return m_target; }
    bool          active() const { return m_active; }
    bool          controlling() const { return m_controlling; }
    void          setTarget(QQuickItem*);
    void          setActive(bool);
    Q_SIGNAL void targetChanged();
    Q_SIGNAL void activeChanged();
    Q_SIGNAL void controllingChanged();

protected:
    void geometryChange(const QRectF&, const QRectF&) override;
    void componentComplete() override;

private:
    void                           acquire();
    void                           release();
    void                           syncGeometry();
    void                           syncImplicitSize();
    QPointer<QQuickItem>           m_target;
    QList<QMetaObject::Connection> m_connections;
    quint64                        m_revision    = 0;
    bool                           m_active      = false;
    bool                           m_controlling = false;
    bool                           m_destroying  = false;
};

} // namespace qml_material
