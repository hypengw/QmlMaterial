#pragma once

#include <QObject>
#include <QPointF>
#include <qqmlregistration.h>
#include "qml_material/export.hpp"

namespace qml_material
{
// Observes consumed content displacement; requests never overwrite the caller's expanded binding.
class QML_MATERIAL_API FloatingToolbarScroll : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool expanded READ expanded WRITE setExpanded NOTIFY expandedChanged FINAL)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(bool reverseLayout READ reverseLayout WRITE setReverseLayout NOTIFY
                   reverseLayoutChanged FINAL)
    Q_PROPERTY(qreal expandScrollThreshold READ expandScrollThreshold WRITE setExpandScrollThreshold
                   NOTIFY expandScrollThresholdChanged FINAL)
    Q_PROPERTY(qreal collapseScrollThreshold READ collapseScrollThreshold WRITE
                   setCollapseScrollThreshold NOTIFY collapseScrollThresholdChanged FINAL)
public:
    explicit FloatingToolbarScroll(QObject* parent = nullptr);
    bool             expanded() const { return m_expanded; }
    bool             enabled() const { return m_enabled; }
    bool             reverseLayout() const { return m_reverseLayout; }
    qreal            expandScrollThreshold() const { return m_expandThreshold; }
    qreal            collapseScrollThreshold() const { return m_collapseThreshold; }
    void             setExpanded(bool);
    void             setEnabled(bool);
    void             setReverseLayout(bool);
    void             setExpandScrollThreshold(qreal);
    void             setCollapseScrollThreshold(qreal);
    Q_INVOKABLE void scrollBy(QPointF consumed);
    Q_INVOKABLE void reset();
    Q_SIGNAL void    expandedChanged();
    Q_SIGNAL void    enabledChanged();
    Q_SIGNAL void    reverseLayoutChanged();
    Q_SIGNAL void    expandScrollThresholdChanged();
    Q_SIGNAL void    collapseScrollThresholdChanged();
    Q_SIGNAL void    expandRequested();
    Q_SIGNAL void    collapseRequested();

private:
    bool  m_expanded          = true;
    bool  m_enabled           = true;
    bool  m_reverseLayout     = false;
    qreal m_expandThreshold   = 40;
    qreal m_collapseThreshold = 40;
    qreal m_remaining         = 40;
};
} // namespace qml_material
