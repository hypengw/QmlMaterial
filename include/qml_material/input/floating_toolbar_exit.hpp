#pragma once

#include <QObject>
#include <QPointF>
#include <QQmlParserStatus>
#include <optional>
#include <qqmlregistration.h>
#include "qml_material/export.hpp"

namespace qml_material
{
class QML_MATERIAL_API FloatingToolbarExit : public QObject, public QQmlParserStatus {
    Q_OBJECT
    QML_ELEMENT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(qreal distance READ distance WRITE setDistance NOTIFY distanceChanged FINAL)
    Q_PROPERTY(qreal offset READ offset WRITE setOffset NOTIFY offsetChanged FINAL)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged FINAL)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(bool reverseLayout READ reverseLayout WRITE setReverseLayout NOTIFY
                   reverseLayoutChanged FINAL)
public:
    explicit FloatingToolbarExit(QObject* parent = nullptr);
    void             classBegin() override;
    void             componentComplete() override;
    qreal            distance() const { return m_distance; }
    qreal            offset() const { return m_offset; }
    bool             active() const { return m_active; }
    bool             enabled() const { return m_enabled; }
    bool             reverseLayout() const { return m_reverseLayout; }
    void             setDistance(qreal);
    void             setOffset(qreal);
    void             setEnabled(bool);
    void             setReverseLayout(bool);
    Q_INVOKABLE void begin();
    Q_INVOKABLE void scrollBy(QPointF consumed);
    Q_INVOKABLE void settle();
    Q_INVOKABLE void reset();
    Q_SIGNAL void    distanceChanged();
    Q_SIGNAL void    offsetChanged();
    Q_SIGNAL void    activeChanged();
    Q_SIGNAL void    inputStarted();
    Q_SIGNAL void    enabledChanged();
    Q_SIGNAL void    reverseLayoutChanged();

private:
    bool                 m_initializing = false;
    std::optional<qreal> m_initialOffset;
    void                 update(qreal offset, bool active);
    qreal                m_distance      = 0;
    qreal                m_offset        = 0;
    bool                 m_active        = false;
    bool                 m_enabled       = true;
    bool                 m_reverseLayout = false;
};
} // namespace qml_material
