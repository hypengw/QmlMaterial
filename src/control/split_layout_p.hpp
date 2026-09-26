#pragma once

#include <QList>
#include <QtGlobal>
#include <limits>
#include <optional>

namespace qml_material::split_layout
{
struct Pane {
    qreal                implicitSize = 0;
    std::optional<qreal> preferred;
    qreal                minimum    = 0;
    qreal                maximum    = std::numeric_limits<qreal>::infinity();
    qreal                handleSize = 0;
    bool                 visible    = true;
    bool                 fill       = false;
};

struct Geometry {
    qreal preferredSize  = 0;
    qreal position       = 0;
    qreal size           = 0;
    qreal handlePosition = 0;
    qreal handleSize     = 0;
    bool  visible        = false;
    bool  handleVisible  = false;
};

struct Result {
    QList<Geometry> panes;
    int             fillIndex      = -1;
    qreal           extent         = 0;
    qreal           implicitExtent = 0;
};

// Axis-independent geometry; the view owns object lifetimes and applies the result.
Result calculate(const QList<Pane>& panes, qreal available);
Result reveal(const QList<Pane>& panes, const QList<qreal>& progress,
              const QList<qreal>& expandedSizes, qreal available);
struct Resize {
    int   index = -1;
    qreal size  = 0;
};
Resize resize(const QList<Pane>& panes, qreal available, int handleIndex, qreal delta);
} // namespace qml_material::split_layout
