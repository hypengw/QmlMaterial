#pragma once

#include <QObject>
#include <QPointF>
#include <QPointer>
#include <qqmlregistration.h>
#include "qml_material/export.hpp"

namespace qml_material
{
class NestedScroll;

class QML_MATERIAL_API NestedScrollConnection : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Use a concrete scroll behavior")
public:
    enum Source
    {
        Drag,
        Wheel,
        Fling
    };
    Q_ENUM(Source)
    explicit NestedScrollConnection(QObject* parent = nullptr): QObject(parent) {}
    // Vectors use local content displacement, not pointer displacement.
    virtual QPointF preScroll(QPointF available, Source source)                    = 0;
    virtual QPointF postScroll(QPointF consumed, QPointF available, Source source) = 0;
    virtual bool    canConsume(QPointF delta, Source source) const                 = 0;
    virtual void    begin(Source source)                                           = 0;
    virtual void    end(bool cancelled)                                            = 0;
    Q_SIGNAL void   invalidated();

private:
    friend class NestedScroll;
    QPointer<QObject> m_attachment;
};
} // namespace qml_material
