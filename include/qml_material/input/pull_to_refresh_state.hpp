#pragma once

#include "qml_material/input/nested_scroll_connection.hpp"

namespace qml_material
{
class QML_MATERIAL_API PullToRefreshState : public NestedScrollConnection {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qreal threshold READ threshold WRITE setThreshold NOTIFY thresholdChanged FINAL)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(bool refreshing READ refreshing WRITE setRefreshing NOTIFY refreshingChanged FINAL)
    Q_PROPERTY(bool settling READ settling WRITE setSettling NOTIFY settlingChanged FINAL)
    Q_PROPERTY(bool dragging READ dragging NOTIFY draggingChanged FINAL)
    Q_PROPERTY(bool armed READ armed NOTIFY distanceChanged FINAL)
    Q_PROPERTY(qreal offset READ offset NOTIFY distanceChanged FINAL)
    Q_PROPERTY(qreal distanceFraction READ distanceFraction NOTIFY distanceChanged FINAL)
public:
    explicit PullToRefreshState(QObject* parent = nullptr);
    qreal         threshold() const { return m_threshold; }
    bool          enabled() const { return m_enabled; }
    bool          refreshing() const { return m_refreshing; }
    bool          settling() const { return m_settling; }
    bool          dragging() const { return m_dragging; }
    bool          armed() const { return m_dragging && m_rawDistance * 0.5 > m_threshold; }
    qreal         offset() const { return m_offset; }
    qreal         distanceFraction() const { return m_offset / m_threshold; }
    void          setThreshold(qreal);
    void          setEnabled(bool);
    void          setRefreshing(bool);
    void          setSettling(bool);
    QPointF       preScroll(QPointF, Source) override;
    QPointF       postScroll(QPointF, QPointF, Source) override;
    bool          canConsume(QPointF, Source) const override;
    void          begin(Source) override;
    QPointF       release(QPointF velocity) override;
    void          end(bool cancelled) override;
    Q_SIGNAL void thresholdChanged();
    Q_SIGNAL void enabledChanged();
    Q_SIGNAL void refreshingChanged();
    Q_SIGNAL void settlingChanged();
    Q_SIGNAL void draggingChanged();
    Q_SIGNAL void distanceChanged();
    Q_SIGNAL void refreshRequested();

private:
    void    update(qreal rawDistance, bool dragging);
    QPointF consume(QPointF available);
    qreal   m_threshold   = 80;
    qreal   m_rawDistance = 0;
    qreal   m_offset      = 0;
    bool    m_enabled     = true;
    bool    m_refreshing  = false;
    bool    m_settling    = false;
    bool    m_dragging    = false;
    quint64 m_revision    = 0;
};
} // namespace qml_material
